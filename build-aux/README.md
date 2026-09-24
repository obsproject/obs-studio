# `build-aux` folder

This folder contains:
- Various formatting scripts:
  - `run-clang-format` which formats C/C++/ObjC/ObjC++ files
  - `run-gersemi` which formats CMake files
  - `run-swift-format` which formats Swift files
  - `format-manifest.py` which formats Flatpak manifest JSON files
- The Flatpak manifest used to build OBS Studio
- Files used for Steam packaging


## Formatting scripts

### `run-clang-format`

This script allows to check the formatting and/or format of C/C++/ObjC/ObjC++ files and requires ZSH and a specific version of `clang-format`.

If the script does not find the latter it will return the required version, we provide `clang-format` Homebrew formulas in our [homebrew-tools repo](https://github.com/obsproject/homebrew-tools/).

Example of use:
```sh
./build-aux/run-clang-format
```

### `run-gersemi`

This script allows to check the formatting and/or format of the CMake files and requires ZSH and `gersemi` Python package.

Example of use:
```sh
./build-aux/run-gersemi
```

### `run-swift-format`

This script allows to check the formatting and/or format of the Swift files and requires ZSH and `swift-format`.

Example of use:
```sh
./build-aux/run-swift-format
```

### `format-manifest.py`

This script allows to check the formatting and/or format of the Flatpak manifest and its modules.

Example of use:
```sh
python3 ./build-aux/format-manifest.py com.obsproject.Studio.json
```
