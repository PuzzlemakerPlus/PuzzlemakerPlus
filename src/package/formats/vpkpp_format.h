// vpkpp_format.h
// Implements package formats using the vpkpp library (covers VPKs and ZIPs)
#pragma once

#include "../package_format.h"
#include "vpkpp/PackFile.h"

namespace PMP
{

class VpkPPFormat : public PackageFormat
{
public:
	VpkPPFormat(std::filesystem::path packagePath) : PackageFormat(packagePath) {}

	VpkPPFormat() = delete;

	virtual bool init();

	virtual bool fileExists(std::filesystem::path path);

	virtual std::optional<std::vector<std::byte>> readFileBinary(std::filesystem::path path);
	virtual std::optional<std::string> readFileText(std::filesystem::path path);

private:
	std::unique_ptr<vpkpp::PackFile> m_packFile;
};

} // namespace PMP
