// dir_format.h
// Format for unpacked folder packages
#pragma once

#include "../package_format.h"

namespace PMP
{

class DirFormat : public PackageFormat
{
public:
	DirFormat(std::filesystem::path packagePath) : PackageFormat(packagePath) {}

	DirFormat() = delete;

	// Not an archive format, so no special init needed
	virtual bool init() { return true; }

	virtual bool fileExists(std::filesystem::path path);

	virtual std::optional<std::vector<std::byte>> readFileBinary(std::filesystem::path path);
	virtual std::optional<std::string> readFileText(std::filesystem::path path);
};

} // namespace PMP
