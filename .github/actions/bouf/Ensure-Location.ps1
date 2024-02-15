function Ensure-Location {
    <#
        .SYNOPSIS
            Ensures current location to be set to specified directory.
        .DESCRIPTION
            If specified directory exists, switch to it. Otherwise create it,
            then switch.
        .EXAMPLE
            Ensure-Location "My-Directory"
            Ensure-Location -Path "Path-To-My-Directory"
    #>

    param(
        [Parameter(Mandatory)]
        [string] $Path
    )

    if ( ! ( Test-Path $Path ) ) {
        $_Params = @{
            ItemType = "Directory"
            Path = ${Path}
            ErrorAction = "SilentlyContinue"
        }

        New-Item @_Params | Set-Location
    } else {
        Set-Location -Path ${Path}
    }
}
