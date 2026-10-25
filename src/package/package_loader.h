// package_loader.h
// Class which enumerates packages in a folder and allows accessing resources from them
#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "package.h"

namespace PMP
{

class PackageLoader
{
public:
	PackageLoader(std::string_view packageDir) : m_packageDir(packageDir) {}

	unsigned long scanPackages();

protected:
	// Scans a folder for packages. Subfolders that are not themselves packages
	// will be scanned for further packages.
	unsigned long scanDirectoryRecursive(std::string_view dir);

private:
	std::string m_packageDir;
	std::vector<std::unique_ptr<Package>> m_packageList;
};

} // namespace PMP
