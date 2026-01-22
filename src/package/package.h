// package.h
// Represents a single package. Holds a reference to a PackageFormat used for accessing contents.
#pragma once

#include <string>
#include <string_view>

namespace PMP
{

class Package
{
public:
	Package(std::string_view name) : m_packageName(name) {}

private:
	std::string m_packageName;
};

} // namespace PMP
