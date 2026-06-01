#pragma once
#include <condition_variable>
#include <thread>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <atomic>
#include <filesystem>
#include <chrono>
#include <iostream>

class ThreadFilesReader {
public:
	int getBlankLines() const {
		return stats.blankLines.load();
	}
	int getCodeLines() const {
		return stats.codeLines.load();
	}
	int getCommentLines() const {
		return stats.commentLines.load();
	}
	int getFilesCount() const {
		return stats.filesCount.load();
	}
    std::string trim(const std::string& str);
    bool processLine(const std::string& str, bool isBlockComment);
    void processFile(const std::string& path);
    bool isValidType(const std::filesystem::path& file);

    void handleUserInput();

	ThreadFilesReader();

	~ThreadFilesReader();

private:
	std::condition_variable cv;
	std::vector<std::thread> threads;
	std::queue<std::string> fileQueue;
	std::mutex thread_mutex;
	std::size_t threadNum;
	std::chrono::time_point<std::chrono::high_resolution_clock> start;
	std::string rootFolder;

	bool isSave = false;
	bool isEnd = false;

	struct Stat {
		std::atomic<int> blankLines{ 0 };
		std::atomic<int> codeLines{ 0 };
		std::atomic<int> commentLines{ 0 };
		std::atomic<int> filesCount{ 0 };
	};

	Stat stats;

	void searchThread(const std::string& path);
	void saveToFile();
	void handleRootFolderInput();
	void startSearching();
	void workThread();
};