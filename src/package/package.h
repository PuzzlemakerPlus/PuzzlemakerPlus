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
	Package(PackageFormat* format);

	[[nodiscard]] std::optional<std::vector<std::byte>> readFileBinary(std::filesystem::path path) const;
	[[nodiscard]] std::optional<std::string> readFileText(std::filesystem::path path) const;

	// Returns if the package failed to initialize properly so the package loader can ignore it
	[[nodiscard]] bool isInvalid() const {return m_isInvalid;}

	[[nodiscard]] std::string_view getID() const {return m_packageID;}
	[[nodiscard]] std::string_view getName() const {return m_packageName;}
	[[nodiscard]] std::string_view getDescription() const {return m_packageDescription;}
	[[nodiscard]] std::string_view getAuthor() const {return m_packageAuthor;}

private:
	// Metadata that is loaded from package.kv
	std::string m_packageID;
	std::string m_packageName;
	std::string m_packageDescription;
	std::string m_packageAuthor;

	std::unique_ptr<PackageFormat> m_format;

	bool m_isInvalid = false;
};

} // namespace PMP
