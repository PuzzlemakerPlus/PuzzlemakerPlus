// package.cpp
// Represents a single package. Holds a reference to a PackageFormat used for accessing contents.
#include "package.h"

#include <utility>
#include <qdebug.h>

#include <kvpp/kvpp.h>

namespace PMP
{

Package::Package(PackageFormat* format) : m_format(format)
{
	if (format->fileExists("info.txt"))
	{
		qDebug() << "\tInvalid package: BEE2.4 packages are not supported";
		m_isInvalid = true;
		return;
	}

	const auto text = format->readFileText("package.kv");
	if (!text.has_value())
	{
		qDebug() << "\tInvalid package: missing package.kv file";
		m_isInvalid = true;
		return;
	}

	const kvpp::KV1 packageKV{text.value()};

	if (!packageKV.hasChild("ID"))
	{
		qDebug() << "\tInvalid package: no ID specified";
		m_isInvalid = true;
		return;
	}

	// init metadata
	m_packageID = packageKV["ID"].getValue();
	m_packageName = packageKV["Name"].getValue();
	m_packageDescription = packageKV["Description"].getValue();
	m_packageAuthor = packageKV["Author"].getValue();
	qDebug() << "\tLoaded" << m_packageName << "(" << m_packageID << ") by" << m_packageAuthor << "-" << m_packageDescription;
}

std::optional<std::vector<std::byte>> Package::readFileBinary(std::filesystem::path path) const
{
	return m_format->readFileBinary(std::move(path));
}

std::optional<std::string> Package::readFileText(std::filesystem::path path) const
{
	return m_format->readFileText(std::move(path));
}

} // namespace PMP
