#include "FileProcessor.h"

namespace ISXFileReader {
	void FileProcessor::ProcessFile(const std::string& path) {
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

	std::string FileProcessor::Trim(const std::string& str) {
		size_t begin = str.find_first_not_of(" \t\r\n");

		if (begin == std::string::npos) return "";

		size_t end = str.find_last_not_of(" \t\r\n");

		return str.substr(begin, end - begin + 1);
	}

	bool FileProcessor::ProcessLine(const std::string& str, bool isBlockComment) {
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
}