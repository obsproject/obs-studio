#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#include "DylibBundler.h"
#include "Settings.h"
#include "Utils.h"

const std::string VERSION = "1.0.0 (2019-12-16)";
std::string installPath = "";

void showHelp()
{
	std::cout << "Usage: dylibbundler [options] -x file" << std::endl;
	std::cout << "Options:" << std::endl;
	std::cout
		<< "  -x,  --fix-file              Object file to bundle dependencies (can enter more than one)"
		<< std::endl;
	std::cout
		<< "  -b,  --bundle-deps           Copy dependencies to app bundle and fix internal names and rpaths"
		<< std::endl;
	std::cout
		<< "  -f,  --bundle-frameworks     Copy dependencies that are frameworks (experimental)"
		<< std::endl;
	std::cout
		<< "  -d,  --dest-dir              Directory (relative) to copy bundled libraries (default: ../Frameworks/)"
		<< std::endl;
	std::cout
		<< "  -p,  --install-path          Inner path (@rpath) of bundled libraries (default: @executable_path/../Frameworks/)"
		<< std::endl;
	std::cout
		<< "  -s,  --search-path           Directory to add to list of locations searched"
		<< std::endl;
	std::cout
		<< "  -of, --overwrite-files       Allow overwriting files in output directory"
		<< std::endl;
	std::cout
		<< "  -od, --overwrite-dir         Overwrite output directory if it exists (implies --create-dir)"
		<< std::endl;
	std::cout
		<< "  -cd, --create-dir            Create output directory if needed"
		<< std::endl;
	std::cout
		<< "  -i,  --ignore                Ignore libraries in this directory (default: /usr/lib & /System/Library)"
		<< std::endl;
	std::cout << "  -q,  --quiet                 Less verbose output"
		  << std::endl;
	std::cout << "  -v,  --verbose               More verbose output"
		  << std::endl;
	std::cout
		<< "  -V,  --version               Print dylibbundler version number and exit"
		<< std::endl;
	std::cout
		<< "  -h,  --help                  Print this message and exit"
		<< std::endl;
}

int main(int argc, char *const argv[])
{
	// parse arguments
	for (int i = 0; i < argc; i++) {
		if (strcmp(argv[i], "-x") == 0 ||
		    strcmp(argv[i], "--fix-file") == 0) {
			i++;
			Settings::addFileToFix(argv[i]);
			continue;
		} else if (strcmp(argv[i], "-b") == 0 ||
			   strcmp(argv[i], "--bundle-deps") == 0) {
			Settings::bundleLibs(true);
			continue;
		} else if (strcmp(argv[i], "-f") == 0 ||
			   strcmp(argv[i], "--bundle-frameworks") == 0) {
			Settings::bundleFrameworks(true);
			continue;
		} else if (strcmp(argv[i], "-p") == 0 ||
			   strcmp(argv[i], "--install-path") == 0) {
			i++;
			Settings::insideLibPath(argv[i]);
			continue;
		} else if (strcmp(argv[i], "-i") == 0 ||
			   strcmp(argv[i], "--ignore") == 0) {
			i++;
			Settings::ignorePrefix(argv[i]);
			continue;
		} else if (strcmp(argv[i], "-d") == 0 ||
			   strcmp(argv[i], "--dest-dir") == 0) {
			i++;
			Settings::destFolder(argv[i]);
			continue;
		} else if (strcmp(argv[i], "-of") == 0 ||
			   strcmp(argv[i], "--overwrite-files") == 0) {
			Settings::canOverwriteFiles(true);
			continue;
		} else if (strcmp(argv[i], "-od") == 0 ||
			   strcmp(argv[i], "--overwrite-dir") == 0) {
			Settings::canOverwriteDir(true);
			Settings::canCreateDir(true);
			continue;
		} else if (strcmp(argv[i], "-cd") == 0 ||
			   strcmp(argv[i], "--create-dir") == 0) {
			Settings::canCreateDir(true);
			continue;
		} else if (strcmp(argv[i], "-h") == 0 ||
			   strcmp(argv[i], "--help") == 0) {
			showHelp();
			exit(0);
		} else if (strcmp(argv[i], "-s") == 0 ||
			   strcmp(argv[i], "--search-path") == 0) {
			i++;
			Settings::addSearchPath(argv[i]);
			continue;
		} else if (strcmp(argv[i], "-q") == 0 ||
			   strcmp(argv[i], "--quiet") == 0) {
			Settings::quietOutput(true);
			continue;
		} else if (strcmp(argv[i], "-v") == 0 ||
			   strcmp(argv[i], "--verbose") == 0) {
			Settings::verboseOutput(true);
			continue;
		} else if (strcmp(argv[i], "-V") == 0 ||
			   strcmp(argv[i], "--version") == 0) {
			std::cout << "dylibbundler " << VERSION << std::endl;
			exit(0);
		} else if (i > 0) {
			// unknown flag, abort
			// ignore first one cause it's usually the path to the executable
			std::cerr << "Unknown flag " << argv[i] << std::endl
				  << std::endl;
			showHelp();
			exit(1);
		}
	}

	if (!Settings::bundleLibs() && Settings::fileToFixAmount() < 1) {
		showHelp();
		exit(0);
	}

	std::cout << "* Collecting dependencies...\n";

	const int amount = Settings::fileToFixAmount();
	for (int n = 0; n < amount; n++)
		collectDependencies(Settings::fileToFix(n));

	collectSubDependencies();
	doneWithDeps_go();

	return 0;
}
