@echo off
setlocal EnableExtensions DisableDelayedExpansion
set "NOVA_CHECK="
set "NOVA_NO_PAUSE="
:arguments
if "%~1"=="" goto run
if /i "%~1"=="--check" (
    set "NOVA_CHECK=-Check"
    shift
    goto arguments
)
if /i "%~1"=="--no-pause" (
    set "NOVA_NO_PAUSE=1"
    shift
    goto arguments
)
echo Usage: build-windows.bat [--check] [--no-pause]
set "NOVA_RESULT=2"
goto finish

:run
echo OBS Nova Windows build
echo A build log will be saved beside this batch file as build-windows.log.
echo.
if not exist "%~dp0scripts\Build-Nova.ps1" (
    echo ERROR: scripts\Build-Nova.ps1 is missing. Download or pull the entire repository.
    set "NOVA_RESULT=1"
    goto finish
)
"%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe" -NoLogo -NoProfile -ExecutionPolicy Bypass -File "%~dp0scripts\Build-Nova.ps1" %NOVA_CHECK%
set "NOVA_RESULT=%ERRORLEVEL%"

:finish
echo.
if not "%NOVA_RESULT%"=="0" echo Build stopped. Read the error above or open build-windows.log.
if not defined NOVA_NO_PAUSE pause
exit /b %NOVA_RESULT%
