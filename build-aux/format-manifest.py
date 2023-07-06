import json
import os
import sys

MAIN_MANIFEST_FILENAME = "com.obsproject.Studio.json"

def main():
    dir_path = os.path.dirname(os.path.realpath(__file__))
    if not os.path.isfile(os.path.join(dir_path, MAIN_MANIFEST_FILENAME)):
        print("The script is not ran in the same folder as the manifest")
        return 1

    for root, dirs, files in os.walk(dir_path):
        for file in files:
            if not file.endswith(".json"):
                continue

            print(f"Formatting {file}")
            # Load JSON file
            with open(os.path.join(root, file), "r") as f:
                j = json.load(f)

            if file == MAIN_MANIFEST_FILENAME:
                # Sort module files order in the manifest
                # Assumption: All modules except the last are strings
                file_modules = j["modules"][0:-1]
                last_module = j["modules"][-1]
                file_modules.sort(key=lambda file_name: file_name)
                j["modules"] = file_modules
                j["modules"].append(last_module)

            # Overwrite JSON file
            with open(os.path.join(root, file), "w") as f:
                json.dump(j, f, indent=4, ensure_ascii=False)
                f.write("\n")

    return 0

if __name__ == '__main__':
    sys.exit(main())
