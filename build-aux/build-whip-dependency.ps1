param(
    [string] $OBSBuildDirectory = 'build_whip_review',
    [ValidateSet('Release', 'RelWithDebInfo')] [string] $Configuration = 'RelWithDebInfo',
    [ValidateRange(1, 8)] [int] $Jobs = 4
)

# Rebuild only the Windows WHIP dependency, using OBS's pinned source and TLS backend.
# Configure OBS normally first. The downloaded obs-deps package stays untouched.
$ErrorActionPreference = 'Stop'
$repoRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$obsBuild = [IO.Path]::GetFullPath((Join-Path $repoRoot $OBSBuildDirectory))
$source = Join-Path $repoRoot '.deps/libdatachannel-whip-review'
$dependencyBuild = Join-Path $repoRoot 'build_whip_dependency'
$install = Join-Path $dependencyBuild 'install'
$testBuild = Join-Path $repoRoot 'build_whip_ice_test'
$patch = Join-Path $PSScriptRoot 'whip-dependency/libjuice-zero-tiebreaker.patch'
# Matches obsproject/obs-deps 2026-08-26, deps.ffmpeg/70-libdatachannel.ps1.
$revision = '4e4f4892dccb2a57fe3a490d0c9d958de4244e74'
$juiceRevision = '5948a4162d37bc213d6051b67ee2876ccc5a99a6'
$prebuilt = Join-Path $repoRoot '.deps/obs-deps-2026-08-26-x64'
$cmake = (Get-Command cmake -ErrorAction Stop).Source

function Invoke-Checked([string] $Program, [string[]] $Arguments) {
    if ($Program -eq $cmake) { $Arguments = @($Arguments | ForEach-Object { $_.Replace('\', '/') }) }
    # Windows PowerShell can turn native stderr (including git progress) into errors.
    $ErrorActionPreference = 'Continue'
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

if ($Jobs -gt [Math]::Floor([Environment]::ProcessorCount / 2)) { throw 'Jobs must not exceed half the logical CPUs' }
if (Get-Process obs64 -ErrorAction SilentlyContinue) { throw 'Close OBS before rebuilding its runtime' }
if (!(Test-Path "$obsBuild/CMakeCache.txt") -or !(Test-Path "$prebuilt/include/rtc/rtc.hpp")) {
    throw 'Configure a Windows x64 OBS build with the 2026-08-26 obs-deps package first'
}
$cache = [IO.File]::ReadAllText("$obsBuild/CMakeCache.txt")
if ($cache -notmatch 'CMAKE_GENERATOR:INTERNAL=(Visual Studio[^\r\n]+)') { throw 'This helper requires an existing Visual Studio build' }
$generator = $Matches[1]
$oldCompilerFlags = $env:_CL_
$oldParallelLevel = $env:CMAKE_BUILD_PARALLEL_LEVEL
try {
    $env:_CL_ = "$oldCompilerFlags /MP1"
    $env:CMAKE_BUILD_PARALLEL_LEVEL = '1'
    if (!(Test-Path "$source/.git")) {
        Invoke-Checked git @('clone', '--depth', '1', '--branch', 'v0.24.2', 'https://github.com/paullouisageneau/libdatachannel.git', $source)
        Invoke-Checked git @('-C', $source, 'fetch', '--depth', '1', 'origin', $revision)
        Invoke-Checked git @('-C', $source, 'checkout', '--detach', $revision)
    }
    if ((git -C $source rev-parse HEAD) -ne $revision) { throw 'Unexpected libdatachannel revision; existing checkout preserved' }
    Invoke-Checked git @('-C', $source, 'diff', '--exit-code', '--', '.', ':!deps/libjuice')
    Invoke-Checked git @('-C', $source, 'submodule', 'update', '--init', '--recursive', '--jobs', "$Jobs")
    $juice = Join-Path $source 'deps/libjuice'
    if ((git -C $juice rev-parse HEAD) -ne $juiceRevision) { throw 'Unexpected libjuice revision' }
    $existingDiff = ((git -C $juice diff --binary) -join "`n").Trim()
    $expectedDiff = [IO.File]::ReadAllText($patch).Replace("`r`n", "`n").Trim()
    if (!$existingDiff) {
        Invoke-Checked git @('-C', $juice, 'apply', '--check', '--ignore-space-change', $patch)
        Invoke-Checked git @('-C', $juice, 'apply', '--ignore-space-change', $patch)
    } elseif ($existingDiff -ne $expectedDiff) {
        throw 'Unexpected libjuice edits; existing checkout preserved'
    }
    Invoke-Checked $cmake @('-S', $source, '-B', $dependencyBuild, '-G', $generator, '-A', 'x64',
        '-DUSE_MBEDTLS=ON', '-DNO_WEBSOCKET=ON', '-DNO_TESTS=ON', '-DNO_EXAMPLES=ON', '-DBUILD_SHARED_LIBS=ON', '-DCMAKE_POLICY_VERSION_MINIMUM=3.5',
        "-DCMAKE_PREFIX_PATH=$prebuilt", "-DCMAKE_INSTALL_PREFIX=$install")
    Invoke-Checked $cmake @('--build', $dependencyBuild, '--config', 'Release', '--target', 'datachannel', '--parallel', "$Jobs", '--', '/p:CL_MPCount=1')
    Invoke-Checked $cmake @('-S', "$repoRoot/test/whip", '-B', $testBuild, '-G', $generator, '-A', 'x64', "-DLIBJUICE_SOURCE_DIR=$juice")
    Invoke-Checked $cmake @('--build', $testBuild, '--config', 'Release', '--target', 'whip-ice-test', '--parallel', "$Jobs", '--', '/p:CL_MPCount=1')
    Invoke-Checked "$testBuild/Release/whip-ice-test.exe" @()
    Invoke-Checked $cmake @('--install', $dependencyBuild, '--config', 'Release')
    Invoke-Checked $cmake @('-S', $repoRoot, '-B', $obsBuild, "-DLibDataChannel_DIR=$install/lib/cmake/LibDataChannel")
    # Several Windows capture targets launch the same x86 build directory.
    # Serialize the outer OBS build so their nested ZERO_CHECK steps cannot race.
    Invoke-Checked $cmake @('--build', $obsBuild, '--config', $Configuration, '--parallel', '1', '--', '/p:CL_MPCount=1')
    # OBS may not relink its frontend when only an imported DLL changes.
    $runtime = "$obsBuild/rundir/$Configuration/bin/64bit"
    Copy-Item -LiteralPath "$install/bin/datachannel.dll" -Destination "$runtime/datachannel.dll"
    Write-Output "Patched WHIP runtime: $runtime/obs64.exe"
    Get-FileHash -LiteralPath "$runtime/datachannel.dll"
} finally {
    $env:_CL_ = $oldCompilerFlags
    $env:CMAKE_BUILD_PARALLEL_LEVEL = $oldParallelLevel
}
