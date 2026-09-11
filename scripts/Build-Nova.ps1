[CmdletBinding()]
param([switch] $Check)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$transcribing = $false
$result = 1

function Find-Executable {
    param([string] $Name, [string[]] $Candidates)
    $command = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($command) { return $command.Source }
    foreach ($candidate in $Candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) { return $candidate }
    }
    return $null
}

function Invoke-BuildCommand {
    param([string] $Executable, [string[]] $Arguments)
    Write-Host "`n> $Executable $($Arguments -join ' ')"
    # Native programs can write normal diagnostics to stderr in Windows PowerShell.
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        & $Executable @Arguments
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    if ($code -ne 0) { throw "Command failed with exit code ${code}. See its output above." }
}

Push-Location -LiteralPath $projectRoot
try {
    try {
        Start-Transcript -Path (Join-Path $projectRoot 'build-windows.log') -Force | Out-Null
    } catch {
        throw 'Cannot write build-windows.log. Close any other build running in this folder and make sure the folder is writable.'
    }
    $transcribing = $true
    Write-Host "Project: $projectRoot"
    $missing = @()
    $git = Find-Executable 'git.exe' @("$env:ProgramFiles\Git\cmd\git.exe")
    if (!$git) { $missing += 'Install Git for Windows: https://git-scm.com/download/win' }

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $visualStudio = $null
    if (Test-Path -LiteralPath $vswhere) {
        $visualStudio = & $vswhere -latest -products '*' -version '[18.0,19.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    }
    if (!$visualStudio) {
        $missing += 'Open Visual Studio Installer. Install/Modify Visual Studio 2026 or Build Tools 2026 and select Desktop development with C++ (MSVC x64/x86 tools).'
    } else {
        $toolsVersionFile = Join-Path $visualStudio 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
        $toolsVersion = (Get-Content -LiteralPath $toolsVersionFile -Raw).Trim()
        $atlHeader = Join-Path $visualStudio "VC\Tools\MSVC\$toolsVersion\atlmfc\include\atlcomcli.h"
        if (!(Test-Path -LiteralPath $atlHeader)) {
            $missing += 'In Visual Studio Installer > Modify > Individual components, install C++ ATL for latest build tools (x86 and x64). OBS DirectShow capture requires atlcomcli.h and atlstr.h.'
        }
    }

    $cmakeCandidates = @("$env:ProgramFiles\CMake\bin\cmake.exe")
    if ($visualStudio) {
        $cmakeCandidates += "$visualStudio\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    }
    $cmake = Find-Executable 'cmake.exe' $cmakeCandidates
    # Prefer a compatible installation even if an older CMake is first on PATH.
    $compatibleCmake = $null
    foreach ($candidate in (@($cmake) + $cmakeCandidates | Select-Object -Unique)) {
        if (!$candidate -or !(Test-Path -LiteralPath $candidate -PathType Leaf)) { continue }
        $help = & $candidate --help
        if ($LASTEXITCODE -eq 0 -and ($help -match 'Visual Studio 18 2026')) {
            $compatibleCmake = $candidate
            break
        }
    }
    $cmake = $compatibleCmake
    if (!$cmake) { $missing += 'Install a current CMake that supports Visual Studio 18 2026: https://cmake.org/download/ (or install the Visual Studio C++ CMake tools component).' }

    $kitsRoot = "${env:ProgramFiles(x86)}\Windows Kits\10"
    $kits = Get-ItemProperty 'HKLM:\SOFTWARE\Microsoft\Windows Kits\Installed Roots' -ErrorAction SilentlyContinue
    if ($kits -and $kits.KitsRoot10) { $kitsRoot = $kits.KitsRoot10 }
    if (!(Test-Path -LiteralPath (Join-Path $kitsRoot 'Include\10.0.26100.0\um\Windows.h'))) {
        $missing += 'In Visual Studio Installer > Individual components, install Windows SDK 10.0.26100.0 (required by this fork).'
    }
    if (!(Test-Path -LiteralPath (Join-Path $projectRoot '.git'))) {
        $missing += 'Use a Git clone, not GitHub Download ZIP. Run: git clone --recursive https://github.com/Kryptographer/obs-studio.git'
    }
    if ($missing.Count) {
        throw ("Build prerequisites are missing:`n`n - " + ($missing -join "`n`n - "))
    }

    # CMake's dependency setup also invokes Git by name.
    $env:PATH = "$(Split-Path -Parent $git);$(Split-Path -Parent $cmake);$env:PATH"
    Write-Host "Visual Studio: $visualStudio"
    Invoke-BuildCommand $cmake @('--version')
    Invoke-BuildCommand $cmake @('--list-presets')
    if ($Check) {
        Write-Host 'Prerequisite checks passed. Run build-windows.bat without --check to build.'
    } else {
        $shallow = & $git rev-parse --is-shallow-repository
        if ($LASTEXITCODE -ne 0) { throw 'Cannot read Git repository metadata.' }
        if ($shallow -eq 'true') {
            Invoke-BuildCommand $git @('fetch', '--unshallow', '--tags', 'origin')
        } else {
            Invoke-BuildCommand $git @('fetch', '--tags', 'origin')
        }
        $version = & $git describe --always --tags
        if ($LASTEXITCODE -eq 0 -and $version -notmatch '^\d+\.\d+\.\d+') {
            # GitHub forks do not necessarily include their upstream release tags.
            Invoke-BuildCommand $git @('fetch', '--tags', 'https://github.com/obsproject/obs-studio.git')
            $version = & $git describe --always --tags
        }
        if ($LASTEXITCODE -ne 0 -or $version -notmatch '^\d+\.\d+\.\d+') {
            throw 'No OBS release version tag is reachable. Fetch the upstream OBS release tags before building.'
        }
        Invoke-BuildCommand $git @('submodule', 'update', '--init', '--recursive')
        Invoke-BuildCommand $cmake @('--preset', 'windows-x64', "-DCMAKE_GENERATOR_INSTANCE=$visualStudio")
        Invoke-BuildCommand $cmake @('--build', '--preset', 'windows-x64', '--config', 'Release', '--parallel')
        $installPath = Join-Path $projectRoot 'build_x64\install'
        Invoke-BuildCommand $cmake @('--install', 'build_x64', '--config', 'Release', '--prefix', $installPath)
        $executable = Join-Path $installPath 'bin\64bit\obs64.exe'
        if (!(Test-Path -LiteralPath $executable)) { throw "Expected executable was not found: $executable" }
        Write-Host "`nReady: $executable"
        Write-Host 'Keep the entire build_x64\install folder together; OBS needs its DLLs and data.'
    }
    $result = 0
} catch {
    Write-Host "`nERROR: $($_.Exception.Message)" -ForegroundColor Red
} finally {
    if ($transcribing) { Stop-Transcript | Out-Null }
    Pop-Location
}
exit $result
