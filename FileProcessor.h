#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "Statistics.h"

namespace ISXFileReader {
	class FileProcessor {
	public:
		FileProcessor(Statistic& stats) : m_stats(stats) {}

		void ProcessFile(const std::string& path);
	private:
		Statistic& m_stats;

		std::string Trim(const std::string& str);
		bool ProcessLine(const std::string& str, bool isBlockComment);
	};
}