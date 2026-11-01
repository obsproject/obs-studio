; MK VISION Studio - Windows installer (Inno Setup 6)
; Built by .github/workflows/mk-studio-windows.yaml
; Defines passed on the ISCC command line:
;   /DAppVersion=1.0.0  /DSourceDir=<repo>\build_x64\install  /DRepoRoot=<repo>  /DOutDir=<dir>
; Licensed GPL-2.0-or-later, same as OBS Studio.

#ifndef AppVersion
  #define AppVersion "0.0.1"
#endif
#ifndef SourceDir
  #error SourceDir must be defined
#endif
#ifndef RepoRoot
  #error RepoRoot must be defined
#endif
#ifndef OutDir
  #define OutDir "."
#endif

#define AppName "MK VISION Studio"
#define AppExe  "obs64.exe"

[Setup]
AppId={{6F1B7E52-3C0A-4D58-9B2E-7A41C5D0E8A3}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=MK VISION
AppPublisherURL=https://github.com/scchoolsystem
DefaultDirName={autopf}\MK VISION Studio
DefaultGroupName=MK VISION Studio
UninstallDisplayIcon={app}\bin\64bit\{#AppExe}
SetupIconFile={#RepoRoot}\frontend\cmake\windows\obs-studio.ico
LicenseFile={#RepoRoot}\frontend\data\license\gplv2.txt
OutputDir={#OutDir}
OutputBaseFilename=MK-VISION-Studio-{#AppVersion}-x64-setup
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
WizardStyle=modern
DisableProgramGroupPage=yes

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
; OBS must start with its working directory set to bin\64bit
Name: "{autoprograms}\MK VISION Studio"; Filename: "{app}\bin\64bit\{#AppExe}"; WorkingDir: "{app}\bin\64bit"
Name: "{autodesktop}\MK VISION Studio"; Filename: "{app}\bin\64bit\{#AppExe}"; WorkingDir: "{app}\bin\64bit"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\64bit\{#AppExe}"; WorkingDir: "{app}\bin\64bit"; Description: "Launch MK VISION Studio"; Flags: nowait postinstall skipifsilent
