# Shared launcher operations; kept separate so bootstrap and installer paths can be tested.
function Get-NovaInstallerCompiler {
    param([string] $Root, [switch] $NonInteractive, [switch] $Check)
    $localDir = Join-Path $Root '.deps\inno-setup'
    $paths = @((Join-Path $localDir 'ISCC.exe'),
        "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe", "$env:ProgramFiles\Inno Setup 7\ISCC.exe")
    $command = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($command) { $paths = @($command.Source) + $paths }
    foreach ($path in $paths) {
        if (Test-Path -LiteralPath $path -PathType Leaf) { return $path }
    }
    if ($NonInteractive -or $Check) { throw 'Inno Setup is required to create the setup EXE. Install Inno Setup 6.7 or later, or run build-windows.bat interactively to install it.' }
    $answer = Read-Host 'Install Inno Setup 6.7.3 to create the setup EXE? [y/N]'
    if ($answer -notmatch '^(y|yes)$') { throw 'Installer compiler installation declined. No setup EXE was created.' }
    New-Item -ItemType Directory -Path (Join-Path $Root '.deps') -Force | Out-Null
    $download = Join-Path $Root '.deps\innosetup-6.7.3.exe'
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -UseBasicParsing -Uri 'https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-6.7.3.exe' -OutFile $download
    $expected = '9C73C3BAE7ED48D44112A0F48E66742C00090BDB5BEF71D9D3C056C66E97B732'
    if ((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash -ne $expected) { throw 'Inno Setup download checksum did not match.' }
    $signature = Get-AuthenticodeSignature -LiteralPath $download
    if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'O=Pyrsys B.V.') { throw 'Inno Setup publisher signature could not be verified.' }
    $process = Start-Process -FilePath $download -ArgumentList @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/CURRENTUSER', ('/DIR="{0}"' -f $localDir)) -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "Inno Setup installation failed: $($process.ExitCode)" }
    $compiler = Join-Path $localDir 'ISCC.exe'
    if (!(Test-Path -LiteralPath $compiler)) { throw 'Inno Setup compiler was not installed.' }
    return $compiler
}

function Test-NovaAtl {
    param([string] $VisualStudio)
    $versionFile = Join-Path $VisualStudio 'VC\Auxiliary\Build\Microsoft.VCToolsVersion.default.txt'
    if (!(Test-Path -LiteralPath $versionFile)) { return $false }
    $version = (Get-Content -LiteralPath $versionFile -Raw).Trim()
    $include = Join-Path $VisualStudio "VC\Tools\MSVC\$version\atlmfc\include"
    return ((Test-Path -LiteralPath (Join-Path $include 'atlcomcli.h')) -and
            (Test-Path -LiteralPath (Join-Path $include 'atlstr.h')))
}

function Install-NovaAtl {
    param([string] $VisualStudio, [string] $Installer, [string] $ChannelId)
    if (!(Test-Path -LiteralPath $Installer)) { throw 'Visual Studio Installer was not found. Repair its installation first.' }
    Write-Host 'Installing C++ ATL. Accept the Windows administrator prompt to continue.'
    $arguments = @('modify', '--installPath', ('"{0}"' -f $VisualStudio),
        '--add', 'Microsoft.VisualStudio.Component.VC.ATL', '--passive', '--norestart')
    if ($ChannelId) { $arguments += @('--channelId', $ChannelId) }
    $process = Start-Process -FilePath $Installer -ArgumentList $arguments -Verb RunAs -WindowStyle Hidden -Wait -PassThru
    if ($process.ExitCode -eq 3010) { throw 'ATL installation requires a Windows restart. Restart and run build-windows.bat again.' }
    if ($process.ExitCode -ne 0) { throw "Visual Studio Installer failed with exit code $($process.ExitCode). Close other installer windows and retry." }
    if (!(Test-NovaAtl $VisualStudio)) { throw 'ATL is still unavailable. Open Visual Studio Installer and check the C++ ATL component for this toolset.' }
}

function Initialize-NovaSource {
    param([string] $ArchiveRoot, [string] $Git)
    if (Test-Path -LiteralPath (Join-Path $ArchiveRoot '.git')) { return $ArchiveRoot }
    $source = Join-Path $ArchiveRoot 'build-source'
    $repository = 'https://github.com/Kryptographer/obs-studio.git'
    Write-Host 'Downloaded ZIP detected. Building the latest master from your GitHub fork.'
    Write-Host "A complete checkout will be kept in: $source"
    Write-Host 'The downloaded files are left intact; changes made only to the ZIP files are not built.'
    if (!(Test-Path -LiteralPath $source)) {
        Invoke-BuildCommand $Git @('clone', '--branch', 'master', '--single-branch', '--recursive', $repository, $source) | Out-Host
    } else {
        if (!(Test-Path -LiteralPath (Join-Path $source '.git'))) {
            throw 'build-source already exists but is not a Git checkout. Rename that folder and retry; no files were removed.'
        }
        $origin = & $Git -C $source config --get remote.origin.url
        if ($LASTEXITCODE -ne 0 -or $origin -ne $repository) { throw 'build-source belongs to a different repository. Rename that folder and retry.' }
        $branch = & $Git -C $source branch --show-current
        if ($LASTEXITCODE -ne 0 -or $branch -ne 'master') { throw 'build-source is not on master. Restore its branch or rename that folder and retry.' }
        $changes = & $Git -C $source status --porcelain
        if ($LASTEXITCODE -ne 0 -or $changes) { throw 'build-source has local changes. Commit or move those changes before retrying; no files were overwritten.' }
        Invoke-BuildCommand $Git @('-C', $source, 'pull', '--ff-only', 'origin', 'master') | Out-Host
    }
    return $source
}
