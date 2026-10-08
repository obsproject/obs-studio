#Requires -Version 7.3

[CmdletBinding(PositionalBinding=$false)]
param(
    [ValidateSet('clang-format', 'swift-format', 'gersemi', 'zizmor', 'xmllint')]
    [string] $Linter,
    [switch] $Check,
    [switch] $GitHubStyle,
    [Parameter(ValueFromRemainingArguments)]
    [string[]] $SourceFiles
)

begin {
    $_EAP = $ErrorActionPreference
    $_DP = $DebugPreference
    $_VP = $VerbosePreference
    $_IP = $InformationPreference

    $ErrorActionPreference = 'Stop'

    if ( $DebugPreference -eq 'Continue' ) {
        $VerbosePreference = 'Continue'
        $InformationPreference = 'Continue'
    }

    $Sep = [System.IO.Path]::DirectorySeparatorChar
    $Sep = "${Sep}${Sep}"

    function Invoke-External {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory=$true, Position=0)]
            [string] $Command,
            [Parameter(ValueFromRemainingArguments, Position=1)]
            [string[]] $Arguments
        )

        begin {
            $_EAP = $ErrorActionPreference
            $ErrorActionPreference = 'Continue'
        }

        process {
            Write-Debug "Invoke-External: ${Command} ${Arguments} 2>&1"

            & $Command @Arguments 2>&1
            $Result = $LASTEXITCODE
        }

        end {
            $ErrorActionPreference = $_EAP

            if ( $Result -ne 0 ) {
                throw "${Command} ${Arguments} exited with non-zero code ${Result}."
            }
        }
    }

    function Test-CommandExists {
        [CmdletBinding()]
        Param(
            [Parameter(Mandatory)]
            [String] $Command
        )

        process {
            try {
                Get-Command $Command -ErrorAction 'Stop'
                $true
            } catch {
                $false
            }
        }
    }

    function Check-Linter {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory)]
            [ValidateSet('clang-format', 'swift-format', 'gersemi', 'zizmor', 'xmllint')]
            [string] $Linter
        )

        begin {
            $Found = $false

            $LinterCommand = $null
            $MinimumVersion = $null
            $VersionNumber = $null
        }

        process {
            switch ($Linter) {
                clang-format {
                    if ((Test-CommandExists 'clang-format-22')) {
                        $LinterCommand = Get-Command 'clang-format-22'
                    } elseif ((Test-CommandExists 'clang-format')) {
                        $LinterCommand = Get-Command 'clang-format'
                    } else {
                        break
                    }

                    $MinimumVersion = New-Object -TypeName System.Version -ArgumentList '22.1.3'

                    $ClangFormatVersion = (($( Invoke-External $LinterCommand --version ) -split ' ')[2])
                    $VersionNumber = New-Object -TypeName System.Version -ArgumentList $ClangFormatVersion

                    $Found = $true
                    break
                }
                gersemi {
                    if ((Test-CommandExists 'gersemi')) {
                        $LinterCommand = Get-Command 'gersemi'
                    } else {
                        break
                    }

                    $MinimumVersion = New-Object -TypeName System.Version -ArgumentList '0.27.0'

                    $GersemiVersion = (($( Invoke-External $LinterCommand --version ) -split ' ')[1])
                    $VersionNumber = New-Object -TypeName System.Version -ArgumentList $GersemiVersion

                    $Found = $true
                    break
                }
                zizmor {
                    if ((Test-CommandExists 'zizmor')) {
                        $LinterCommand = Get-Command 'zizmor'
                    } else {
                        break
                    }

                    $MinimumVersion = New-Object -TypeName System.Version -ArgumentList '1.25.0'

                    $ZizmorVersion = (($( Invoke-External $LinterCommand --version ) -split ' ')[1])
                    $VersionNumber = New-Object -TypeName System.Version -ArgumentList $ZizmorVersion

                    $Found = $true
                    break
                }
                default {
                    throw "Unsupported linter '${Linter}' specified."
                }
            }
        }

        end {
            if ($Found -eq $false) {
                throw "Unable to find '${Linter}' on system."
            }

            if (!($VersionNumber -ge $MinimumVersion)) {
                throw "${Linter} ${VersionNumber} found (Required: ${MinimumVersion})."
            }

            $LinterCommand
        }
    }

    function Generate-File-List {
        [CmdletBinding()]
        param(
            [Parameter(Mandatory)]
            [ValidateSet('clang-format', 'swift-format', 'gersemi', 'zizmor', 'xmllint')]
            [string] $Linter
        )

        begin {
            $Files = $null
            $ProjectRootPattern = "^$([regex]::Escape($ProjectRoot))${Sep}"
        }

        process {

            switch ($Linter) {
                clang-format {
                    $Directories = Get-ChildItem -Path $ProjectRoot -Attribute Directory | Where-Object {
                        $_.Name -match '^(libobs*|frontend|plugins|deps|shared|test)'
                    }

                    $Pattern = ".*${Sep}(decklink${Sep}.+${Sep}decklink-sdk|obs-websocket|obs-browser|libdshowcapture)"
                    $Files = $Directories | ForEach-Object {
                        Get-ChildItem -Path $_  -Recurse -File -Include '*.c','*.h','*.m','*.hpp','*.cpp','*.mm'
                    } | Where-Object {
                        ! ($_.Directory.FullName -match $Pattern)
                    } | ForEach-Object {
                        $Sep = [System.IO.Path]::DirectorySeparatorChar
                        ($_.FullName) -replace $ProjectRootPattern,".${Sep}"
                    }
                    break
                }
                gersemi {
                    $Directories = Get-ChildItem -Path $ProjectRoot -Attribute Directory | Where-Object {
                        $_.Name -match '^(libobs*|frontend|plugins|deps|shared|cmake|test)'
                    }

                    $Pattern = ".*${Sep}(jansson|decklink${Sep}.+${Sep}decklink-sdk|libdshowcapture|obs-websocket|obs-browser)"
                    $Files = $Directories | ForEach-Object {
                        Get-ChildItem -Path $_ -Recurse -File -Include '*.cmake','CmakeLists.txt'
                    } | Where-Object {
                        ! ($_.Directory.FullName -match $Pattern)
                    } | ForEach-Object {
                        $Sep = [System.IO.Path]::DirectorySeparatorChar
                        ($_.FullName) -replace $ProjectRootPattern,".${Sep}"
                    }
                    $Files += ".${Sep}CMakeLists.txt"
                    break
                }
                zizmor {
                    $Directories = Get-ChildItem -Path $ProjectRoot -Attribute Directory | Where-Object {
                        $_.Name -match '^.github/(workflows|actions)'
                    }
                    $Files = $Directories | ForEach-Object {
                        Get-ChildItem -Path $_  -Recurse -File -Include '*.yaml','*.yml'
                    } | ForEach-Object {
                        $Sep = [System.IO.Path]::DirectorySeparatorChar
                        ($_.FullName) -replace $ProjectRootPattern,".${Sep}"
                    }
                    break
                }
                default {
                    break
                }
            }
        }

        end {
            $Files
        }
    }

    function Invoke-Formatter {
        [CmdletBinding(PositionalBinding=$false)]
        param(
            [Parameter(Mandatory)]
            [ValidateSet('clang-format', 'swift-format', 'gersemi', 'zizmor', 'xmllint')]
            [string] $Formatter,
            [Parameter(Mandatory)]
            [object] $FormatterCommand,
            [Parameter(ValueFromRemainingArguments)]
            [string[]] $SourceFiles
        )

        begin {
            $FormatterArguments = $null
            $TempFile = New-TemporaryFile
        }

        process {
            switch ($Formatter) {
                clang-format {
                    $FormatterArguments = @(
                        '--style=file'
                        '--fallback-style=none'
                        $( if($VerbosePreference -eq 'Continue') {'--verbose'} )
                        '-i'
                    )
                    break
                }
                gersemi {
                    $FormatterArguments = @(
                        '--no-cache'
                        '-i'
                    )
                    break
                }
                default {
                    throw "Unsupported formatter '${Formatter}' specified"
                }
            }

            # Special handling for clang-format: Due to the amount of eligible source files in the project,
            # a command line listing all files to format will exceed Windows's limit of 8191 characters.
            # clang and clang-format support a special mode to provide the list of files in a separate file,
            # which is used here.
            if ($Formatter -eq 'clang-format') {
                $SourceFiles | Out-File -FilePath $TempFile
                $FilesToFormat = @( "@$( $TempFile.FullName )" )
            } else {
                $FilesToFormat = $SourceFiles
            }

            try {
                Invoke-External $FormatterCommand.Source @FormatterArguments @FilesToFormat
            } catch {}
        }

        end {
            Remove-Item $TempFile
        }
    }

    function Invoke-Linter {
        [CmdletBinding(PositionalBinding=$false)]
        param(
            [Parameter(Mandatory)]
            [ValidateSet('clang-format', 'swift-format', 'gersemi', 'zizmor', 'xmllint')]
            [string] $Linter,
            [Parameter(Mandatory)]
            [object] $LinterCommand,
            [Parameter(ValueFromRemainingArguments)]
            [string[]] $SourceFiles
        )

        begin {
            $LinterArguments = $null
            $RegexpPattern = $null
            $Indices = $null

            $NumFailures = 0

            $TempFile = New-TemporaryFile
        }

        process {
            switch ( $Linter ) {
                clang-format {
                    $RegexpPattern = '^([^:]+):([0-9]+):[0-9]+:\s(.+):\s(.+)\[-W(.+)\]$'
                    $Indices = @(1,2,3,5,4)
                    $LinterArguments = @(
                        '--style=file'
                        '--fallback-style=none'
                        '-Werror'
                        '--dry-run'
                        $( if($VerbosePreference -eq 'Continue') {'--verbose'} )
                    )
                    break
                }
                gersemi {
                    $RegexpPattern = "^$([regex]::Escape($ProjectRoot))${Sep}([^\s]+)\s(.+)"
                    $Indices = @(1,'Entire File','error','gersemi',2)
                    $LinterArguments = @(
                        '--check'
                        '--no-cache'
                        '--warnings-as-errors'
                    )
                    break
                }
                zizmor {
                    $RegexpPattern = '^::(.+)\sfile=(.+),line=([0-9]+),title=(.+)::.+:[0-9]+:\s(.+)$'
                    $Indices = @(2,3,1,4,5)
                    $LinterArguments = @(
                        '--offline'
                        '--persona=auditor'
                        '--format=github'
                        '--no-progress'
                        '--quiet'
                    )
                    break
                }
                default {
                    throw "Unsupported linter '${Linter}' specified"
                }
            }

            # Special handling for clang-format: Due to the amount of eligible source files in the project,
            # a command line listing all files to format will exceed Windows's limit of 8191 characters.
            # clang and clang-format support a special mode to provide the list of files in a separate file,
            # which is used here.
            if ($Linter -eq 'clang-format') {
                $SourceFiles | Out-File -FilePath $TempFile
                $FilesToFormat = @( "@$($TempFile.FullName)" )
            } else {
                $FilesToFormat = $SourceFiles
            }

            try {
                Invoke-External $LinterCommand.Source @LinterArguments @FilesToFormat | ForEach-Object {
                    if (($Linter -eq 'zizmor') -and ($script:GitHubStyle)) {
                        Write-Host $_
                        $NumFailures += 1
                        continue
                    }

                    $Matched = $_ -match $RegexpPattern

                    if ($Matched -eq $true) {
                        $FilePath, $LineNumber, $ErrorLevel, $ErrorTitle, $ErrorMessage = $Indices | ForEach-Object {
                            $Matches[$_] ?? $_
                        }

                        $FilePath = ($FilePath -replace '\\','/') -replace '\./','/'
                        if ($script:GitHubStyle) {
                            $FileName = ($FilePath | Get-Item).Name
                            Write-Host "::${ErrorLevel} file=${FilePath},line=${LineNumber},title=${ErrorTitle}::${FileName}:${LineNumber}: ${ErrorMessage}"
                        } else {
                            Write-Host -NoNewLine -ForegroundColor Red "  ✖  "
                            Write-Host "${FilePath}:${LineNumber} - ${ErrorTitle}: ${ErrorMessage}"
                        }
                        $NumFailures += 1
                    } else {
                        Write-Host $_
                    }
                }
            } catch {}
        }

        end {
            Remove-Item $TempFile

            $NumFailures
        }
    }
}

process {
    $ScriptHome = $PSScriptRoot
    $ProjectRoot = ($PSScriptRoot | Get-Item).Parent

    $LinterCommand = $null

    if (($null -eq $Linter) -and ($null -ne $env:LINTER_COMMAND)) {
        $Linter = $env:LINTER_COMMAND
    }

    $LinterCommand = Check-Linter -Linter $Linter

    if ($Linter -eq $null) {
        throw 'No linter detected or provided via ''LINTER_COMMAND'' environment variable.'
    }

    if ($SourceFiles -eq $null ) {
        $SourceFiles = Generate-File-List -Linter $Linter
    } else {
        $SourceFiles = Get-ChildItem -Path ${SourceFiles} | ForEach-Object {
            ($_.FullName)  -replace "^$([regex]::Escape($ProjectRoot))${Sep}",''
        }
    }

    $NumFailures = 0

    if ($script:Check) {
        $NumFailures = Invoke-Linter -LinterCommand $LinterCommand -Linter $Linter @SourceFiles
    } else {
        Invoke-Formatter -FormatterCommand $LinterCommand -Formatter $Linter @SourceFiles
    }
}

end {
    $ErrorActionPreference = $_EAP
    $DebugPreference = $_DP
    $VerbosePreference = $_VP
    $InformationPreference = $_IP

    if ($NumFailures -gt 0) {
        exit 1
    } else {
        exit 0
    }
}
