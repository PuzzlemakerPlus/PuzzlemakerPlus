// package_loader.cpp
// Class which enumerates packages in a folder and allows accessing resources from them
#include "package_loader.h"

#include <qdebug.h>
#include <qdirlisting.h>
#include <qfileinfo.h>

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
		if (dirEntry.isDir())
		{
			if (!QFileInfo::exists(dirEntry.absoluteFilePath() + "/package.kv"))
			{
				if (QFileInfo::exists(dirEntry.absoluteFilePath() + "/info.txt"))
				{
					// TODO: Show dialog informing users that BEE2 packages are not supported
					qDebug() << "Skipped BEE package " << dirEntry.fileName();
					continue;
				}
				else
				{
					// not a package, so scan deeper
					scanDirectoryRecursive(dirEntry.absoluteFilePath().toStdString());
					continue;
				}
			}
		}
		else if (dirEntry.suffix() == "bee_pack")
		{
			// TODO: Show dialog informing users that BEE2 packages are not supported
			qDebug() << "Skipped BEE package " << dirEntry.fileName();
			continue;
		}

		// todo: package format lookup
		qDebug() << "Added package " << dirEntry.fileName();
		m_packageList.push_back(std::make_unique<Package>(dirEntry.baseName().toStdString()));
		continue;
	}

	return (m_packageList.size() > 0);
}

} // namespace PMP
