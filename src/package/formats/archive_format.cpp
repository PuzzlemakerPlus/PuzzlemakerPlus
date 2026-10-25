// archive_format.cpp
// Implements package formats using the vpkpp library (covers VPKs and ZIPs)
#include "vpkpp/format/VPK.h"

#include <qdebug.h>

#include "archive_format.h"
#include "sourcepp/String.h"
#include "vpkpp/PackFile.h"
#include "vpkpp/format/ZIP.h"

namespace PMP
{

bool ArchiveFormat::init()
{
	// hackhack: auto detection doesn't work for pmpack so we have to manually tell it it's a zip
	if (m_packagePath.extension().string() == ".pmpack")
		m_packFile = vpkpp::ZIP::open(m_packagePath.string());
	else
		m_packFile = vpkpp::PackFile::open(m_packagePath.string());

	if (!m_packFile)
	{
		qDebug() << "Package file" << m_packagePath.string() << "is invalid, corrupt, or in use by another program";
		return false;
	}

	return true;
}

bool ArchiveFormat::fileExists(std::filesystem::path path)
{
	return m_packFile->hasEntry(path.string());
}

std::optional<std::vector<std::byte>> ArchiveFormat::readFileBinary(std::filesystem::path path)
{
	// TODO: I think this is going to be copying this twice - probably really slow for large files
	return m_packFile->readEntry(path.string());
}

std::optional<std::string> ArchiveFormat::readFileText(std::filesystem::path path)
{
	// TODO: see above
	return m_packFile->readEntryText(path.string());
}

} // namespace PMP
