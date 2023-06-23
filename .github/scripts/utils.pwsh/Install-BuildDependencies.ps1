function Install-BuildDependencies {
    <#
        .SYNOPSIS
            Installs required build dependencies.
        .DESCRIPTION
            Additional packages might be needed for successful builds. This module contains additional
            dependencies available for installation via winget and, if possible, adds their locations
            to the environment path for future invocation.
        .EXAMPLE
            Install-BuildDependencies
    #>

    param(
        [string] $WingetFile = "$PSScriptRoot/.Wingetfile"
    )

    if ( ! ( Test-Path function:Log-Warning ) ) {
        . $PSScriptRoot/Logger.ps1
    }

    $Prefixes = @{
        'x64' = ${env:ProgramFiles}
        'x86' = ${env:ProgramFiles(x86)}
        'arm64' = ${env:ProgramFiles(arm)}
    }
    
    $Paths = $env:Path -split [System.IO.Path]::PathSeparator

    $WingetOptions = @('install', '--accept-package-agreements', '--accept-source-agreements')

    if ( $script:Quiet ) {
        $WingetOptions += '--silent'
    }

    Log-Group 'Check Windows build requirements'
    Get-Content $WingetFile | ForEach-Object {
        $_, $Package, $_, $Path, $_, $Binary, $_, $Version = $_ -replace ',','' -split " +(?=(?:[^\']*\'[^\']*\')*[^\']*$)" -replace "'",''

        $Prefixes.GetEnumerator() | ForEach-Object {
            $Prefix = $_.value
            $FullPath = "${Prefix}\${Path}"
            if ( ( Test-Path $FullPath ) -and ! ( $Paths -contains $FullPath ) ) {
                $Paths = @($FullPath) + $Paths
                $env:Path = $Paths -join [System.IO.Path]::PathSeparator
            }
        }

        Log-Debug "Checking for command ${Binary}"
        $Found = Get-Command -ErrorAction SilentlyContinue $Binary

        if ( $Found ) {
            Log-Status "Found dependency ${Binary} as $($Found.Source)"
        } else {
            Log-Status "Installing package ${Package} $(if ( $Version -ne $null ) { "Version: ${Version}" } )"

            if ( $Version -ne $null ) {
                $WingGetOptions += @('--version', ${Version})
            }

            try {
                $Params = $WingetOptions + $Package

                winget @Params
            } catch {
                throw "Error while installing winget package ${Package}: $_"
            }
        }
    }
    Log-Group
}
