# Run with Windows PowerShell 5.1; no downloads, installer changes or Pester needed.
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot '..\Build-Nova.Support.ps1')
$testRoot = Join-Path ([IO.Path]::GetTempPath()) ('nova launcher tests ' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $testRoot | Out-Null
$script:calls = @()
$script:origin = 'https://github.com/Kryptographer/obs-studio.git'
$script:branch = 'master'
$script:changes = ''

function Assert($Condition, $Message) {
    if (!$Condition) { throw "FAIL: $Message" }
}
function Assert-Throws([scriptblock] $Action, [string] $Pattern) {
    try { & $Action } catch {
        Assert ($_.Exception.Message -match $Pattern) $_.Exception.Message
        return
    }
    throw "FAIL: expected error matching $Pattern"
}
function Invoke-BuildCommand {
    param($Executable, [string[]] $Arguments)
    $script:calls += ,$Arguments
    if ($Arguments[0] -eq 'clone') {
        New-Item -ItemType Directory -Path (Join-Path $Arguments[-1] '.git') -Force | Out-Null
    }
}
function NovaTestGit {
    $global:LASTEXITCODE = 0
    if ($args -contains 'config') { return $script:origin }
    if ($args -contains 'branch') { return $script:branch }
    if ($args -contains 'status') { return $script:changes }
    throw 'Unexpected Git query'
}
function Start-Process {
    param($FilePath, $ArgumentList, $Verb, $WindowStyle, [switch] $Wait, [switch] $PassThru)
    $script:installerArgs = $ArgumentList
    Assert ($Verb -eq 'RunAs') 'installer requests elevation'
    Assert ($Wait -and $PassThru) 'installer waits and checks result'
    if ($script:installerCode -eq 0) {
        New-Item -ItemType File -Path (Join-Path $script:atlInclude 'atlcomcli.h') -Force | Out-Null
        New-Item -ItemType File -Path (Join-Path $script:atlInclude 'atlstr.h') -Force | Out-Null
    }
    return [pscustomobject]@{ExitCode = $script:installerCode}
}
try {
    $source = Initialize-NovaSource $testRoot 'NovaTestGit'
    Assert ($source -eq (Join-Path $testRoot 'build-source')) 'ZIP returns checkout path only'
    Assert ($script:calls[0][0] -eq 'clone') 'ZIP triggers clone'
    Assert ($script:calls[0] -contains '--recursive') 'clone includes submodules'
    Assert ($script:calls[0] -contains $script:origin) 'clone uses requested fork'
    $again = Initialize-NovaSource $testRoot 'NovaTestGit'
    Assert ($again -eq $source) 'repeat uses same checkout'
    Assert ($script:calls[1] -contains '--ff-only') 'repeat updates without overwriting history'
    $script:changes = ' M custom.cpp'
    Assert-Throws { Initialize-NovaSource $testRoot 'NovaTestGit' } 'local changes'
    $script:changes = ''
    $script:origin = 'https://example.com/another-repo.git'
    Assert-Throws { Initialize-NovaSource $testRoot 'NovaTestGit' } 'different repository'
    $script:origin = 'https://github.com/Kryptographer/obs-studio.git'
    $script:branch = 'custom'
    Assert-Throws { Initialize-NovaSource $testRoot 'NovaTestGit' } 'not on master'
    $before = $script:calls.Count
    Assert ((Initialize-NovaSource $source 'NovaTestGit') -eq $source) 'existing Git checkout is kept'
    Assert ($script:calls.Count -eq $before) 'existing checkout is not bootstrapped'
    $incomplete = Join-Path $testRoot 'incomplete'
    New-Item -ItemType Directory -Path (Join-Path $incomplete 'build-source') -Force | Out-Null
    Assert-Throws { Initialize-NovaSource $incomplete 'NovaTestGit' } 'not a Git checkout'

    $vs = Join-Path $testRoot 'Visual Studio'
    $versionDir = Join-Path $vs 'VC\Auxiliary\Build'
    New-Item -ItemType Directory -Path $versionDir -Force | Out-Null
    Set-Content -LiteralPath (Join-Path $versionDir 'Microsoft.VCToolsVersion.default.txt') -Value '14.51.0'
    $script:atlInclude = Join-Path $vs 'VC\Tools\MSVC\14.51.0\atlmfc\include'
    New-Item -ItemType Directory -Path $script:atlInclude -Force | Out-Null
    Assert (!(Test-NovaAtl $vs)) 'missing ATL detected'
    $installer = Join-Path $testRoot 'setup.exe'
    New-Item -ItemType File -Path $installer | Out-Null
    $script:installerCode = 0
    Install-NovaAtl $vs $installer 'VisualStudio.18.Release'
    Assert (Test-NovaAtl $vs) 'successful install rechecks headers'
    Assert ($script:installerArgs -contains 'Microsoft.VisualStudio.Component.VC.ATL') 'only ATL requested'
    Assert ($script:installerArgs -contains ('"{0}"' -f $vs)) 'installation path with spaces quoted'
    $script:installerCode = 3010
    Assert-Throws { Install-NovaAtl $vs $installer '' } 'restart'
    $script:installerCode = 1602
    Assert-Throws { Install-NovaAtl $vs $installer '' } 'exit code 1602'
    Write-Host 'PASS: ZIP bootstrap/reuse, checkout protection, ATL detection/install/restart/failure'
} finally {
    # Delete only this test's verified, uniquely named temporary fixture directory.
    $resolved = [IO.Path]::GetFullPath($testRoot)
    $tempRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath()).TrimEnd('\') + '\'
    if ($resolved.StartsWith($tempRoot, [StringComparison]::OrdinalIgnoreCase) -and
        (Split-Path -Leaf $resolved).StartsWith('nova launcher tests ')) {
        Remove-Item -LiteralPath $resolved -Recurse -Force
    }
}
