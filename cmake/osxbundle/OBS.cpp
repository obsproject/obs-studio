#include <iostream>
#include <vector>
#include <unistd.h>
using namespace std;

int main(int argc, char** argv) {

    int i;
    string start_cmd = "./obs";

    for (i = 1; i < argc; i++) {
        start_cmd += " ";
        start_cmd += argv[i];
    }

    string work_dir = "";

    if (argc > 0) {
        int first_len = strlen(argv[0]);

        vector<string> list(first_len, "");
        int ptr = 0;

        for (i = 0; i < first_len; i++) {
            char cur = argv[0][i];
            if (cur == '/') {
                ptr++;
            } else {
                list[ptr] += cur;
            }
        }
        for (i = 0; i < ptr; i++) {
            work_dir += list[i] + "/";
        }
    }

    work_dir += "../Resources/bin";

    cout << "Setting up working dir: " << work_dir << endl;
    if (chdir(work_dir.c_str())) {
        cout << "Error with setting up working directory! OBS will closed!" << endl;
        return 1;
    }

    cout << "Exec main OBS runnable: " << start_cmd << endl << endl;
    system(start_cmd.c_str());

    return 0;
}