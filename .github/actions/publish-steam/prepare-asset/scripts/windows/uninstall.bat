@echo off
@cd /d "%~dp0"
goto checkAdmin

:checkAdmin
	net session >nul 2>&1
	if %errorLevel% == 0 (
		echo.
	) else (
		echo Administrative rights are required. Please re-run this script as Administrator.
		goto end
	)

:clearRegistry
	reg delete "HKLM\SOFTWARE\OBS Studio" /f /reg:32
	reg delete "HKLM\SOFTWARE\OBS Studio" /f /reg:64
	:: Vulkan layer keys
	reg delete "HKLM\SOFTWARE\Khronos\Vulkan\ImplicitLayers" /f /v "%PROGRAMDATA%\obs-studio-hook\obs-vulkan64.json" /reg:32
	reg delete "HKLM\SOFTWARE\Khronos\Vulkan\ImplicitLayers" /f /v "%PROGRAMDATA%\obs-studio-hook\obs-vulkan32.json" /reg:64

:deleteProgramDataFolder
	RMDIR /S /Q "%PROGRAMDATA%\obs-studio-hook"
	RMDIR /S /Q "%PROGRAMDATA%\obs-studio\shader-cache"

:uninstallDLLs
	regsvr32.exe /u /s %1\data\obs-plugins\win-dshow\obs-virtualcam-module32.dll
	regsvr32.exe /u /s %1\data\obs-plugins\win-dshow\obs-virtualcam-module64.dll

:endSuccess
	echo Virtual Cam uninstalled!
	echo.

:end
	exit
