#include "ThreadFilesReader.h"

ThreadFilesReader::ThreadFilesReader() {
	size_t thread_num = std::thread::hardware_concurrency();
	if (thread_num == 0) thread_num = 4;

	threadNum = thread_num;

	for (int i = 0; i < threadNum - 1; i++) {
		threads.emplace_back(&ThreadFilesReader::workThread, this);
	}
	threads.emplace_back(&ThreadFilesReader::saveToFile, this);
}

ThreadFilesReader::~ThreadFilesReader() {
	{
		std::lock_guard<std::mutex> lock(thread_mutex);
		isEnd = true;
	}

	cv.notify_all();

	for (std::thread& t : threads) {
		t.join();
	}
}

void ThreadFilesReader::searchThread(const std::string& path) {
	for (auto& file : std::filesystem::recursive_directory_iterator(path)) {
		if (!file.is_regular_file()) continue;

		if (!isValidType(file.path())) continue;

		{
			std::lock_guard<std::mutex> lock(thread_mutex);
			fileQueue.push(file.path().string());
			cv.notify_one();
		}
	}

	{
		std::lock_guard<std::mutex> lock(thread_mutex);
		isSave = true;
		cv.notify_all();
	}
}

void ThreadFilesReader::saveToFile() {
	while (true) {
		{
			std::unique_lock<std::mutex> lock(thread_mutex);
			cv.wait(lock, [this]() { return isSave || isEnd; });
			if (isEnd) return;

			std::ofstream stream("log.txt");

			std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();

			stream << "Time of execution: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms\n";
			stream << "Blank Lines: " << std::to_string(stats.blankLines) << '\n';
			stream << "Comment Lines: " << std::to_string(stats.commentLines) << '\n';
			stream << "Code Lines: " << std::to_string(stats.codeLines) << '\n';
			stream << "Files processed: " << stats.filesCount << '\n';

			stats.blankLines = 0;
			stats.codeLines = 0;
			stats.commentLines = 0;
			stats.filesCount = 0;

			isSave = false;
		}
	}
}

void ThreadFilesReader::handleRootFolderInput() {
	std::cout << "Enter the root folder: ";

	std::string temp{};
	std::getline(std::cin, temp);

	std::filesystem::path dir = temp;

	if (std::filesystem::exists(dir) &&
		std::filesystem::is_directory(dir)) {

		rootFolder = temp;
		std::cout << "Root folder assigned successfully!\n";
	}
	else {
		std::cout << "Root folder does not exist!\n";
	}
}

void ThreadFilesReader::startSearching() {
	start = std::chrono::high_resolution_clock::now();
	if (rootFolder.empty()) {
		std::cout << "Root folder was not assigned!\n";
		return;
	}
	std::thread search(&ThreadFilesReader::searchThread, this, ".");
	search.join();
	cv.notify_all();
}

std::string ThreadFilesReader::trim(const std::string& str) {
	size_t begin = str.find_first_not_of(" \t\r\n");

	if (begin == std::string::npos) return "";

	size_t end = str.find_last_not_of(" \t\r\n");

	return str.substr(begin, end - begin + 1);
}
bool ThreadFilesReader::processLine(const std::string& str, bool isBlockComment) {
	if (str.empty()) {
		stats.blankLines++;
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

	if (isComment) stats.commentLines++;
	if (isCode) stats.codeLines++;

	return isBlockComment;
}
void ThreadFilesReader::processFile(const std::string& path) {
	std::ifstream file(path);

	if (!file.is_open()) {
		std::cout << "File not opened!" << std::endl;
		return;
	}

	std::string str;
	bool isBlockComment = false;
	while (std::getline(file, str)) {
		str = trim(str);

		isBlockComment = processLine(str, isBlockComment);
	}

	stats.filesCount++;
}
void ThreadFilesReader::workThread() {
	while (true) {
		std::string file;

		{
			std::unique_lock<std::mutex> lock(thread_mutex);
			cv.wait(lock, [this]() { return !fileQueue.empty() || isEnd; });

			if (fileQueue.empty() && isEnd) return;

			file = fileQueue.front();
			fileQueue.pop();
		}

		processFile(file);
	}
}

bool ThreadFilesReader::isValidType(const std::filesystem::path& file) {
	std::string type = file.extension().string();

	return type == ".h" || type == ".hpp" || type == ".cpp" || type == ".c";
}

void ThreadFilesReader::handleUserInput() {
	std::string menu =
		"=====MENU=====\n"
		"1. Enter the root folder\n"
		"2. Process files\n"
		"3. Exit\n";

	while (true) {
		std::cout << menu;
		std::cout << "Enter the choose: ";

		std::string input;
		std::getline(std::cin, input);

		if (input == "1") {
			handleRootFolderInput();
		}
		else if (input == "2") {
			startSearching();
		}
		else if (input == "3") {
			return;
		}
	}
}
