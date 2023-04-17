# `build-aux` folder

This folder contains:
- The Flatpak manifest used to build OBS Studio
- The script `format-manifest.py` which format manifest JSON files

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
