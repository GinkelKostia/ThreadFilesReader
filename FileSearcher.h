#pragma once
#include <filesystem>
#include <functional>
#include <string>

namespace ISXFileReader {
	class FileSearcher {
	public:
		FileSearcher() {};

		void Search(const std::string& path, std::function<void(const std::string&)> callback);
	private:
		bool IsValidType(const std::filesystem::path& file);
	};
}