#ifndef _utils_h_
#define _utils_h_

#include <string>
#include <vector>

class Library;

void tokenize(const std::string &str, const char *delimiters,
	      std::vector<std::string> *);
bool fileExists(std::string filename);

void copyFile(std::string from, std::string to);

// executes a command in the native shell and returns output in string
std::string system_get_output(std::string cmd);

// like 'system', runs a command on the system shell, but also prints the command to stdout.
int systemp(std::string &cmd);
std::string getUserInputDirForFile(const std::string &filename);

#endif
