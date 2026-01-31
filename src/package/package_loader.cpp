// package_loader.cpp
// Class which enumerates packages in a folder and allows accessing resources from them
#include "package_loader.h"

#include <qdebug.h>
#include <qdirlisting.h>
#include <qfileinfo.h>

#include "formats/dir_format.h"
#include "formats/vpkpp_format.h"
#include "package.h"
#include "package_format.h"

namespace PMP
{

bool PackageLoader::scanPackages()
{
	if (!std::filesystem::exists(m_packageDir.data()))
	{
		qDebug() << "Specified package directory is not present!";
		return false;
	}

	return scanDirectoryRecursive(m_packageDir);
}

bool PackageLoader::scanDirectoryRecursive(std::string_view dir)
{
	// Have to do recursion manually because folders which are packages shouldn't be scanned
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
					qDebug() << "Skipped BEE package" << dirEntry.fileName();
					continue;
				}
				else
				{
					// not a package, so scan deeper
					qDebug() << "Scanning recursive directory" << dirEntry.fileName();
					scanDirectoryRecursive(dirEntry.absoluteFilePath().toStdString());
					continue;
				}
			}

			format = std::make_unique<DirFormat>(dirEntry.absoluteFilePath().toStdString());
		}
		else
		{
			QString extension = dirEntry.suffix().toLower();
			if (extension == "bee_pack")
			{
				// TODO: Show dialog informing users that BEE2 packages are not supported
				qDebug() << "Skipped BEE package" << dirEntry.fileName();
				continue;
			}
			// VPKpp supports a bunch of formats, but to avoid confusion, we are only allowing
			// ZIP (optionally renamed to PMPACK) and VPK formats to be used in PMP
			// TODO: pmpack doesn't work because sourcepp doesn't know that extension
			else if (extension == "zip" || extension == "vpk" || extension == "pmpack")
			{
				format = std::make_unique<VpkPPFormat>(dirEntry.absoluteFilePath().toStdString());
				if (!format->init())
				{
					qDebug() << "Failed to load package" << dirEntry.fileName();
				}
			}
			else
			{
				// not a package
				qDebug() << "Skipped non-package" << dirEntry.fileName();
				continue;
			}
		}

		// transfer format unique_ptr ownership to the package
		m_packageList.push_back(std::make_unique<Package>(dirEntry.baseName().toStdString(), format.release()));
		qDebug() << "Added package " << dirEntry.fileName();
		continue;
	}

	return (m_packageList.size() > 0);
}

} // namespace PMP
