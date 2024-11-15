#pragma once
#ifndef UTILS_HXX
#define UTILS_HXX

#include <filesystem>
#include <string>

namespace trash{

/* contains common shared program utilities */

//This exists because it does two actions and by default directory_entry follows symbolic links. And I don't want todo std::filesystem::exists(entry.symlink_status) every single time.
struct Verity
{
	std::filesystem::file_type type{};
	bool exists{};

	Verity(const std::filesystem::directory_entry& entry);
};

std::string fileTypeToString(std::filesystem::file_type ft);

void externRename(const std::filesystem::path& filePath, const std::filesystem::path& newCopy);
bool canMvFileChk(const std::filesystem::directory_entry& file);
bool canReadDirChk(const std::filesystem::directory_entry& entry);
std::array<std::array<bool, 3>, 3> getFilePerms(const std::filesystem::path& file);
unsigned long long directorySize(const std::filesystem::directory_entry& directory);
int formatToTimestamp(const std::string& format, long long& timestamp);
std::string dataUnitConversion();

//Add a thread pool here

} //namespace trash

#endif //UTILS_HXX
