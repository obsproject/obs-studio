echo on 

set GPUPriority=1
set MAIN_DIR=%CD%

if defined ReleaseName (
    echo "ReleaseName is defined no need in default env variables"
) else (
    set ReleaseName=release
    set BuildConfig=RelWithDebInfo
    set CefBuildConfig=Release

    set InstallPath=packed_build
    set BUILD_DIRECTORY=build
)

call slobs_CI\win-install-dependency.cmd

cd "%MAIN_DIR%"

cmake -H. ^
         -B"%CD%\%BUILD_DIRECTORY%" ^
         -G"%CmakeGenerator%" -A x64 ^
         -DCMAKE_SYSTEM_VERSION="10.0.18363.657" ^
         -DCMAKE_INSTALL_PREFIX="%CD%\%InstallPath%" ^
         -DVLCPath="%VLC_DIR%" ^
         -DCEF_ROOT_DIR="%CEFPATH%" ^
         -DUSE_UI_LOOP=false ^
         -DENABLE_UI=false ^
         -DCOPIED_DEPENDENCIES=false ^
         -DCOPY_DEPENDENCIES=true ^
         -DENABLE_SCRIPTING=false ^
         -DGPU_PRIORITY_VAL="%GPUPriority%" ^
         -DBUILD_CAPTIONS=false ^
         -DCOMPILE_D3D12_HOOK=true ^
         -DBUILD_BROWSER=true ^
         -DENABLE_BROWSER=true ^
         -DENABLE_BROWSER_PANELS=false ^
         -DENABLE_BROWSER_QT_LOOP=false ^
         -DBROWSER_FRONTEND_API_SUPPORT=false ^
         -DBROWSER_PANEL_SUPPORT=false ^
         -DBROWSER_USE_STATIC_CRT=true ^
         -DEXPERIMENTAL_SHARED_TEXTURE_SUPPORT=true ^
         -DCHECK_FOR_SERVICE_UPDATES=true ^
         -DOPENSSL_ROOT_DIR=%OPENSSL_LOCAL_PATH% ^
         -DWEBRTC_INCLUDE_PATH=%WEBRTC_DIR% ^
         -DWEBRTC_LIB_PATH=%WEBRTC_DIR%/webrtc.lib ^
         -DMEDIASOUP_INCLUDE_PATH=%MEDIASOUPCLIENT_DIR%/include/mediasoupclient/ ^
         -DMEDIASOUP_LIB_PATH=%MEDIASOUPCLIENT_DIR%/lib/mediasoupclient.lib ^
         -DMEDIASOUP_SDP_LIB_PATH=%MEDIASOUPCLIENT_DIR%/lib/sdptransform.lib ^
         -DMEDIASOUP_SDP_INCLUDE_PATH=%MEDIASOUPCLIENT_DIR%/include/sdptransform ^
         -DProtobuf_DIR="%GRPC_DIST%\cmake" ^
         -Dabsl_DIR="%GRPC_DIST%\lib\cmake\absl" ^
         -DgRPC_DIR="%GRPC_DIST%\lib\cmake\grpc" ^
         -DCMAKE_PREFIX_PATH=%DEPS_DIR% ^
         -DCMAKE_BUILD_TYPE=%BuildConfig% ^
         -DBUILD_FOR_DISTRIBUTION=true ^
         -DCURL_INCLUDE_DIR=%DEPS_DIR%/ ^
         -DENABLE_VLC=true ^
         -DVIRTUALCAM_GUID="27B05C2D-93DC-474A-A5DA-9BBA34CB2A9C"

del /q /s %CD%\%InstallPath%
cmake --build %CD%\%BUILD_DIRECTORY% --target install --config %BuildConfig% -v
cmake -S . -B %CD%\%BUILD_DIRECTORY% -DCOPIED_DEPENDENCIES=OFF -DCOPY_DEPENDENCIES=ON
cmake --build %CD%\%BUILD_DIRECTORY% --target install --config %BuildConfig% -v

cmake --build %CD%\%BUILD_DIRECTORY% --target check_dependencies --config %BuildConfig% -v
if %errorlevel% neq 0 exit /b %errorlevel%