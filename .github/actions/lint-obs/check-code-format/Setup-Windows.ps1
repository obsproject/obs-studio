#Requires -Version 7.3

[CmdletBinding(PositionalBinding=$false)]
param()

begin {
  if ($null -eq $env:CI) { throw }
  if ($null -ne $env:RUNNER_DEBUG) { Set-PSDebug -Trace 1 }

  $ErrorActionPreference = 'Stop'
}

process {
  $WingetArguments = @(
    "--accept-package-agreements"
    "--accept-source-agreements"
    "--disable-interactivity"
    "--exact"
  )

  Write-Output "::group::Installing ${env:LINTER_COMMAND}..."
  case ( $env:LINTER_COMMAND ) {
    clang-format {
      break
    }
    gersemi {
      winget install BlankSpruce.Gersemi
      break
    }
    zizmor {
      winget install zizmor.zizmor
      break
    }
    { ( $_ -eq "swift-format" ) -or ( $_ -eq "xmllint" ) } {
      Write-Output "::error::The linter '${env:LINTER_COMMAND}' is supported on macOS and Linux only."
      throw
    }
    default {
      Write-Output "::error::Unsupported linter '${env:LINTER_COMMAND}' provided."
      throw
    }
  }
  Write-Output '::endgroup::'
}
