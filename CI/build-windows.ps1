Param(
    [Switch]$Help,
    [Switch]$Quiet,
    [Switch]$Verbose,
    [Switch]$Package,
    [Switch]$SkipDependencyChecks,
    [Switch]$BuildInstaller,
    [Switch]$CombinedArchs,
    [String]$BuildDirectory = "build",
    [ValidateSet("32-bit", "64-bit")]
    [String]$BuildArch = (Get-CimInstance CIM_OperatingSystem).OSArchitecture,
    [ValidateSet("Release", "RelWithDebInfo", "MinSizeRel", "Debug")]
    [String]$BuildConfiguration = "RelWithDebInfo"
)

##############################################################################
# Windows OBS build script
##############################################################################
#
# This script contains all steps necessary to:
#
#   * Build OBS with all required dependencies
#   * Create 64-bit and 32-bit variants
#
# Parameters:
#   -Help                   : Print usage help
#   -Quiet                  : Suppress most build process output
#   -Verbose                : Enable more verbose build process output
#   -SkipDependencyChecks   : Skip dependency checks
#   -BuildDirectory         : Directory to use for builds
#                             Default: build64 on 64-bit systems
#                                      build32 on 32-bit systems
#   -BuildArch              : Build architecture to use ("32-bit" or "64-bit")
#   -BuildConfiguration     : Build configuration to use
#                             Default: RelWithDebInfo
#   -CombinedArchs          : Create combined packages and installer
#                             (64-bit and 32-bit) - Default: off
#   -Package                : Prepare folder structure for installer creation
#
# Environment Variables (optional):
#  WindowsDepsVersion       : Pre-compiled Windows dependencies version
#  WindowsQtVersion         : Pre-compiled Qt version
#
##############################################################################

$ErrorActionPreference = "Stop"

$_RunObsBuildScript = $true
$ProductName = "OBS-Studio"

$CheckoutDir = Resolve-Path -Path "$PSScriptRoot\.."
$DepsBuildDir = "${CheckoutDir}/../obs-build-dependencies"
$ObsBuildDir = "${CheckoutDir}/../obs-studio"

. ${CheckoutDir}/CI/include/build_support_windows.ps1

# Handle installation of build system components and build dependencies
. ${CheckoutDir}/CI/windows/01_install_dependencies.ps1

# Handle OBS build configuration
. ${CheckoutDir}/CI/windows/02_build_obs.ps1

# Handle packaging
. ${CheckoutDir}/CI/windows/03_package_obs.ps1

function Build-OBS-Main {
    Ensure-Directory ${CheckoutDir}
    Write-Step "Fetching version tags..."
    $null = git fetch origin --tags
    $GitBranch = git rev-parse --abbrev-ref HEAD
    $GitHash = git rev-parse --short HEAD
    $ErrorActionPreference = "SilentlyContinue"
    $GitTag = git describe --tags --abbrev=0
    $ErrorActionPreference = "Stop"

    if(Test-Path variable:BUILD_FOR_DISTRIBUTION) {
        $VersionString = "${GitTag}"
    } else {
        $VersionString = "${GitTag}-${GitHash}"
    }

    $FileName = "${ProductName}-${VersionString}"

    if($CombinedArchs.isPresent) {
        if (!(Test-Path env:obsInstallerTempDir)) {
            $Env:obsInstallerTempDir = "${CheckoutDir}/install_temp"
        }

        if(!($SkipDependencyChecks.isPresent)) {
            Install-Dependencies -BuildArch 64-bit
        }

        Build-OBS -BuildArch 64-bit

        if(!($SkipDependencyChecks.isPresent)) {
            Install-Dependencies -BuildArch 32-bit
        }

        Build-OBS -BuildArch 32-bit
    } else {
        if(!($SkipDependencyChecks.isPresent)) {
            Install-Dependencies
        }

        Build-OBS
    }

    if($Package.isPresent) {
        Package-OBS -CombinedArchs:$CombinedArchs
    }
}

## MAIN SCRIPT FUNCTIONS ##
function Print-Usage {
    Write-Host "build-windows.ps1 - Build script for ${ProductName}"
    $Lines = @(
        "Usage: ${_ScriptName}",
        "-Help                    : Print this help",
        "-Quiet                   : Suppress most build process output"
        "-Verbose                 : Enable more verbose build process output"
        "-SkipDependencyChecks    : Skip dependency checks - Default: off",
        "-BuildDirectory          : Directory to use for builds - Default: build64 on 64-bit systems, build32 on 32-bit systems",
        "-BuildArch               : Build architecture to use ('32-bit' or '64-bit') - Default: local architecture",
        "-BuildConfiguration      : Build configuration to use - Default: RelWithDebInfo",
        "-CombinedArchs           : Create combined packages and installer (64-bit and 32-bit) - Default: off"
        "-Package                 : Prepare folder structure for installer creation"
    )
    $Lines | Write-Host
}

$_ScriptName = "$($MyInvocation.MyCommand.Name)"
if($Help.isPresent) {
    Print-Usage
    exit 0
}

Build-OBS-Main
