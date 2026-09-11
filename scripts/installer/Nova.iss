#ifndef StageDir
  #error StageDir is required
#endif
#ifndef OutputPath
  #error OutputPath is required
#endif
#ifndef RedistDir
  #error RedistDir is required
#endif
#ifndef AppVersion
  #error AppVersion is required
#endif

[Setup]
AppId={{B5DB1C28-BA67-4DA8-BD54-FC12503536E4}
AppName=OBS Nova
AppVersion={#AppVersion}
AppPublisher=Kryptographer
AppPublisherURL=https://github.com/Kryptographer/obs-studio
AppSupportURL=https://github.com/Kryptographer/obs-studio/issues
DefaultDirName={autopf}\OBS Nova
DefaultGroupName=OBS Nova
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.19044
PrivilegesRequired=admin
OutputDir={#OutputPath}
OutputBaseFilename=OBS-Nova-Setup-{#AppVersion}-x64
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
LicenseFile=..\..\frontend\data\license\gplv2.txt
UninstallDisplayIcon={app}\bin\64bit\obs64.exe
CloseApplications=yes
CloseApplicationsFilter=obs64.exe
RestartApplications=no

[Tasks]
Name: desktopicon; Description: "Create a desktop shortcut"; GroupDescription: "Shortcuts:"; Flags: unchecked

[Files]
Source: "{#StageDir}\*"; DestDir: "{app}"; Excludes: "*.pdb,*.lib,*.exp"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#RedistDir}\vc_redist.x64.exe"; Flags: dontcopy
Source: "{#RedistDir}\vc_redist.x86.exe"; Flags: dontcopy

[Icons]
Name: "{group}\OBS Nova"; Filename: "{app}\bin\64bit\obs64.exe"; WorkingDir: "{app}\bin\64bit"
Name: "{autodesktop}\OBS Nova"; Filename: "{app}\bin\64bit\obs64.exe"; WorkingDir: "{app}\bin\64bit"; Tasks: desktopicon

[Run]
Filename: "{app}\bin\64bit\obs64.exe"; WorkingDir: "{app}\bin\64bit"; Description: "Launch OBS Nova"; Flags: nowait postinstall skipifsilent runasoriginaluser

[Code]
function InstallRuntime(Name: String; var NeedsRestart: Boolean): String;
var
  Code: Integer;
begin
  Result := '';
  ExtractTemporaryFile(Name);
  if not Exec(ExpandConstant('{tmp}\') + Name, '/install /quiet /norestart', '', SW_HIDE, ewWaitUntilTerminated, Code) then
    Result := 'Could not start the Microsoft Visual C++ runtime installer.'
  else if Code = 3010 then
    NeedsRestart := True
  else if (Code <> 0) and (Code <> 1638) then
    Result := 'Microsoft Visual C++ runtime installation failed (code ' + IntToStr(Code) + '). Restart Windows and try again.';
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := InstallRuntime('vc_redist.x64.exe', NeedsRestart);
  if Result = '' then
    Result := InstallRuntime('vc_redist.x86.exe', NeedsRestart);
end;
