[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string] $InstallPath,
    [Parameter(Mandatory = $true)][string] $OutputPath,
    [Parameter(Mandatory = $true)][string] $CompilerPath,
    [Parameter(Mandatory = $true)][string] $VisualStudio
)
$ErrorActionPreference = 'Stop'
try {
    $InstallPath = (Resolve-Path -LiteralPath $InstallPath).Path
    # Refuse incomplete staging trees instead of producing a broken installer.
    foreach ($required in @('bin\64bit\obs64.exe', 'bin\64bit\obs.dll', 'bin\64bit\Qt6Core.dll',
            'obs-plugins\64bit', 'data\obs-studio\themes\Nova.ovt', 'bin\64bit\platforms\qwindows.dll')) {
        if (!(Test-Path -LiteralPath (Join-Path $InstallPath $required))) {
            throw "Incomplete OBS build: missing $required. Complete the build and install steps first."
        }
    }
    New-Item -ItemType Directory -Path $OutputPath -Force | Out-Null
    $OutputPath = (Resolve-Path -LiteralPath $OutputPath).Path
    $redistPath = Join-Path (Split-Path -Parent $InstallPath) 'installer-redist'
    New-Item -ItemType Directory -Path $redistPath -Force | Out-Null
    foreach ($arch in @('x64', 'x86')) {
        $name = "vc_redist.$arch.exe"
        $candidates = Get-ChildItem -Path (Join-Path $VisualStudio "VC\Redist\MSVC\*\$name") -File |
            Sort-Object { $_.VersionInfo.FileVersionRaw } -Descending
        $redist = $candidates | Select-Object -First 1
        if (!$redist) { throw "Missing $name in Visual Studio. Install its C++ redistributable component." }
        $signature = Get-AuthenticodeSignature -LiteralPath $redist.FullName
        if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Microsoft Corporation') {
            throw "Microsoft signature verification failed for $name. Repair Visual Studio before packaging."
        }
        Copy-Item -LiteralPath $redist.FullName -Destination (Join-Path $redistPath $name) -Force
    }
    $version = (Get-Item -LiteralPath (Join-Path $InstallPath 'bin\64bit\obs64.exe')).VersionInfo
    $appVersion = '{0}.{1}.{2}.{3}' -f $version.FileMajorPart, $version.FileMinorPart, $version.FileBuildPart, $version.FilePrivatePart
    $arguments = @("/DStageDir=$InstallPath", "/DOutputPath=$OutputPath", "/DRedistDir=$redistPath",
        "/DAppVersion=$appVersion", (Join-Path $PSScriptRoot 'installer\Nova.iss'))
    & $CompilerPath @arguments
    if ($LASTEXITCODE -ne 0) { throw "Installer compilation failed (exit code $LASTEXITCODE)." }
    $setup = Join-Path $OutputPath "OBS-Nova-Setup-$appVersion-x64.exe"
    if (!(Test-Path -LiteralPath $setup)) { throw 'Installer compiler did not produce the expected setup EXE.' }
    $hash = (Get-FileHash -LiteralPath $setup -Algorithm SHA256).Hash
    Set-Content -LiteralPath ($setup + '.sha256') -Value "$hash  $([IO.Path]::GetFileName($setup))" -Encoding Ascii
    Write-Host "`nINSTALLER READY: $setup"
    Write-Host 'Copy this setup EXE to a supported Windows x64 PC. No build tools or source files are needed there.'
} catch {
    Write-Error $_
    exit 1
}
