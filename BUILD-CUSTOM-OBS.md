# Build OBS with Record + Stream and scheduling

Run `build-windows.bat` from a Command Prompt on Windows x64. It initializes
submodules, configures the repository's `windows-x64` preset, builds Release,
and installs the executable with its plugins and runtime files into
`build_x64/install`. The resulting executable is
`build_x64/install/bin/64bit/obs64.exe`. Keep the whole install folder together.
This creates a runnable application folder, not a setup installer.

Install Git and CMake on PATH, plus Visual Studio 2026 with **Desktop development
with C++** and Windows SDK **10.0.26100.0**. CMake must support the
**Visual Studio 18 2026** generator used by this fork's `CMakePresets.json`.
The first build needs internet access and sufficient disk space for OBS and its
dependencies. The batch file stops on the first failed command and returns a
nonzero exit code. `build-windows.bat --check` checks command availability and
that CMake can read the presets; it does not verify the compiler or compile OBS.

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
