#pragma once 
#include <atomic>

struct Statistic {
	std::atomic<int> blank_lines{ 0 };
	std::atomic<int> code_lines{ 0 };
	std::atomic<int> comment_lines{ 0 };
	std::atomic<int> files_count{ 0 };

	void Reset() {
		blank_lines = 0;
		code_lines = 0;
		comment_lines = 0;
		files_count = 0;
	}
};