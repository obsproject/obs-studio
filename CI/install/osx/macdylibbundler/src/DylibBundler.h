#ifndef _DYLIB_BUNDLER_H_
#define _DYLIB_BUNDLER_H_

#include <string>

void collectDependencies(std::string filename);
void collectSubDependencies();
void doneWithDeps_go();
bool isRpath(const std::string &path);
std::string searchFilenameInRpaths(const std::string &rpath_dep);

#endif
