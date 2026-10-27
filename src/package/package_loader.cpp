// package_loader.cpp
// Class which enumerates packages in a folder and allows accessing resources from them
#include "package_loader.h"

#include <qdebug.h>
#include <qdirlisting.h>
#include <qfileinfo.h>

#include "formats/archive_format.h"
#include "formats/dir_format.h"
#include "package.h"
#include "package_format.h"

namespace PMP
{

unsigned int PackageLoader::scanPackages()
{
	if (!std::filesystem::exists(m_packageDir.data()))
	{
		qDebug() << "Specified package directory is not present!";
		return 0;
	}

	return scanDirectoryRecursive(m_packageDir);
}

unsigned int PackageLoader::scanDirectoryRecursive(std::string_view dir)
{
	// Have to recurse manually because folders which are packages shouldn't be scanned
	// BEE seems to pick up hidden packages, guess we should too???
	QDirListing dirList(dir.data(), QDirListing::IteratorFlag::IncludeHidden);
	for (const auto& dirEntry : dirList)
	{
		std::unique_ptr<PackageFormat> format;

		if (dirEntry.isDir())
		{
			if (!QFileInfo::exists(dirEntry.absoluteFilePath() + "/package.kv"))
			{
				if (QFileInfo::exists(dirEntry.absoluteFilePath() + "/info.txt"))
				{
					// TODO: Show dialog informing users that BEE2 packages are not supported
					qDebug() << "Ignoring BEE package" << dirEntry.fileName();
				}
				else
				{
					// not a package, so scan deeper
					qDebug() << "Scanning recursive directory" << dirEntry.fileName();
					scanDirectoryRecursive(dirEntry.absoluteFilePath().toStdString());
				}
				continue;
			}

			format = std::make_unique<DirFormat>(dirEntry.absoluteFilePath().toStdString());
		}
		else
		{
			QString extension = dirEntry.suffix().toLower();
			if (extension == "bee_pack")
			{
				// TODO: Show dialog informing users that BEE2 packages are not supported
				qDebug() << "Ignoring BEE package" << dirEntry.fileName();
				continue;
			}
			// Since we really only have 2 package formats, I'm just doing this with a simple
			// if/else for now, we can add a more advanced lookup system later if needed
			if (extension == "zip" || extension == "vpk" || extension == "pmpack")
			{
				format = std::make_unique<ArchiveFormat>(dirEntry.absoluteFilePath().toStdString());
				if (!format->init())
				{
					qDebug() << "Failed to load" << dirEntry.fileName() << "- invalid or corrupt";
				}
			}
			else
			{
				// not a package
				continue;
			}
		}

		qDebug() << "Loading " << dirEntry.fileName();
		auto package = std::make_unique<Package>(format.release());
		if (!package->isInvalid())
			m_packageList.push_back(std::move(package));
	}

	return m_packageList.size();
}

} // namespace PMP
