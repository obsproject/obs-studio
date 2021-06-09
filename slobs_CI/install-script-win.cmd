set DEPS=dependencies2019.0
set DepsURL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%DEPS%.zip
set VLCURL=https://obsproject.com/downloads/vlc.zip
set CEFURL=https://s3-us-west-2.amazonaws.com/streamlabs-cef-dist
set CMakeGenerator=Visual Studio 16 2019
set CefFileName=cef_binary_%CEF_VERSION%_windows64_minimal
set GPUPriority=1
set AMD_OLD=enc-amf_old
set AMD_URL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%AMD_OLD%.zip
set OBS_VIRTUALCAM=obs-virtualsource_32bit
set OBS_VIRTUALCAM_URL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%OBS_VIRTUALCAM%.zip

if exist %DEPS%.zip (curl -kLO %DepsURL% -f --retry 5 -z %DEPS%.zip) else (curl -kLO %DepsURL% -f --retry 5 -C -)
if exist vlc.zip (curl -kLO %VLCURL% -f --retry 5 -z vlc.zip) else (curl -kLO %VLCURL% -f --retry 5 -C -)
if exist %CefFileName%.zip (curl -kLO %CEFURL%/%CefFileName%.zip -f --retry 5 -z %CefFileName%.zip) else (curl -kLO %CEFURL%/%CefFileName%.zip -f --retry 5 -C -)
if exist %AMD_OLD%.zip (curl -kLO %AMD_URL% -f --retry 5 -z %AMD_OLD%.zip) else (curl -kLO %AMD_URL% -f --retry 5 -C -)
if exist %OBS_VIRTUALCAM%.zip (curl -kLO %OBS_VIRTUALCAM_URL% -f --retry 5 -z %OBS_VIRTUALCAM%.zip) else (curl -kLO %OBS_VIRTUALCAM_URL% -f --retry 5 -C -)

mkdir build

7z x %DEPS%.zip -o%DEPS%
7z x vlc.zip -ovlc
7z x %CefFileName%.zip -oCEF
7z x %AMD_OLD%.zip -o%AMD_OLD%
7z x %OBS_VIRTUALCAM%.zip -o%OBS_VIRTUALCAM%

set CEFPATH=%CD%\CEF\%CefFileName%

cmake -G"%CMakeGenerator%" -A x64 -H%CEFPATH% -B%CEFPATH%\build -DCEF_RUNTIME_LIBRARY_FLAG="/MD"
cmake --build %CEFPATH%\build --config %CefBuildConfig% --target libcef_dll_wrapper -v

cmake -H. ^
         -B%CD%\build ^
         -G"%CmakeGenerator%" ^
         -A x64 ^
         -DCMAKE_SYSTEM_VERSION=10.0 ^
         -DCMAKE_INSTALL_PREFIX=%CD%\%InstallPath% ^
         -DDepsPath=%CD%\%DEPS%\win64 ^
         -DVLCPath=%CD%\vlc ^
         -DCEF_ROOT_DIR=%CEFPATH% ^
         -DUSE_UI_LOOP=false ^
         -DENABLE_UI=false ^
         -DCOPIED_DEPENDENCIES=false ^
         -DCOPY_DEPENDENCIES=true ^
         -DENABLE_SCRIPTING=false ^
         -DGPU_PRIORITY_VAL="%GPUPriority%" ^
         -DBUILD_CAPTIONS=false ^
         -DCOMPILE_D3D12_HOOK=true ^
         -DBUILD_BROWSER=true ^
         -DBROWSER_FRONTEND_API_SUPPORT=false ^
         -DBROWSER_PANEL_SUPPORT=false ^
         -DBROWSER_USE_STATIC_CRT=false ^
         -DEXPERIMENTAL_SHARED_TEXTURE_SUPPORT=true ^
         -DCHECK_FOR_SERVICE_UPDATES=true

cmake --build %CD%\build --target install --config %BuildConfig% -v

move %CD%\%AMD_OLD% %CD%\%InstallPath%\%AMD_OLD%

mkdir %CD%\%InstallPath%\data\obs-plugins\obs-virtualoutput
move %CD%\%OBS_VIRTUALCAM% %CD%\%InstallPath%\data\obs-plugins\obs-virtualoutput\%OBS_VIRTUALCAM%