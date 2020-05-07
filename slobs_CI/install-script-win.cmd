set DEPS=dependencies2017.2
set DepsURL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%DEPS%.zip
set VLCURL=https://obsproject.com/downloads/vlc.zip
set CEFURL=https://s3-us-west-2.amazonaws.com/streamlabs-cef-dist
set CMakeGenerator=Visual Studio 16 2019
set CefFileName=cef_binary_%CEF_VERSION%_windows64_minimal
set GPUPriority=1

if exist %DEPS%.zip (curl -kLO %DepsURL% -f --retry 5 -z %DEPS%.zip) else (curl -kLO %DepsURL% -f --retry 5 -C -)
if exist vlc.zip (curl -kLO %VLCURL% -f --retry 5 -z vlc.zip) else (curl -kLO %VLCURL% -f --retry 5 -C -)
if exist %CefFileName%.zip (curl -kLO %CEFURL%/%CefFileName%.zip -f --retry 5 -z %CefFileName%.zip) else (curl -kLO %CEFURL%/%CefFileName%.zip -f --retry 5 -C -)

mkdir build

7z x %DEPS%.zip -o%DEPS%
7z x vlc.zip -ovlc
7z x %CefFileName%.zip -oCEF

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
         -DEXPERIMENTAL_SHARED_TEXTURE_SUPPORT=true

cmake --build %CD%\build --target install --config %BuildConfig% -v