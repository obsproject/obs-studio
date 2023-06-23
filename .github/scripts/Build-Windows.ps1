[CmdletBinding()]
param(
    [ValidateSet('x64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
    [switch] $SkipAll,
    [switch] $SkipBuild,
    [switch] $SkipDeps
)

$ErrorActionPreference = 'Stop'

if ( $DebugPreference -eq 'Continue' ) {
    $VerbosePreference = 'Continue'
    $InformationPreference = 'Continue'
}

if ( ! ( [System.Environment]::Is64BitOperatingSystem ) ) {
    throw "obs-studio requires a 64-bit system to build and run."
}

if ( $PSVersionTable.PSVersion -lt '7.2.0' ) {
    Write-Warning 'The obs-studio PowerShell build script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Build {
    trap {
        Pop-Location -Stack BuildTemp -ErrorAction 'SilentlyContinue'
        Write-Error $_
        Log-Group
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpecFile = "${ProjectRoot}/buildspec.json"

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach($Utility in $UtilityFunctions) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    $BuildSpec = Get-Content -Path ${BuildSpecFile} -Raw | ConvertFrom-Json

    if ( ! $SkipDeps ) {
        Install-BuildDependencies -WingetFile "${ScriptHome}/.Wingetfile"
    }

    Push-Location -Stack BuildTemp
    if ( ! ( ( $SkipAll ) -or ( $SkipBuild ) ) ) {
        Ensure-Location $ProjectRoot

        $Preset = "windows-$(if ( $env:CI -ne $null ) { 'ci-' })${Target}"
        $CmakeArgs = @(
            '--preset', $Preset
        )

        $CmakeBuildArgs = @('--build')
        $CmakeInstallArgs = @()

        if ( ( $env:TWITCH_CLIENTID -ne '' ) -and ( $env:TWITCH_HASH -ne '' ) ) {
            $CmakeArgs += @(
                "-DTWITCH_CLIENTID:STRING=${env:TWITCH_CLIENTID}"
                "-DTWITCH_HASH:STRING=${env:TWITCH_HASH}"
            )
        }

        if ( ( $env:RESTREAM_CLIENTID -ne '' ) -and ( $env:RESTREAM_HASH -ne '' ) ) {
            $CmakeArgs += @(
                "-DRESTREAM_CLIENTID:STRING=${env:RESTREAM_CLIENTID}"
                "-DRESTREAM_HASH:STRING=${env:RESTREAM_HASH}"
            )
        }

        if ( ( $env:YOUTUBE_CLIENTID -ne '' ) -and ( $env:YOUTUBE_CLIENTID_HASH -ne '' ) -and
             ( $env:YOUTUBE_SECRET -ne '' ) -and ( $env:YOUTUBE_SECRET_HASH-ne '' ) ) {
            $CmakeArgs += @(
                "-DYOUTUBE_CLIENTID:STRING=${env:YOUTUBE_CLIENTID}"
                "-DYOUTUBE_CLIENTID_HASH:STRING=${env:YOUTUBE_CLIENTID_HASH}"
                "-DYOUTUBE_SECRET:STRING=${env:YOUTUBE_SECRET}"
                "-DYOUTUBE_SECRET_HASH:STRING=${env:YOUTUBE_SECRET_HASH}"
            )
        }

        if ( $env:GPU_PRIORITY_VAL -ne '' ) {
            $CmakeArgs += @(
                "-DGPU_PRIORITY_VAL:STRING=${env:GPU_PRIORITY_VAL}"
            )
        }

        if ( ( $env:CI -ne $null ) -and ( $env:CCACHE_CONFIGPATH -ne $null ) ) {
            $CmakeArgs += @(
                "-DENABLE_CCACHE:BOOL=TRUE"
            )
        }

        if ( $VerbosePreference -eq 'Continue' ) {
            $CmakeBuildArgs += ('--verbose')
            $CmakeInstallArgs += ('--verbose')
        }

        if ( $DebugPreference -eq 'Continue' ) {
            $CmakeArgs += ('--debug-output')
        }

        $CmakeBuildArgs += @(
            '--preset', "windows-${Target}"
            '--config', $Configuration
            '--parallel'
            '--', '/consoleLoggerParameters:Summary', '/noLogo'
        )

        $CmakeInstallArgs += @(
            '--install', "build_${Target}"
            '--prefix', "${ProjectRoot}/build_${Target}/install"
            '--config', $Configuration
        )

        Log-Group "Configuring obs-studio..."
        Invoke-External cmake @CmakeArgs

        Log-Group "Building obs-studio..."
        Invoke-External cmake @CmakeBuildArgs
    }

    Log-Group "Installing obs-studio..."
    Invoke-External cmake @CmakeInstallArgs

    Pop-Location -Stack BuildTemp
    Log-Group
}

Build
