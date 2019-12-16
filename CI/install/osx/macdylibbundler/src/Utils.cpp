#include "Utils.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>

#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Dependency.h"
#include "Settings.h"

void tokenize(const std::string &str, const char *delim,
	      std::vector<std::string> *vectorarg)
{
	std::vector<std::string> &tokens = *vectorarg;
	std::string delimiters(delim);

	// skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// find first "non-delimiter".
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while (pos != std::string::npos || lastPos != std::string::npos) {
		// found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}
}

bool fileExists(std::string filename)
{
	if (access(filename.c_str(), F_OK) != -1) {
		return true; // file exists
	} else {
		std::string delims = " \f\n\r\t\v";
		std::string rtrimmed = filename.substr(
			0, filename.find_last_not_of(delims) + 1);
		std::string ftrimmed =
			rtrimmed.substr(rtrimmed.find_first_not_of(delims));
		if (access(ftrimmed.c_str(), F_OK) != -1)
			return true;
		else
			return false; // file doesn't exist
	}
}

void copyFile(std::string from, std::string to)
{
	bool overwrite = Settings::canOverwriteFiles();
	if (fileExists(to) && !overwrite) {
		std::cerr
			<< "\n\nError: File " << to
			<< " already exists. Remove it or enable overwriting\n";
		exit(1);
	}

	std::string overwrite_permission =
		std::string(overwrite ? "-f " : "-n ");

	// copy file to local directory
	std::string command = std::string("cp -R ") + overwrite_permission +
			      from + std::string(" ") + to;
	if (from != to && systemp(command) != 0) {
		std::cerr
			<< "\n\nError: An error occured while trying to copy file "
			<< from << " to " << to << "\n";
		exit(1);
	}

	// give it write permission
	std::string command2 = std::string("chmod -R +w ") + to;
	if (systemp(command2) != 0) {
		std::cerr
			<< "\n\nError: An error occured while trying to set write permissions on file "
			<< to << "\n";
		exit(1);
	}
}

std::string system_get_output(std::string cmd)
{
	FILE *command_output;
	char output[128];
	int amount_read = 1;
	std::string full_output;

	try {
		command_output = popen(cmd.c_str(), "r");
		if (command_output == NULL)
			throw;

		while (amount_read > 0) {
			amount_read = fread(output, 1, 127, command_output);
			if (amount_read <= 0) {
				break;
			} else {
				output[amount_read] = '\0';
				full_output += output;
			}
		}
	} catch (...) {
		std::cerr << "An error occured while executing command " << cmd
			  << "\n";
		pclose(command_output);
		return "";
	}

	int return_value = pclose(command_output);
	if (return_value != 0)
		return "";

	return full_output;
}

int systemp(std::string &cmd)
{
	if (!Settings::quietOutput()) {
		std::cout << "    " << cmd << "\n";
	}
	return system(cmd.c_str());
}

std::string getUserInputDirForFile(const std::string &filename)
{
	const int searchPathAmount = Settings::searchPathAmount();
	for (int n = 0; n < searchPathAmount; ++n) {
		auto searchPath = Settings::searchPath(n);
		if (!searchPath.empty() &&
		    searchPath[searchPath.size() - 1] != '/')
			searchPath += "/";
		if (!fileExists(searchPath + filename)) {
			continue;
		} else {
			if (!Settings::quietOutput()) {
				std::cerr
					<< (searchPath + filename)
					<< " was found\n"
					<< "/!\\ WARNING: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'"
					<< "\n";
			}
			return searchPath;
		}
	}

	while (true) {
		if (Settings::quietOutput())
			std::cerr
				<< "\n/!\\ WARNING: Library " << filename
				<< " has an incomplete name (location unknown)\n";
		std::cout
			<< "\nPlease specify the directory where this library is located (or enter 'quit' to abort): ";
		fflush(stdout);

		std::string prefix;
		std::cin >> prefix;
		std::cout << std::endl;

		if (prefix.compare("quit") == 0)
			exit(1);

		if (!prefix.empty() && prefix[prefix.size() - 1] != '/')
			prefix += "/";

		if (!fileExists(prefix + filename)) {
			std::cerr << (prefix + filename)
				  << " does not exist. Try again...\n";
			continue;
		} else {
			std::cerr
				<< (prefix + filename) << " was found\n"
				<< "/!\\ WARNINGS: dylibbundler MAY NOT CORRECTLY HANDLE THIS DEPENDENCY: Check the executable with 'otool -L'\n";
			return prefix;
		}
	}
}
