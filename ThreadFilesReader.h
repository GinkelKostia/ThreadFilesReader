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
#include "Statistics.h"
#include "FileProcessor.h"
#include "FileSearcher.h"
#include "ThreadPool.h"
#include "FileSaver.h"

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

		void Menu();
		void SetRootFolder(const std::string& root);
		void StartSearching();

		ThreadFilesReader();

	private:
		ThreadPool m_thread_pool;
		FileProcessor m_processor;
		FileSearcher m_searcher;
		FileSaver m_saver{ "log.txt" };

		std::string m_rootFolder;

		Statistic m_stats;
	};
}