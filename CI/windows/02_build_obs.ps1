Param(
    [Switch]$Help = $(if (Test-Path variable:Help) { $Help }),
    [Switch]$Quiet = $(if (Test-Path variable:Quiet) { $Quiet }),
    [Switch]$Verbose = $(if (Test-Path variable:Verbose) { $Verbose }),
    [String]$BuildDirectory = $(if (Test-Path variable:BuildDirectory) { "${BuildDirectory}" } else { "build" }),
    [ValidateSet("32-bit", "64-bit")]
    [String]$BuildArch = $(if (Test-Path variable:BuildArch) { "${BuildArch}" } else { (Get-CimInstance CIM_OperatingSystem).OSArchitecture }),
    [ValidateSet("Release", "RelWithDebInfo", "MinSizeRel", "Debug")]
    [String]$BuildConfiguration = $(if (Test-Path variable:BuildConfiguration) { "${BuildConfiguration}" } else { "RelWithDebInfo" })
)

##############################################################################
# Windows libobs library build function
##############################################################################
#
# This script file can be included in build scripts for Windows or run
# directly
#
##############################################################################

$ErrorActionPreference = "Stop"

function Build-OBS {
    Param(
        [String]$BuildDirectory = $(if (Test-Path variable:BuildDirectory) { "${BuildDirectory}" }),
        [String]$BuildArch = $(if (Test-Path variable:BuildArch) { "${BuildArch}" }),
        [String]$BuildConfiguration = $(if (Test-Path variable:BuildConfiguration) { "${BuildConfiguration}" })
    )

    Write-Status "Build OBS"

    Configure-OBS

    Ensure-Directory ${CheckoutDir}
    Write-Step "Build OBS targets..."

    Invoke-Expression "cmake --build $(if($BuildArch -eq "64-bit") { "build64" } else { "build32" }) --config ${BuildConfiguration}"
}

function Configure-OBS {
    Ensure-Directory ${CheckoutDir}
    Write-Status "Configuration of OBS build system..."

    # TODO: Clean up archive and directory naming across dependencies
    $QtDirectory = "${CheckoutDir}/../obs-build-dependencies/Qt_${WindowsQtVersion}/msvc2019$(if (${BuildArch} -eq "64-bit") { "_64" })"
    $DepsDirectory = "${CheckoutDir}/../obs-build-dependencies/dependencies${WindowsDepsVersion}/win$(if (${BuildArch} -eq "64-bit") { "64" } else { "32" })"
    $CefDirectory = "${CheckoutDir}/../obs-build-dependencies/cef_binary_${WindowsCefVersion}_windows_$(if (${BuildArch} -eq "64-bit") { "x64" } else { "x86" })"
    $CmakePrefixPath = "${QtDirectory};${DepsDirectory}/bin;${DepsDirectory}"
    $BuildDirectoryActual = "${BuildDirectory}$(if (${BuildArch} -eq "64-bit") { "64" } else { "32" })"
    $GeneratorPlatform = "$(if (${BuildArch} -eq "64-bit") { "x64" } else { "Win32" })"

    $CmakeCommand = @(
        "-S . -B `"${BuildDirectoryActual}`"",
        "-G `"${CmakeGenerator}`"",
        "-DCMAKE_GENERATOR_PLATFORM=`"${GeneratorPlatform}`"",
        "-DCMAKE_SYSTEM_VERSION=`"${CmakeSystemVersion}`"",
        "-DCMAKE_PREFIX_PATH=`"${CmakePrefixPath}`"",
        "-DCEF_ROOT_DIR=`"${CefDirectory}`"",
        "-DVLC_PATH=`"${CheckoutDir}/../obs-build-dependencies/vlc-${WindowsVlcVersion}`"",
        "-DCMAKE_INSTALL_PREFIX=`"${BuildDirectoryActual}/install`"",
        "-DVIRTUALCAM_GUID=`"${Env:VIRTUALCAM-GUID}`"",
        "-DTWITCH_CLIENTID=`"${Env:TWITCH_CLIENTID}`"",
        "-DTWITCH_HASH=`"${Env:TWITCH_HASH}`"",
        "-DRESTREAM_CLIENTID=`"${Env:RESTREAM_CLIENTID}`"",
        "-DRESTREAM_HASH=`"${Env:RESTREAM_HASH}`"",
        "-DYOUTUBE_CLIENTID=`"${Env:YOUTUBE_CLIENTID}`"",
        "-DYOUTUBE_CLIENTID_HASH=`"${Env:YOUTUBE_CLIENTID_HASH}`"",
        "-DYOUTUBE_SECRET=`"${Env:YOUTUBE_SECRET}`"",
        "-DYOUTUBE_SECRET_HASH=`"${Env:YOUTUBE_SECRET_HASH}`"",
        "-DCOPIED_DEPENDENCIES=OFF",
        "-DCOPY_DEPENDENCIES=ON",
        "-DBUILD_FOR_DISTRIBUTION=`"$(if (Test-Path Env:BUILD_FOR_DISTRIBUTION) { "ON" } else { "OFF" })`"",
        "$(if (Test-Path Env:CI) { "-DOBS_BUILD_NUMBER=${Env:GITHUB_RUN_ID}" })",
        "$(if (Test-Path Variable:$Quiet) { "-Wno-deprecated -Wno-dev --log-level=ERROR" })"
    )

    Invoke-Expression "cmake ${CmakeCommand}"

    Ensure-Directory ${CheckoutDir}
}

function Build-OBS-Standalone {
    $ProductName = "OBS-Studio"

    $CheckoutDir = Resolve-Path -Path "$PSScriptRoot\..\.."
    $DepsBuildDir = "${CheckoutDir}/../obs-build-dependencies"
    $ObsBuildDir = "${CheckoutDir}/../obs-studio"

    . ${CheckoutDir}/CI/include/build_support_windows.ps1

    Build-OBS
}

function Print-Usage {
    $Lines = @(
        "Usage: ${_ScriptName}",
        "-Help                    : Print this help",
        "-Quiet                   : Suppress most build process output",
        "-Verbose                 : Enable more verbose build process output",
        "-BuildDirectory          : Directory to use for builds - Default: build64 on 64-bit systems, build32 on 32-bit systems",
        "-BuildArch               : Build architecture to use (32-bit or 64-bit) - Default: local architecture",
        "-BuildConfiguration      : Build configuration to use - Default: RelWithDebInfo"
    )

    $Lines | Write-Host
}

if(!(Test-Path variable:_RunObsBuildScript)) {
    $_ScriptName = "$($MyInvocation.MyCommand.Name)"
    if($Help.isPresent) {
        Print-Usage
        exit 0
    }

    Build-OBS-Standalone
}
