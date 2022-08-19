$CIWorkflow = "${CheckoutDir}/.github/workflows/main.yml"
$WorkflowContent = Get-Content ${CIWorkflow}

$CIDepsVersion = ${WorkflowContent} | Select-String "[ ]+DEPS_VERSION_WIN: '([0-9\-]+)'" | ForEach-Object{$_.Matches.Groups[1].Value}
$CIQtVersion = ${WorkflowContent} | Select-String "[ ]+QT_VERSION_WIN: '([0-9\.]+)'" | ForEach-Object{$_.Matches.Groups[1].Value}
$CIVlcVersion = ${WorkflowContent} | Select-String "[ ]+VLC_VERSION_WIN: '(.+)'" | ForEach-Object{$_.Matches.Groups[1].Value}
$CICefVersion = ${WorkflowContent} | Select-String "[ ]+CEF_BUILD_VERSION_WIN: '([0-9\.]+)'" | ForEach-Object{$_.Matches.Groups[1].Value}
$CIGenerator = ${WorkflowContent} | Select-String "[ ]+CMAKE_GENERATOR: '(.+)'" | ForEach-Object{$_.Matches.Groups[1].Value}

function Write-Status {
    Param(
        [Parameter(Mandatory=$true)]
        [String] $Output
    )

    if (!($Quiet.isPresent)) {
        if (Test-Path Env:CI) {
            Write-Host "[${ProductName}] ${Output}"
        } else {
            Write-Host -ForegroundColor blue "[${ProductName}] ${Output}"
        }
    }
}

function Write-Info {
    Param(
        [Parameter(Mandatory=$true)]
        [String] $Output
    )

    if (!($Quiet.isPresent)) {
        if (Test-Path Env:CI) {
            Write-Host " + ${Output}"
        } else {
            Write-Host -ForegroundColor DarkYellow " + ${Output}"
        }
    }
}

function Write-Step {
    Param(
        [Parameter(Mandatory=$true)]
        [String] $Output
    )

    if (!($Quiet.isPresent)) {
        if (Test-Path Env:CI) {
            Write-Host " + ${Output}"
        } else {
            Write-Host -ForegroundColor green " + ${Output}"
        }
    }
}

function Write-Failure {
    Param(
        [Parameter(Mandatory=$true)]
        [String] $Output
    )

    if (Test-Path Env:CI) {
        Write-Host " + ${Output}"
    } else {
        Write-Host -ForegroundColor red " + ${Output}"
    }
}

function Test-CommandExists {
    Param(
        [Parameter(Mandatory=$true)]
        [String] $Command
    )

    $CommandExists = $false
    $OldActionPref = $ErrorActionPreference
    $ErrorActionPreference = "stop"

    try {
        if (Get-Command $Command) {
            $CommandExists = $true
        }
    } Catch {
        $CommandExists = $false
    } Finally {
        $ErrorActionPreference = $OldActionPref
    }

    return $CommandExists
}

function Ensure-Directory {
    Param(
        [Parameter(Mandatory=$true)]
        [String] $Directory
    )

    if (!(Test-Path $Directory)) {
        $null = New-Item -ItemType Directory -Force -Path $Directory
    }

    Set-Location -Path $Directory
}

function Invoke-External {
    <#
        .SYNOPSIS
            Invokes a non-PowerShell command.
        .DESCRIPTION
            Runs a non-PowerShell command, and captures its return code.
            Throws an exception if the command returns non-zero.
        .EXAMPLE
            Invoke-External 7z x $MyArchive
    #>

    if ( $args.Count -eq 0 ) {
        throw 'Invoke-External called without arguments.'
    }

    $Command = $args[0]
    $CommandArgs = @()

    if ( $args.Count -gt 1) {
        $CommandArgs = $args[1..($args.Count - 1)]
    }

    $_EAP = $ErrorActionPreference
    $ErrorActionPreference = "Continue"

    Write-Debug "Invoke-External: ${Command} ${CommandArgs}"

    & $command $commandArgs
    $Result = $LASTEXITCODE

    $ErrorActionPreference = $_EAP

    if ( $Result -ne 0 ) {
        throw "${Command} ${CommandArgs} exited with non-zero code ${Result}."
    }
}

$BuildDirectory = "$(if (Test-Path Env:BuildDirectory) { $env:BuildDirectory } else { $BuildDirectory })"
$BuildConfiguration = "$(if (Test-Path Env:BuildConfiguration) { $env:BuildConfiguration } else { $BuildConfiguration })"
$BuildArch = "$(if (Test-Path Env:BuildArch) { $env:BuildArch } else { $BuildArch })"
$WindowsDepsVersion = "$(if (Test-Path Env:WindowsDepsVersion ) { $env:WindowsDepsVersion } else { $CIDepsVersion })"
$WindowsQtVersion = "$(if (Test-Path Env:WindowsQtVersion ) { $env:WindowsQtVersion } else { $CIQtVersion })"
$WindowsVlcVersion = "$(if (Test-Path Env:WindowsVlcVersion ) { $env:WindowsVlcVersion } else { $CIVlcVersion })"
$WindowsCefVersion = "$(if (Test-Path Env:WindowsCefVersion ) { $env:WindowsCefVersion } else { $CICefVersion })"
$CmakeSystemVersion = "$(if (Test-Path Env:CMAKE_SYSTEM_VERSION) { $Env:CMAKE_SYSTEM_VERSION } else { "10.0.18363.657" })"
$CmakeGenerator = "$(if (Test-Path Env:CmakeGenerator) { $Env:CmakeGenerator } else { $CIGenerator })"


function Install-Windows-Dependencies {
    $WingetFile = "$PSScriptRoot/Wingetfile"

    $Host64Bit = [System.Environment]::Is64BitOperatingSystem

    $Prefix = (${Env:ProgramFiles(x86)}, $Env:ProgramFiles)[$Host64Bit]

    $Paths = $Env:Path -split [System.IO.Path]::PathSeparator

    $WingetOptions = @('install', '--accept-package-agreements', '--accept-source-agreements')

    if ( $script:Quiet ) {
        $WingetOptions += '--silent'
    }

    Get-Content $WingetFile | ForEach-Object {
        $_, $Package, $_, $Path, $_, $Binary = $_ -replace ',','' -replace "'", '' -split ' '

        $FullPath = "${Prefix}\${Path}"
        if ( ( Test-Path $FullPath  ) -and ! ( $Paths -contains $FullPath ) ) {
            $Paths += $FullPath
            $Env:Path = $Paths -join [System.IO.Path]::PathSeparator
        }

        Write-Step "Checking for command ${Binary}"
        $Found = Get-Command -ErrorAction SilentlyContinue $Binary

        if ( $Found ) {
            Write-Info "Found dependency ${Binary} as $($Found.Source)"
        } else {
            Write-Info "Installing package ${Package}"

            try {
                $Params = $WingetOptions + $Package

                winget @Params
            } catch {
                throw "Error while installing winget package ${Package}: $_"
            }
        }
    }
}
