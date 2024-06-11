# `build-aux` folder

This folder contains:
- Various formatting scripts:
  - `run-clang-format` which formats C/C++/ObjC/ObjC++ files
  - `run-cmake-format` which formats CMake files
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

### `run-gersemi-format`

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

## OBS Studio Flatpak Manifest

The manifest is composed of multiple files:
 - The main manifest `com.obsproject.Studio.json`
 - The `modules` folder which contains OBS Studio dependencies modules

### Manifest modules

Modules are ordered/dispatched in numbered categories following a short list of rules:
- A module must not depend on another module from the same category, so a module can only depend on modules from lower numbered categories.
- A module without dependencies must be placed in the highest numbered category in use, excluding categories meant for specific types of dependency.

Actual categories:
 - `99-`: CEF
 - `90-`: Headers-only libraries that are dependencies of only OBS Studio
 - `50-`: Modules that are dependencies of only OBS Studio
 - `40-`: Modules that are dependencies of the `50-` category
 - `30-`: FFmpeg
 - `20-`: Modules that are dependencies of FFmpeg
 - `10-`: Modules that are dependencies of the `20-` category
