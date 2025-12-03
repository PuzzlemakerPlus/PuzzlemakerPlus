//
// Created by hannah on 2025-12-02.
//

#ifndef PUZZLEMAKERPLUS_FILEHANDLER_H
#define PUZZLEMAKERPLUS_FILEHANDLER_H
#include <fstream>

class FileHandler {

private:
	std::fstream m_file;
	std::string path;
#ifdef _WIN32
	const char PATH_SEPARATOR = '\\';
#else
	const char m_PATH_SEPARATOR = '/';
#endif

public:

	/* path can be in / or \ it corrects for you
	 * the different modes are:
	 *r for read,
	 *w for write,
	 *b for binary files,
	 *e for starting at the end of a file,
	 *a for appened mode
	 *t for truncate, where if the file is already opened somewhere it will be deleted and replaced
	 *	(not sure how useful it will be)
	 *Any combinations can be used
	 */
	FileHandler(std::string &path, const std::string &mode);
	~FileHandler();

	bool fileExists();



};


#endif //PUZZLEMAKERPLUS_FILEHANDLER_H