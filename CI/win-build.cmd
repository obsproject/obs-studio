if not exist "dependencies2017.zip" appveyor DownloadFile "%DependenciesUrl%"
if not exist "%CefZip%" appveyor DownloadFile "%CefUrl%" -FileName "%CefZip%"
7z x "%CefZip%"
7z x dependencies2017.zip -odependencies2017

cmake ^
	-G"%CMakeGenerator%" ^
	-A x64 ^
	-H"%CefPath%" ^
	-B"%CefBuildPath%" ^
	-DCEF_RUNTIME_LIBRARY_FLAG="/MD"

cmake ^
	--build "%CefBuildPath%" ^
	--config %CefBuildConfig% ^
	--target libcef_dll_wrapper ^
	-- /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"

cmake ^
	-H. ^
	-B"%BuildPath64%" ^
	-G"%CmakeGenerator%" ^
	-A x64 ^
	-DCMAKE_INSTALL_PREFIX="%InstallPath%" ^
	-DDepsPath="%DepsPath64%" ^
	-DCEF_ROOT_DIR="%CefPath%" ^
	-DENABLE_UI=false ^
	-DCOPIED_DEPENDENCIES=false ^
	-DCOPY_DEPENDENCIES=true ^
	-DENABLE_SCRIPTING=false ^
	-DBUILD_CAPTIONS=false ^
	-DCOMPILE_D3D12_HOOK=true ^
	-DBUILD_BROWSER=true ^
	-DBROWSER_FRONTEND_API_SUPPORT=false ^
	-DBROWSER_PANEL_SUPPORT=false ^
	-DBROWSER_USE_STATIC_CRT=false ^
	-DEXPERIMENTAL_SHARED_TEXTURE_SUPPORT=false

cmake ^
	--build "%BuildPath64%" ^
	--target install ^
	--config %BuildConfig% ^
	-- /logger:"C:\Program Files\AppVeyor\BuildAgent\Appveyor.MSBuildLogger.dll"