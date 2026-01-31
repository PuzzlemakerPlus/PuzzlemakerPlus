// package_format.h
// Abstract class representing format-specific (dir, zip, vpk, etc.) package operations
#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

namespace PMP
{

class PackageFormat
{
public:
	PackageFormat(std::filesystem::path packagePath) : m_packagePath(packagePath) {}

	PackageFormat() = delete;

	virtual bool init() = 0;

	virtual bool fileExists(std::filesystem::path path) = 0;

	virtual std::optional<std::vector<std::byte>> readFileBinary(std::filesystem::path path) = 0;
	virtual std::optional<std::string> readFileText(std::filesystem::path path) = 0;

protected:
	// Location of the package file/folder on disk
	std::filesystem::path m_packagePath;
};

} // namespace PMP
