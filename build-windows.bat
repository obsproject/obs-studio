@echo off
setlocal EnableExtensions DisableDelayedExpansion
pushd "%~dp0"
if errorlevel 1 exit /b 1

where git >nul 2>nul
if errorlevel 1 (
    echo ERROR: Install Git for Windows and add it to PATH.
    goto failure
)
where cmake >nul 2>nul
if errorlevel 1 (
    echo ERROR: Install CMake with Visual Studio 18 2026 support and add it to PATH.
    echo Also install Visual Studio 2026 Desktop development with C++
    echo and Windows SDK 10.0.26100.0, as required by CMakePresets.json.
    goto failure
)

echo Checking the Windows x64 build preset...
call cmake --list-presets
if errorlevel 1 goto failure
if /i "%~1"=="--check" goto success
if not "%~1"=="" (
    echo Usage: build-windows.bat [--check]
    goto failure
)

echo Initializing OBS submodules...
call git submodule update --init --recursive
if errorlevel 1 goto failure

echo Configuring OBS. The first build downloads dependencies and needs internet access.
call cmake --preset windows-x64
if errorlevel 1 goto failure

echo Building the Release executable...
call cmake --build --preset windows-x64 --config Release --parallel
if errorlevel 1 goto failure

echo Collecting the executable, plugins and runtime dependencies...
call cmake --install build_x64 --config Release --prefix "%~dp0build_x64\install"
if errorlevel 1 goto failure
if not exist "%~dp0build_x64\install\bin\64bit\obs64.exe" (
    echo ERROR: Build completed but the expected installed executable was not found.
    goto failure
)

echo.
echo Ready: "%~dp0build_x64\install\bin\64bit\obs64.exe"
echo Keep the entire build_x64\install folder together; OBS needs its DLLs and data.
:success
popd
exit /b 0

:failure
echo.
echo Build stopped. Fix the error above, then run this batch file again.
popd
exit /b 1
