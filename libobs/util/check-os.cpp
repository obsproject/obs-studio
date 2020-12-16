#include "check-os.h"
#include <fstream>
#include <iostream>
#include <string>

using namespace std;

int is_BigSur_OS() {
    #ifndef __APPLE__
        return 0;
    #endif
    static int status_resolved = -1;
	if (status_resolved != -1) {
		return status_resolved;
	}
    fstream file;
    file.open("/System/Library/CoreServices/SystemVersion.plist", ios::in);
    if (file.fail()) {
        status_resolved = 0;
        return status_resolved;
    }
    string file_str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    if (file_str.find("<key>ProductUserVisibleVersion</key>\n\t<string>11") != string::npos) {
        status_resolved = 1;
    } else {
        status_resolved = 0;
    }
    file.close();
    return status_resolved;
}