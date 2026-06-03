#pragma once
#include "Statistics.h"
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <atomic>

namespace ISXFileReader {
	class FileSaver {
	public:
		FileSaver(const std::string& name): m_filename(name) {}
		void Save(Statistic& stats, std::chrono::milliseconds execution_time) {
			std::ofstream stream(m_filename);
			if (!stream.is_open()) {
				std::cout << "File not opened!" << std::endl;
				return;
			}

			stream << "Time of execution: " << execution_time.count() << " ms\n";
			stream << "Blank Lines: " << std::to_string(stats.blank_lines.load()) << '\n';
			stream << "Comment Lines: " << std::to_string(stats.comment_lines.load()) << '\n';
			stream << "Code Lines: " << std::to_string(stats.code_lines.load()) << '\n';
			stream << "Files processed: " << stats.files_count.load() << '\n';

			stats.blank_lines.store(0);
			stats.code_lines.store(0);
			stats.comment_lines.store(0);
			stats.files_count.store(0);
		}
	private:
		std::string m_filename;
	};
}