// dir_format.cpp
// Format for unpacked folder packages
#include "dir_format.h"

#include <filesystem>
#include <fstream>
#include <iterator>
#include <qdebug.h>

namespace PMP
{

bool DirFormat::fileExists(std::filesystem::path path)
{
	return std::filesystem::exists(m_packagePath / path);
}

std::optional<std::vector<std::byte>> DirFormat::readFileBinary(std::filesystem::path path)
{
	if (!fileExists(path))
		return std::nullopt;

	std::ifstream stream(m_packagePath / path, std::ios::binary | std::ios::ate);
	if (!stream.is_open())
	{
		qDebug() << "Failed to open file" << path.string() << "- file in use?";
		return std::nullopt;
	}

	auto size = stream.tellg();
	std::vector<std::byte> out;
	out.reserve(size);

	// read the whole file
	stream.seekg(0);
	stream.read((char*)out[0], size);
	return out;
}

std::optional<std::string> DirFormat::readFileText(std::filesystem::path path)
{
	if (!fileExists(path))
		return std::nullopt;

	std::ifstream stream(m_packagePath / path, std::ios::ate);
	if (!stream.is_open())
	{
		qDebug() << "Failed to open file" << path.string() << "- file in use?";
		return std::nullopt;
	}

	stream.seekg(0);
	std::string out(std::istreambuf_iterator<char>(stream), {});
	return out;
}

} // namespace PMP
