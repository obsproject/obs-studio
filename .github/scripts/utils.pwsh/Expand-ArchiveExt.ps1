function Expand-ArchiveExt {
    <#
        .SYNOPSIS
            Expands archive files.
        .DESCRIPTION
            Allows extraction of zip, 7z, gz, and xz archives.
            Requires tar and 7-zip to be available on the system.
            Archives ending with .zip but created using LZMA compression are
            expanded using 7-zip as a fallback.
        .EXAMPLE
            Expand-ArchiveExt -Path <Path-To-Your-Archive>
            Expand-ArchiveExt -Path <Path-To-Your-Archive> -DestinationPath <Expansion-Path>
    #>

    param(
        [Parameter(Mandatory)]
        [string] $Path,
        [string] $DestinationPath = [System.IO.Path]::GetFileNameWithoutExtension($Path),
        [switch] $Force
    )

    switch ( [System.IO.Path]::GetExtension($Path) ) {
        .zip {
            try {
                Expand-Archive -Path $Path -DestinationPath $DestinationPath -Force:$Force
            } catch {
                if ( Get-Command 7z ) {
                    Invoke-External 7z x -y $Path "-o${DestinationPath}"
                } else {
                    throw "Fallback utility 7-zip not found. Please install 7-zip first."
                }
            }
            break
        }
        { ( $_ -eq ".7z" ) -or ( $_ -eq ".exe" ) } {
            if ( Get-Command 7z ) {
                Invoke-External 7z x -y $Path "-o${DestinationPath}"
            } else {
                throw "Extraction utility 7-zip not found. Please install 7-zip first."
            }
            break
        }
        .gz {
            try {
                Invoke-External tar -x -o $DestinationPath -f $Path
            } catch {
                if ( Get-Command 7z ) {
                    Invoke-External 7z x -y $Path "-o${DestinationPath}"
                } else {
                    throw "Fallback utility 7-zip not found. Please install 7-zip first."
                }
            }
            break
        }
        .xz {
            try {
                Invoke-External tar -x -o $DestinationPath -f $Path
            } catch {
                if ( Get-Command 7z ) {
                    Invoke-External 7z x -y $Path "-o${DestinationPath}"
                } else {
                    throw "Fallback utility 7-zip not found. Please install 7-zip first."
                }
            }
        }
        default {
            throw "Unsupported archive extension provided."
        }
    }
}
