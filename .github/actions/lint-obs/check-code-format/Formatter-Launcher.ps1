#Requires -Version 7.3

[CmdletBinding(PositionalBinding=$false)]
param()

begin {
  if ($null -eq $env:CI) { throw }
  if ($null -ne $env:RUNNER_DEBUG) { Set-PSDebug -Trace 1 }

  $_EAP = $ErrorActionPreference
  if ($env:FAIL_MODE -eq "never") {
    $ErrorActionPreference = "SilentlyContinue"
  } else {
    $ErrorActionPreference = 'Stop'
  }
}

process {
  $ChangedFiles = $env:CHANGED_FILES | ConvertFrom-JSON

  Launcher = "${env:GITHUB_WORKSPACE}/build-aux/.run-format.ps1"

  . ${Launcher} -Linter ${env:LINTER_COMMAND} -Check -GitHubStyle @ChangedFiles
}

end {
  $ErrorActionPreference = $_EAP
}
