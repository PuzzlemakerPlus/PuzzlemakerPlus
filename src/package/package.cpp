// package.cpp
// Represents a single package. Holds a reference to a PackageFormat used for accessing contents.
#include "package.h"

namespace PMP
{

std::optional<std::vector<std::byte>> Package::readFileBinary(std::filesystem::path path)
{
	return m_format->readFileBinary(path);
}

std::optional<std::string> Package::readFileText(std::filesystem::path path)
{
	return m_format->readFileText(path);
}

} // namespace PMP
