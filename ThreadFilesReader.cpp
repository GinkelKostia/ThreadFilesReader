#include "ThreadFilesReader.h"

using namespace ISXFileReader;

ThreadFilesReader::ThreadFilesReader(): m_processor(m_stats) {}

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
	if (m_rootFolder.empty()) {
		std::cout << "Root folder was not assigned!\n";
		return;
	}

	m_stats.Reset();

	auto m_start = std::chrono::high_resolution_clock::now();

	m_searcher.Search(m_rootFolder, [this](const std::string& path) {
		m_thread_pool.AddTask([this, path]() {
			m_processor.ProcessFile(path);
			});
	});

	m_thread_pool.Idle();

	auto end = std::chrono::high_resolution_clock::now();

	m_saver.Save(m_stats, std::chrono::duration_cast<std::chrono::milliseconds>(end - m_start));
}

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