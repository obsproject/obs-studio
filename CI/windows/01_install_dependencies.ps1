Param(
    [Switch]$Help = $(if (Test-Path variable:Help) { $Help }),
    [Switch]$Quiet = $(if (Test-Path variable:Quiet) { $Quiet }),
    [Switch]$Verbose = $(if (Test-Path variable:Verbose) { $Verbose }),
    [ValidateSet("32-bit", "64-bit")]
    [String]$BuildArch = $(if (Test-Path variable:BuildArch) { "${BuildArch}" } else { (Get-CimInstance CIM_OperatingSystem).OSArchitecture })
)

##############################################################################
# Windows dependency management function
##############################################################################
#
# This script file can be included in build scripts for Windows or run
# directly
#
##############################################################################

$ErrorActionPreference = "Stop"

Function Install-obs-deps {
    Param(
        [Parameter(Mandatory=$true)]
        [String]$Version
    )

    Write-Status "Setup for pre-built Windows OBS dependencies v${Version}"
    Ensure-Directory $DepsBuildDir

    if (!(Test-Path "$DepsBuildDir/dependencies${Version}")) {

        Write-Step "Download..."
        $ProgressPreference = $(if ($Quiet.isPresent) { "SilentlyContinue" } else { "Continue" })
        Invoke-WebRequest -Uri "https://cdn-fastly.obsproject.com/downloads/dependencies${Version}.zip" -UseBasicParsing -OutFile "dependencies${Version}.zip"
        $ProgressPreference = "Continue"

        Write-Step "Unpack..."

        Expand-Archive -Path "dependencies${Version}.zip"
    } else {
        Write-Step "Found existing pre-built dependencies..."
    }
}

function Install-qt-deps {
    Param(
        [Parameter(Mandatory=$true)]
        [String]$Version
    )

    Write-Status "Setup for pre-built dependency Qt v${Version}"
    Ensure-Directory $DepsBuildDir

    if (!(Test-Path "$DepsBuildDir/Qt_${Version}")) {

        Write-Step "Download..."
        $ProgressPreference = $(if ($Quiet.isPresent) { 'SilentlyContinue' } else { 'Continue' })
        Invoke-WebRequest -Uri "https://cdn-fastly.obsproject.com/downloads/Qt_${Version}.7z" -UseBasicParsing -OutFile "Qt_${Version}.7z"
        $ProgressPreference = "Continue"
        
        Write-Step "Unpack..."

        # TODO: Replace with zip and properly package Qt to share directory with other deps
        Invoke-Expression "7z x Qt_${Version}.7z"
        Move-Item -Path "${Version}" -Destination "Qt_${Version}"
    } else {
        Write-Step "Found existing pre-built Qt..."
    }
}

function Install-vlc {
    Param(
        [Parameter(Mandatory=$true)]
        [String]$Version
    )

    Write-Status "Setup for dependency VLC v${Version}"
    Ensure-Directory $DepsBuildDir

    if (!((Test-Path "$DepsBuildDir/vlc-${Version}") -and (Test-Path "$DepsBuildDir/vlc-${Version}/include/vlc/vlc.h"))) {
        Write-Step "Download..."
        $ProgressPreference = $(if ($Quiet.isPresent) { 'SilentlyContinue' } else { 'Continue' })
        Invoke-WebRequest -Uri "https://cdn-fastly.obsproject.com/downloads/vlc.zip" -UseBasicParsing -OutFile "vlc_${Version}.zip"
        $ProgressPreference = "Continue"

        Write-Step "Unpack..."
        # Expand-Archive -Path "vlc_${Version}.zip"
        Invoke-Expression "7z x vlc_${Version}.zip -ovlc"
        Move-Item -Path vlc -Destination "vlc-${Version}"
    } else {
        Write-Step "Found existing VLC..."
    }
}

function Install-cef {
    Param(
        [Parameter(Mandatory=$true)]
        [String]$Version
    )
    Write-Status "Setup for dependency CEF v${Version} - ${BuildArch}"

    Ensure-Directory $DepsBuildDir
    $ArchSuffix = "$(if ($BuildArch -eq "64-bit") { "x64" } else { "x86" })"

    if (!((Test-Path "${DepsBuildDir}/cef_binary_${Version}_windows_${ArchSuffix}") -and (Test-Path "${DepsBuildDir}/cef_binary_${Version}_windows_${ArchSuffix}/build/libcef_dll_wrapper/Release/libcef_dll_wrapper.lib"))) {
        Write-Step "Download..."
        $ProgressPreference = $(if ($Quiet.isPresent) { 'SilentlyContinue' } else { 'Continue' })
        Invoke-WebRequest -Uri "https://cdn-fastly.obsproject.com/downloads/cef_binary_${Version}_windows_${ArchSuffix}.zip" -UseBasicParsing -OutFile "cef_binary_${Version}_windows_${ArchSuffix}.zip"
        $ProgressPreference = "Continue"

        Write-Step "Unpack..."
        Invoke-Expression "7z x cef_binary_${Version}_windows_${ArchSuffix}.zip"
    } else {
        Write-Step "Found existing CEF framework and loader library..."
    }
}

function Install-Dependencies {
    Param(
        [String]$BuildArch = $(if (Test-Path variable:BuildArch) { "${BuildArch}" })
    )

    if($Choco.isPresent) {
        Install-Windows-Dependencies
    }

    $BuildDependencies = @(
        @('obs-deps', $WindowsDepsVersion),
        @('qt-deps', $WindowsQtVersion),
        @('vlc', $WindowsVlcVersion),
        @('cef', $WindowsCefVersion)
    )

    Foreach($Dependency in ${BuildDependencies}) {
        $DependencyName = $Dependency[0]
        $DependencyVersion = $Dependency[1]

        $FunctionName = "Install-${DependencyName}"
        Invoke-Expression "${FunctionName} -Version ${DependencyVersion}"
    }

    Ensure-Directory ${CheckoutDir}
}

function Install-Dependencies-Standalone {
    $ProductName = "OBS-Studio"
    $CheckoutDir = Resolve-Path -Path "$PSScriptRoot\..\.."
    $DepsBuildDir = "${CheckoutDir}/../obs-build-dependencies"
    $ObsBuildDir = "${CheckoutDir}/../obs-studio"

    . ${CheckoutDir}/CI/include/build_support_windows.ps1

    Write-Status "Setup of OBS build dependencies"
    Install-Dependencies
}

function Print-Usage {
    $Lines = @(
        "Usage: ${_ScriptName}",
        "-Help                    : Print this help",
        "-Quiet                   : Suppress most build process output",
        "-Verbose                 : Enable more verbose build process output",
        "-Choco                   : Enable automatic dependency installation via Chocolatey - Default: off"
        "-BuildArch               : Build architecture to use (32-bit or 64-bit) - Default: local architecture"
    )

    $Lines | Write-Host
}

if(!(Test-Path variable:_RunObsBuildScript)) {
    $_ScriptName = "$($MyInvocation.MyCommand.Name)"
    if($Help.isPresent) {
        Print-Usage
        exit 0
    }

    Install-Dependencies-Standalone
}
