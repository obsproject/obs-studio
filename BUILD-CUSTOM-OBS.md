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

### Preview size and screen sizes

The preview keeps its share of the window on any screen. OBS stores dock sizes
in pixels, so a layout saved on a large monitor used to squeeze the preview when
the window opened on a smaller screen. Nova now remembers how much of the window
the preview occupied, re-applies that share after the saved layout is restored,
and trims the docks whenever a resize or monitor change would leave the preview
under about a third of the window height or 40% of its width.

To adjust the preview yourself, drag the handles between the preview and the
docks (they highlight in green), or use **View > Preview Size**: *Larger* and
*Smaller* step the preview, *Compact*, *Balanced* and *Large* are presets, and
*Studio Header* hides the header to give the preview the full height. Docks can
still be dragged, floated or closed from the **Docks** menu. The chosen size is
saved with the window layout.

## Windows build

Run `build-windows.bat` from a Command Prompt on Windows x64. It initializes
submodules, configures the repository's `windows-x64` preset, builds Release,
and installs the executable with its plugins and runtime files into
`build_x64/install`. It then creates a **setup installer EXE** in the `dist`
folder beside the batch file: **OBS-Nova-Setup-&lt;version&gt;-x64.exe**. Give that
single setup EXE to other people; they do not need the repository, Git, Visual
Studio, CMake, ATL or Inno Setup. A SHA-256 checksum file is written alongside it.

The installer targets Windows 10 (build 19044+) and Windows 11 on x64-compatible
PCs with hardware/drivers supported by OBS. It does not run on macOS, Linux or
32-bit Windows. It installs OBS Nova under Program Files, creates a Start menu
shortcut and optional desktop shortcut, and supports removal through Windows
Installed Apps. It embeds the Microsoft x64 and x86 C++ runtimes, plugins,
Qt, browser files and application data, so the destination PC does not need
internet to obtain those dependencies. Installation requires administrator
approval. OBS hardware and streaming-service requirements still apply.

The build PC also needs Inno Setup 6.7 or later. If it is missing, the launcher
offers to install the pinned, checksum- and signature-verified official 6.7.3
compiler. This compiler is not installed on destination PCs. Inno Setup's
[licensing terms](https://jrsoftware.org/isdl.php) apply to its use.
The generated setup EXE is unsigned unless you arrange code signing separately.

**GitHub Download ZIP is supported.** Extract the complete ZIP and double-click
`build-windows.bat`. The launcher creates a full checkout of this fork's latest
`master` under `build-source`, including submodules. ZIP builds place the setup
EXE in the original extracted folder's `dist` directory. The raw application
also remains in `build-source/build_x64/install/bin/64bit/obs64.exe`. Your extracted files are
left intact; edits made only to those extracted files are not compiled. Repeat
runs update the managed checkout with a fast-forward pull. If it has local
changes, a different remote or a different branch, the launcher stops without
overwriting them. Regular Git checkouts continue to build in place.

Install Git and CMake, plus Visual Studio 2026 with **Desktop development
with C++** and Windows SDK **10.0.26100.0**. CMake must support the
**Visual Studio 18 2026** generator used by this fork's `CMakePresets.json`.
If **C++ ATL** is missing, the launcher offers to install that component using
Visual Studio Installer. Enter **Y** and accept the Windows administrator prompt.
The launcher checks the installer result and the headers before continuing. If
a restart is required, restart Windows and rerun the batch file. You can also
install **C++ ATL for latest build tools (x86 and x64)** manually under Individual
components. Installer integration follows Microsoft's documented
[command-line interface](https://learn.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio?view=visualstudio).
The first build needs internet access and sufficient disk space for OBS and its
dependencies. The launcher also finds Git/CMake in their standard installation
folders and CMake bundled with Visual Studio Build Tools, even when not on PATH.
It fetches history and official OBS release tags when needed for version detection.
For ZIP downloads, the managed checkout supplies the missing submodules.

Double-click `build-windows.bat`. The window now stays open on success or failure,
and diagnostics are saved in `build-windows.log` beside the batch file. Share the
last error from that log if a build fails. `build-windows.bat --check` validates
the installed compiler component, SDK, CMake generator and preset without building.
Use `--no-pause` for automation; this also disables interactive installation
prompts. `--check` never installs components or clones source. Failures return a
nonzero exit code. Keep both included helpers, `scripts/Build-Nova.ps1` and
`scripts/Build-Nova.Support.ps1`, with the batch file.
The packaging files `scripts/Package-Nova.ps1` and `scripts/installer/Nova.iss`
are also required and included in the repository.

Launcher regression checks run with Windows PowerShell using
`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/tests/Build-Nova.Tests.ps1`.
They cover archive checkout creation/reuse, protection of existing work, ATL
detection, installer arguments, restart and failure results using simulated Git
and installer calls. They do not install software or build OBS.

The Controls dock includes **Record + Stream** and **Schedule Record + Stream**.
The scheduler has a month calendar with highlighted dates and a daily agenda.
Create named one-time, daily, or weekly schedules with any combination of weekday
checkboxes. Set a starting date, a 24-hour start time and an optional inclusive
end date. Select a schedule to edit it, uncheck Schedule enabled to pause it,
then click Save schedule. Changes are saved immediately after Save; closing or
switching schedules prompts before discarding edits. Delete removes the whole
recurrence. Existing future one-time schedules are migrated automatically.

Times follow the computer's local timezone. OBS must remain open and the computer
awake. Starts use the current profile and scene. Configure streaming and recording
beforehand; broadcast services require a prepared broadcast with auto-start enabled.
Starts missed by 60 seconds or more are skipped. A spring-forward time that does
not exist is skipped; the next valid recurrence still runs. A repeated fall-back
time runs once. Simultaneous schedules trigger one combined output start. Use
the individual output buttons to stop; scheduled stops are not included.

The EXE, installer and Windows shortcuts use the Nova icon from the obsolete
Kryptographer/obs repository. Only its icon asset was reused; see
`frontend/cmake/windows/NOVA-ICON.md` for the source commit and embedded sizes.

The combined action requests recording once the stream start is accepted. It
does not guarantee both outputs succeed: existing OBS output errors still apply.
If streaming is active, the button starts recording without stopping streaming.

Validation: source checks and launcher regression tests passed. The installer
definition compiled with Inno Setup 6.7.3 using an isolated test payload. That
fixture is not a distributable OBS build. A full OBS build, clean-PC installation,
upgrade/uninstall and live output tests remain outstanding.

Scheduler validation: the standalone C++/Qt model tests and native dialog smoke
test pass. They cover recurrence boundaries, end dates, disabled entries, persistence,
missed starts, clock rollback, DST, and creating a weekday schedule through the UI.
The dialog was also rendered with Windows fonts for visual inspection.
