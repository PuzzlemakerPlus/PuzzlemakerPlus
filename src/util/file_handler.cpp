// file_handler.cpp
// Class for handling file access
#include "file_handler.h"

#include <algorithm>

FileHandler::FileHandler(std::string& path, const std::string& mode)
{

#ifndef _WIN32
	// Linux doesn't like backslashes let's turn them into forward slashes
	if (path.find('\\') != std::string::npos)
	{
		std::replace(path.begin(), path.end(), '\\', m_PATH_SEPARATOR);
	}
#endif

	// convert the mode combination into it's assioated flags
	std::ios::openmode flagCombination = {};
	for (const char flag : mode)
	{
		switch (flag)
		{
		case 'r': flagCombination |= std::ios::in; break;
		case 'w': flagCombination |= std::ios::out; break;
		case 'b': flagCombination |= std::ios::binary; break;
		case 'e':
			// idk why this flag is called ate, but it opens at the end of the file
			flagCombination |= std::ios::ate;
			break;
		case 't': flagCombination |= std::ios::trunc; break;
		default: throw std::invalid_argument("invalid file mode: " + std::string(1, flag));
		}
	}

	m_file.open(path, flagCombination);
}

FileHandler::~FileHandler()
{
	m_file.close();
}

bool FileHandler::fileExists()
{
	return m_file.good();
}
