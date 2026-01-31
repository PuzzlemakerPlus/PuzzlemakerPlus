// package.h
// Represents a single package. Holds a reference to a PackageFormat used for accessing contents.
#pragma once

#include <string>
#include <string_view>

#include "package_format.h"

namespace PMP
{

class Package
{
public:
	Package(std::string_view name, PackageFormat* format) : m_packageName(name), m_format(format) {}

	std::optional<std::vector<std::byte>> readFileBinary(std::filesystem::path path);
	std::optional<std::string> readFileText(std::filesystem::path path);

private:
	std::string m_packageName;
	std::unique_ptr<PackageFormat> m_format;
};

} // namespace PMP
