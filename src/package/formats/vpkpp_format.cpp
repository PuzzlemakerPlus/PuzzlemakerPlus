// vpkpp_format.cpp
// Implements package formats using the vpkpp library (covers VPKs and ZIPs)
#include "vpkpp_format.h"

#include <qdebug.h>

#include "sourcepp/String.h"
#include "vpkpp/format/VPK.h"
#include "vpkpp/format/ZIP.h"

namespace PMP
{

bool VpkPPFormat::init()
{
	m_packFile = vpkpp::PackFile::open(m_packagePath.string());
	if (!m_packFile)
	{
		qDebug() << "Package file" << m_packagePath.string() << "is invalid, corrupt, or in use by another program";
		return false;
	}

	return true;
}

bool VpkPPFormat::fileExists(std::filesystem::path path)
{
	return m_packFile->hasEntry(path.string());
}

std::optional<std::vector<std::byte>> VpkPPFormat::readFileBinary(std::filesystem::path path)
{
	// TODO: I think this is going to be copying this twice - probably really slow for large files
	return m_packFile->readEntry(path.string());
}

std::optional<std::string> VpkPPFormat::readFileText(std::filesystem::path path)
{
	// TODO: see above
	return m_packFile->readEntryText(path.string());
}

} // namespace PMP
