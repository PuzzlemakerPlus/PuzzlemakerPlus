// archive_format.h
// Implements VPK and ZIP package formats (both covered by the vpkpp library)
#pragma once

#include "../package_format.h"
#include "vpkpp/PackFile.h"

namespace PMP
{

class ArchiveFormat : public PackageFormat
{
public:
	ArchiveFormat(std::filesystem::path packagePath) : PackageFormat(packagePath) {}

	ArchiveFormat() = delete;

	virtual bool init();

	virtual bool fileExists(std::filesystem::path path);

	virtual std::optional<std::vector<std::byte>> readFileBinary(std::filesystem::path path);
	virtual std::optional<std::string> readFileText(std::filesystem::path path);

private:
	std::unique_ptr<vpkpp::PackFile> m_packFile;
};

} // namespace PMP
