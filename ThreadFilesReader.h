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

namespace ISXFileReader {

	class ThreadFilesReader {
	public:
		int GetBlankLines() const {
			return m_stats.blank_lines.load();
		}
		int GetCodeLines() const {
			return m_stats.code_lines.load();
		}
		int GetCommentLines() const {
			return m_stats.comment_lines.load();
		}
		int GetFilesCount() const {
			return m_stats.files_count.load();
		}

		std::string Trim(const std::string& str);
		bool ProcessLine(const std::string& str, bool isBlockComment);
		void ProcessFile(const std::string& path);
		bool IsValidType(const std::filesystem::path& file);
		void Menu();

		ThreadFilesReader();
		~ThreadFilesReader();

	private:
		std::condition_variable m_cv;
		std::vector<std::thread> m_threads;
		std::queue<std::string> m_file_queue;
		std::mutex m_thread_mutex;
		std::size_t m_thread_num;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
		std::string m_rootFolder;

		bool m_is_save = false;
		bool m_is_end = false;

		struct Stat {
			std::atomic<int> blank_lines{ 0 };
			std::atomic<int> code_lines{ 0 };
			std::atomic<int> comment_lines{ 0 };
			std::atomic<int> files_count{ 0 };
		};

		Stat m_stats;

		void SearchThread(const std::string& path);
		void SaveToFile();
		void SetRootFolder(const std::string& root);
		void StartSearching();
		void WorkThread();
	};
}