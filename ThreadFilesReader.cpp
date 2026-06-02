#include "ThreadFilesReader.h"

using namespace ISXFileReader;

ThreadFilesReader::ThreadFilesReader() {
	size_t thread_num = std::thread::hardware_concurrency();
	if (thread_num == 0) thread_num = 4;

	m_thread_num = thread_num;

	for (int i = 0; i < m_thread_num - 1; i++) {
		m_threads.emplace_back(&ThreadFilesReader::WorkThread, this);
	}
	m_threads.emplace_back(&ThreadFilesReader::SaveToFile, this);
}

ThreadFilesReader::~ThreadFilesReader() {
	{
		std::lock_guard<std::mutex> lock(m_thread_mutex);
		m_is_end = true;
	}

	m_cv.notify_all();

	for (std::thread& t : m_threads) {
		t.join();
	}
}

void ThreadFilesReader::SearchThread(const std::string& path) {
	for (auto& file : std::filesystem::recursive_directory_iterator(path)) {
		if (!file.is_regular_file()) continue;

		if (!IsValidType(file.path())) continue;

		{
			std::lock_guard<std::mutex> lock(m_thread_mutex);
			m_file_queue.push(file.path().string());
			m_cv.notify_one();
		}
	}

	{
		std::lock_guard<std::mutex> lock(m_thread_mutex);
		m_is_save = true;
		m_cv.notify_all();
	}
}

void ThreadFilesReader::SaveToFile() {
	while (true) {
		{
			std::unique_lock<std::mutex> lock(m_thread_mutex);
			m_cv.wait(lock, [this]() { return m_is_save || m_is_end; });
			if (m_is_end) return;

			std::ofstream stream("log.txt");

			std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();

			stream << "Time of execution: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start).count() << " ms\n";
			stream << "Blank Lines: " << std::to_string(m_stats.blank_lines) << '\n';
			stream << "Comment Lines: " << std::to_string(m_stats.comment_lines) << '\n';
			stream << "Code Lines: " << std::to_string(m_stats.code_lines) << '\n';
			stream << "Files processed: " << m_stats.files_count << '\n';

			m_stats.blank_lines = 0;
			m_stats.code_lines = 0;
			m_stats.comment_lines = 0;
			m_stats.files_count = 0;

			m_is_save = false;
		}
	}
}

void ThreadFilesReader::SetRootFolder(const std::string& root) {
	std::filesystem::path dir = root;

	if (std::filesystem::exists(dir) &&
		std::filesystem::is_directory(dir)) {

		m_rootFolder = root;
	}
	else {
		std::cout << "Root folder does not exist!\n";
	}
}

void ThreadFilesReader::StartSearching() {
	m_start = std::chrono::high_resolution_clock::now();
	if (m_rootFolder.empty()) {
		std::cout << "Root folder was not assigned!\n";
		return;
	}
	std::thread search(&ThreadFilesReader::SearchThread, this, ".");
	search.join();
	m_cv.notify_all();
}

std::string ThreadFilesReader::Trim(const std::string& str) {
	size_t begin = str.find_first_not_of(" \t\r\n");

	if (begin == std::string::npos) return "";

	size_t end = str.find_last_not_of(" \t\r\n");

	return str.substr(begin, end - begin + 1);
}
bool ThreadFilesReader::ProcessLine(const std::string& str, bool isBlockComment) {
	if (str.empty()) {
		m_stats.blank_lines++;
		return isBlockComment;
	}

	bool isCode = false;
	bool isComment = false;
	bool isInChar = false;
	bool isInString = false;
	bool isEscape = false;

	for (size_t i = 0; i < str.size(); i++) {
		char curr = str[i];
		char last = (i + 1 < str.size()) ? str[i + 1] : '\0';

		if (isBlockComment) {
			isComment = true;

			if (curr == '*' && last == '/') {
				isBlockComment = false;
				++i;
			}

			continue;
		}
		if (curr == '\\' && !isEscape) {
			isEscape = true;

			if (!isInChar && !isInString) isCode = true;

			continue;
		}

		if (curr == '\'' && !isInString && !isEscape) {
			isInChar = !isInChar;
			isCode = true;
			continue;
		}
		if (curr == '"' && !isInChar && !isEscape) {
			isInString = !isInString;
			isCode = true;
			continue;
		}

		if (!isInString && !isInChar) {
			if (curr == '/' && last == '*') {
				isBlockComment = true;
				isComment = true;
				i++;
				continue;
			}
			else if (curr == '/' && last == '/') {
				isComment = true;
				break;
			}
		}

		if (!isspace(static_cast<unsigned char>(curr))) {
			isCode = true;
		}

		isEscape = false;
	}

	if (isComment) m_stats.comment_lines++;
	if (isCode) m_stats.code_lines++;

	return isBlockComment;
}
void ThreadFilesReader::ProcessFile(const std::string& path) {
	std::ifstream file(path);

	if (!file.is_open()) {
		std::cout << "File not opened!" << std::endl;
		return;
	}

	std::string str;
	bool isBlockComment = false;
	while (std::getline(file, str)) {
		str = Trim(str);

		isBlockComment = ProcessLine(str, isBlockComment);
	}

	m_stats.files_count++;
}

void ThreadFilesReader::WorkThread() {
	while (true) {
		std::string file;

		{
			std::unique_lock<std::mutex> lock(m_thread_mutex);
			m_cv.wait(lock, [this]() { return !m_file_queue.empty() || m_is_end; });

			if (m_file_queue.empty() && m_is_end) return;

			file = m_file_queue.front();
			m_file_queue.pop();
		}

		ProcessFile(file);
	}
}

bool ThreadFilesReader::IsValidType(const std::filesystem::path& file) {
	std::string type = file.extension().string();

	return type == ".h" || type == ".hpp" || type == ".cpp" || type == ".c";
}

#include <iostream>
#include <string>

void ThreadFilesReader::Menu()
{
	const std::string menu =
		"====== Menu ======\n"
		"1. Set root folder\n"
		"2. Start searching\n"
		"3. Exit\n";

	while (true)
	{
		std::cout << menu;
		std::cout << "Enter your choice: ";

		std::string input;
		std::getline(std::cin, input);

		if (input.empty())
		{
			std::cout << "Please enter a choice.\n\n";
			continue;
		}

		if (input == "1")
		{
			std::cout << "Enter root folder: ";

			std::string path;
			std::getline(std::cin, path);

			SetRootFolder(path);
		}
		else if (input == "2")
		{
			StartSearching();
		}
		else if (input == "3")
		{
			return;
		}
		else
		{
			std::cout << "Invalid choice!\n\n";
		}
	}
}