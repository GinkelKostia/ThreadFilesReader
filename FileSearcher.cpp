#include "FileSearcher.h"

namespace ISXFileReader {
	void FileSearcher::Search(const std::string& path, std::function<void(const std::string&)> callback) {
		for (auto& file : std::filesystem::recursive_directory_iterator(path)) {
			if (!file.is_regular_file()) continue;

			if (!IsValidType(file.path())) continue;

			callback(file.path().string());

		}

	}

	bool FileSearcher::IsValidType(const std::filesystem::path& file) {
		std::string type = file.extension().string();

		return type == ".h" || type == ".hpp" || type == ".cpp" || type == ".c";
	}
}