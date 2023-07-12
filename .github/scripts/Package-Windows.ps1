[CmdletBinding()]
param(
    [ValidateSet('x64')]
    [string] $Target = 'x64',
    [ValidateSet('Debug', 'RelWithDebInfo', 'Release', 'MinSizeRel')]
    [string] $Configuration = 'RelWithDebInfo',
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
    Write-Warning 'The obs-studio packaging script requires PowerShell Core 7. Install or upgrade your PowerShell version: https://aka.ms/pscore6'
    exit 2
}

function Package {
    trap {
        Write-Error $_
        exit 2
    }

    $ScriptHome = $PSScriptRoot
    $ProjectRoot = Resolve-Path -Path "$PSScriptRoot/../.."
    $BuildSpecFile = "${ProjectRoot}/buildspec.json"

    $UtilityFunctions = Get-ChildItem -Path $PSScriptRoot/utils.pwsh/*.ps1 -Recurse

    foreach( $Utility in $UtilityFunctions ) {
        Write-Debug "Loading $($Utility.FullName)"
        . $Utility.FullName
    }

    if ( ! $SkipDeps ) {
        Install-BuildDependencies -WingetFile "${ScriptHome}/.Wingetfile"
    }

    $GitDescription = Invoke-External git describe --tags --long
    $Tokens = ($GitDescription -split '-')
    $CommitVersion = $Tokens[0..$($Tokens.Count - 3)] -join '-'
    $CommitHash = $($Tokens[-1]).SubString(1)
    $CommitDistance = $Tokens[-2]

    if ( $CommitDistance -gt 0 ) {
        $OutputName = "obs-studio-${CommitVersion}-${CommitHash}"
    } else {
        $OutputName = "obs-studio-${CommitVersion}"
    }

    $CpackArgs = @(
        '-C', "${Configuration}"
    )

    if ( $VerbosePreference -eq 'Continue' ) {
        $CpackArgs += ('--verbose')
    }

    Log-Group "Packaging obs-studio..."

    Push-Location -Stack PackageTemp "build_${Target}"

    cpack @CpackArgs

    $Package = Get-ChildItem -filter "obs-studio-*-windows-x64.zip" -File
    Move-Item -Path $Package -Destination "${OutputName}-windows-x64.zip"

    Pop-Location -Stack PackageTemp
}

Package
