# Build OBS with Record + Stream and scheduling

## Nova interface

The Nova style follows the supplied dark green and lime studio mockup. It adds
a native studio header, a Broadcast dock on the right with an elapsed session
timer, and Studio Tools shortcuts beside the bottom Mixer and Transitions docks.
Scenes and Sources stay on the left. Preview content remains your actual scene.
The timer measures wall-clock time while either output is active, including
recording pauses; it is not the duration of the recorded file. It resets when
a new session starts after both outputs have stopped.

On an existing installation, choose **View > Apply Nova studio layout** to apply
the theme and reset dock positions. This also resets custom dock positions, like
OBS's Reset Docks command. Fresh configurations use Nova by default. To change
only colors, choose **Settings > Appearance > Yami > Nova**. Existing themes
remain available. Native window decorations and OBS preview rendering are retained.

## Windows build

Run `build-windows.bat` from a Command Prompt on Windows x64. It initializes
submodules, configures the repository's `windows-x64` preset, builds Release,
and installs the executable with its plugins and runtime files into
`build_x64/install`. The resulting executable is
`build_x64/install/bin/64bit/obs64.exe`. Keep the whole install folder together.
This creates a runnable application folder, not a setup installer.

Install Git and CMake, plus Visual Studio 2026 with **Desktop development
with C++** and Windows SDK **10.0.26100.0**. CMake must support the
**Visual Studio 18 2026** generator used by this fork's `CMakePresets.json`.
Also select **C++ ATL for latest build tools (x86 and x64)** under the installer's
Individual components tab. Without ATL, DirectShow capture fails on missing
`atlcomcli.h` / `atlstr.h` headers. The launcher checks for this before building.
The first build needs internet access and sufficient disk space for OBS and its
dependencies. The launcher also finds Git/CMake in their standard installation
folders and CMake bundled with Visual Studio Build Tools, even when not on PATH.
It fetches history and official OBS release tags when needed for version detection.
Use a Git clone of the whole repository; GitHub's Download ZIP omits submodules.

Double-click `build-windows.bat`. The window now stays open on success or failure,
and diagnostics are saved in `build-windows.log` beside the batch file. Share the
last error from that log if a build fails. `build-windows.bat --check` validates
the installed compiler component, SDK, CMake generator and preset without building.
Use `--no-pause` for automation; failures still return a nonzero exit code.
The batch file requires the included `scripts/Build-Nova.ps1` helper.

The Controls dock includes **Record + Stream** and **Schedule Record + Stream**.
The scheduler saves multiple one-time starts in local time. OBS must remain open
and the computer awake. Starts use the current profile and scene. Configure
streaming and recording beforehand; broadcast services require a prepared
broadcast with auto-start enabled. Starts missed by 60 seconds or more are
skipped. Use the individual output buttons to stop. Recurrence and scheduled
stops are not included.

The combined action requests recording once the stream start is accepted. It
does not guarantee both outputs succeed: existing OBS output errors still apply.
If streaming is active, the button starts recording without stopping streaming.

Validation: source whitespace, UI XML, localization references, and patch
application were checked. A full OBS build and live output tests have not yet
been performed in the development environment.
