#include "DylibBundler.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#ifdef __linux
#include <linux/limits.h>
#endif

#include "Dependency.h"
#include "Settings.h"
#include "Utils.h"

std::vector<Dependency> deps;
std::map<std::string, std::vector<Dependency>> deps_per_file;
std::set<std::string> rpaths;
std::map<std::string, std::vector<std::string>> rpaths_per_file;
std::map<std::string, bool> deps_collected;

void changeLibPathsOnFile(std::string file_to_fix)
{
	if (deps_collected.find(file_to_fix) == deps_collected.end())
		collectDependencies(file_to_fix);

	std::cout << "* Fixing dependencies on " << file_to_fix << "\n";

	std::vector<Dependency> deps_in_file = deps_per_file[file_to_fix];
	const int dep_amount = deps_in_file.size();
	for (int n = 0; n < dep_amount; ++n)
		deps_in_file[n].fixFileThatDependsOnMe(file_to_fix);
}

bool isRpath(const std::string &path)
{
	return path.find("@rpath") == 0 || path.find("@loader_path") == 0;
}

void collectRpaths(const std::string &filename)
{
	if (!fileExists(filename)) {
		std::cerr
			<< "\n/!\\ WARNING: Can't collect rpaths for nonexistent file '"
			<< filename << "'\n";
		return;
	}

	std::string cmd = "otool -l " + filename;
	std::string output = system_get_output(cmd);

	std::vector<std::string> lc_lines;
	tokenize(output, "\n", &lc_lines);

	size_t pos = 0;
	bool read_rpath = false;
	while (pos < lc_lines.size()) {
		std::string line = lc_lines[pos];
		pos++;

		if (read_rpath) {
			size_t start_pos = line.find("path ");
			size_t end_pos = line.find(" (");
			if (start_pos == std::string::npos ||
			    end_pos == std::string::npos) {
				std::cerr
					<< "\n/!\\ WARNING: Unexpected LC_RPATH format\n";
				continue;
			}
			start_pos += 5; // to exclude "path "
			std::string rpath =
				line.substr(start_pos, end_pos - start_pos);
			if (Settings::verboseOutput()) {
				std::cout << "rpath line: " << line
					  << std::endl;
				std::cout << "inserting rpath: " << rpath
					  << std::endl;
			}
			rpaths.insert(rpath);
			rpaths_per_file[filename].push_back(rpath);
			read_rpath = false;
			continue;
		}

		if (line.find("LC_RPATH") != std::string::npos) {
			read_rpath = true;
			pos++;
		}
	}
}

void collectRpathsForFilename(const std::string &filename)
{
	if (rpaths_per_file.find(filename) == rpaths_per_file.end())
		collectRpaths(filename);
}

std::string searchFilenameInRpaths(const std::string &rpath_file)
{
	char buffer[PATH_MAX];
	std::string fullpath;
	std::string suffix = rpath_file.substr(rpath_file.rfind("/") + 1);

	for (std::set<std::string>::iterator it = rpaths.begin();
	     it != rpaths.end(); ++it) {
		std::string path = *it + "/" + suffix;
		if (realpath(path.c_str(), buffer)) {
			fullpath = buffer;
			break;
		}
	}

	if (Settings::verboseOutput()) {
		std::cout << "rpath file: " << rpath_file << std::endl;
		std::cout << "suffix: " << suffix << std::endl;
		std::cout << "rpath fullpath: " << fullpath << std::endl;
	}

	if (fullpath.empty()) {
		if (!Settings::quietOutput())
			std::cerr << "\n/!\\ WARNING: Can't get path for '"
				  << rpath_file << "'\n";
		fullpath = getUserInputDirForFile(suffix) + suffix;
		if (Settings::quietOutput() && fullpath.empty())
			std::cerr << "\n/!\\ WARNING: Can't get path for '"
				  << rpath_file << "'\n";
		if (realpath(fullpath.c_str(), buffer))
			fullpath = buffer;
	}

	return fullpath;
}

void fixRpathsOnFile(const std::string &original_file,
		     const std::string &file_to_fix)
{
	std::vector<std::string> rpaths_to_fix;
	std::map<std::string, std::vector<std::string>>::iterator found =
		rpaths_per_file.find(original_file);
	if (found != rpaths_per_file.end())
		rpaths_to_fix = found->second;

	for (size_t i = 0; i < rpaths_to_fix.size(); ++i) {
		std::string command = std::string("install_name_tool -rpath ") +
				      rpaths_to_fix[i] + " " +
				      Settings::insideLibPath() + " " +
				      file_to_fix;
		if (systemp(command) != 0) {
			std::cerr
				<< "\n\n/!\\ ERROR: An error occured while trying to fix dependencies of "
				<< file_to_fix << "\n";
			exit(1);
		}
	}
}

void addDependency(std::string path, std::string filename)
{
	Dependency dep(path);

	// check if this library was already added to |deps| to avoid duplicates
	bool in_deps = false;
	const int dep_amount = deps.size();
	for (int n = 0; n < dep_amount; n++)
		if (dep.mergeIfSameAs(deps[n]))
			in_deps = true;

	// check if this library was already added to |deps_per_file[filename]| to avoid duplicates
	std::vector<Dependency> deps_in_file = deps_per_file[filename];
	bool in_deps_per_file = false;
	for (int n = 0; n < deps_in_file.size(); n++)
		if (dep.mergeIfSameAs(deps_in_file[n]))
			in_deps_per_file = true;

	// check if this library is in /usr/lib, /System/Library, or in ignored list
	if (!Settings::isPrefixBundled(dep.getPrefix()))
		return;

	if (!in_deps)
		deps.push_back(dep);

	if (!in_deps_per_file) {
		deps_in_file.push_back(dep);
		deps_per_file[filename] = deps_in_file;
	}
}

// Fill |lines| with dependencies of given |filename|
void collectDependencies(std::string filename, std::vector<std::string> &lines)
{
	std::string cmd = "otool -L " + filename;
	std::string output = system_get_output(cmd);

	if (output.find("can't open file") != std::string::npos ||
	    output.find("No such file") != std::string::npos ||
	    output.size() < 1) {
		std::cerr << "\n\n/!\\ ERROR: Cannot find file " << filename
			  << " to read its dependencies\n";
		exit(1);
	}
	// split output
	tokenize(output, "\n", &lines);
	deps_collected[filename] = true;
}

void collectDependencies(std::string filename)
{
	std::vector<std::string> lines;
	collectDependencies(filename, lines);

	const int line_amount = lines.size();
	for (int n = 0; n < line_amount; n++) {
		if (!Settings::bundleFrameworks())
			if (lines[n].find(".framework") != std::string::npos)
				continue;
		// lines containing path begin with a tab
		if (lines[n][0] != '\t')
			continue;
		// trim useless info, keep only library path
		std::string dep_path =
			lines[n].substr(1, lines[n].rfind(" (") - 1);
		if (isRpath(dep_path))
			collectRpathsForFilename(filename);

		addDependency(dep_path, filename);
	}
}

void collectSubDependencies()
{
	int dep_amount = deps.size();

	// recursively collect each dependency's dependencies
	while (true) {
		dep_amount = deps.size();

		for (int n = 0; n < dep_amount; n++) {
			std::string original_path = deps[n].getOriginalPath();
			if (Settings::verboseOutput())
				std::cout << "original path: " << original_path
					  << std::endl;
			if (isRpath(original_path))
				original_path =
					searchFilenameInRpaths(original_path);

			collectRpathsForFilename(original_path);

			std::vector<std::string> lines;
			collectDependencies(original_path, lines);

			const int line_amount = lines.size();
			for (int n = 0; n < line_amount; n++) {
				if (!Settings::bundleFrameworks())
					if (lines[n].find(".framework") !=
					    std::string::npos)
						continue;
				// lines containing path begin with a tab
				if (lines[n][0] != '\t')
					continue;
				// trim useless info, keep only library name
				std::string dep_path = lines[n].substr(
					1, lines[n].rfind(" (") - 1);
				std::string full_path = dep_path;
				if (isRpath(dep_path)) {
					full_path = searchFilenameInRpaths(
						dep_path);
					collectRpathsForFilename(full_path);
				}

				addDependency(dep_path, full_path);
			}
		}
		// if no more dependencies were added on this iteration, stop searching
		if (deps.size() == dep_amount)
			break;
	}
}

void createDestDir()
{
	std::string dest_folder = Settings::destFolder();
	std::cout << "* Checking output directory " << dest_folder << "\n";

	bool dest_exists = fileExists(dest_folder);

	if (dest_exists && Settings::canOverwriteDir()) {
		std::cout << "* Erasing old output directory " << dest_folder
			  << "\n";
		std::string command = std::string("rm -r ") + dest_folder;
		if (systemp(command) != 0) {
			std::cerr
				<< "\n\n/!\\ ERROR: An error occured while attempting to overwrite dest folder\n";
			exit(1);
		}
		dest_exists = false;
	}

	if (!dest_exists) {
		if (Settings::canCreateDir()) {
			std::cout << "* Creating output directory "
				  << dest_folder << "\n\n";
			std::string command =
				std::string("mkdir -p ") + dest_folder;
			if (systemp(command) != 0) {
				std::cerr
					<< "\n/!\\ ERROR: An error occured while creating dest folder\n";
				exit(1);
			}
		} else {
			std::cerr
				<< "\n\n/!\\ ERROR: Dest folder does not exist. Create it or pass the appropriate flag for automatic dest dir creation\n";
			exit(1);
		}
	}
}

void doneWithDeps_go()
{
	const int dep_amount = deps.size();

	std::cout << "\n";
	for (int n = 0; n < dep_amount; n++)
		deps[n].print();
	std::cout << "\n";

	// copy files if requested by user
	if (Settings::bundleLibs()) {
		createDestDir();

		for (int i = 0; i < dep_amount; ++i) {
			deps[i].copyYourself();
			changeLibPathsOnFile(deps[i].getInstallPath());
			fixRpathsOnFile(deps[i].getOriginalPath(),
					deps[i].getInstallPath());
		}
	}

	const int fileToFixAmount = Settings::fileToFixAmount();
	for (int n = 0; n < fileToFixAmount; n++) {
		changeLibPathsOnFile(Settings::fileToFix(n));
		fixRpathsOnFile(Settings::fileToFix(n), Settings::fileToFix(n));
	}
}
