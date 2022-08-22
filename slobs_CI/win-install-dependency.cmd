

set WORK_DIR=%CD%
set SUBDIR=build\deps

set DepsURL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%DEPS_VERSION%.zip
set DEPS_DIR=%CD%\%SUBDIR%\deps_bin

set VLCURL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%VLC_VERSION%.zip
set VLC_DIR=%CD%\%SUBDIR%\vlc

set CEFURL=https://streamlabs-cef-dist.s3.us-west-2.amazonaws.com
set CefFileName=cef_binary_%CEF_VERSION%_windows_x64
set CEFPATH=%CD%\%SUBDIR%\%CefFileName%

set OBS_VIRTUALCAM=obs-virtualsource_32bit
set OBS_VIRTUALCAM_URL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%OBS_VIRTUALCAM%.zip

set GRPC_DIST=%CD%\%SUBDIR%\grpc_dist
set GRPC_FILE=grpc-%ReleaseName%-%GRPC_VERSION%.7z
set GRPC_URL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%GRPC_FILE%

set OPENSSL_DIST_NAME=openssl-1.1.1c-x64
set OPENSSL_LOCAL_PATH=%CD%\%SUBDIR%\openssl_dist
set OPENSSL_URI=https://s3-us-west-2.amazonaws.com/streamlabs-obs-updater-deps/%OPENSSL_DIST_NAME%.7z

set WEBRTC_DIST=webrtc_dist_m94_vs2022
set WEBRTC_DIR=%CD%\%SUBDIR%\webrtc_dist\webrtc_dist
set WEBRTC_URL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%WEBRTC_DIST%.7z

set MEDIASOUPCLIENT=libmediasoupclient_dist_8b36a915
set MEDIASOUPCLIENT_DIR=%CD%\%SUBDIR%\libmediasoupclient_dist\libmediasoupclient_dist
set MEDIASOUPCLIENT_URL=https://obs-studio-deployment.s3-us-west-2.amazonaws.com/%MEDIASOUPCLIENT%.7z

mkdir %SUBDIR%
cd %SUBDIR%

if exist libmediasoupclient_dist\ (
    echo "media soup client already installed"
) else (
    if exist %MEDIASOUPCLIENT%.7z (curl -kLO %MEDIASOUPCLIENT_URL% -f --retry 5 -z %MEDIASOUPCLIENT%.7z) else (curl -kLO %MEDIASOUPCLIENT_URL% -f --retry 5 -C -)
    7z x %MEDIASOUPCLIENT%.7z -aoa -olibmediasoupclient_dist
)

if exist webrtc_dist\ (
    echo "webrtc alredy installed"
) else (
    if exist %WEBRTC_DIST%.7z (curl -kLO %WEBRTC_URL% -f --retry 5 -z %WEBRTC_DIST%.7z) else (curl -kLO %WEBRTC_URL% -f --retry 5 -C -)
    7z x %WEBRTC_DIST%.7z -aoa -owebrtc_dist
)

if exist deps_bin\ (
    echo "OBS binary dependencies already installed"
) else (
    if exist %DEPS_VERSION%.zip (curl -kLO %DepsURL% -f --retry 5 -z %DEPS_VERSION%.zip) else (curl -kLO %DepsURL% -f --retry 5 -C -)
    7z x %DEPS_VERSION%.zip -aoa -odeps_bin
)

if exist grpc_dist\ (
    echo "grpc dependencie already installed"
) else (
    if exist %GRPC_FILE% (curl -kLO %GRPC_URL% -f --retry 5 -z GRPC_FILE) else (curl -kLO %GRPC_URL% -f --retry 5 -C -)
    7z x %GRPC_FILE% -aoa -ogrpc_dist
)

if exist vlc\ (
    echo "VLC already installed"
) else (
    if exist vlc.zip (curl -kLO %VLCURL% -f --retry 5 -z vlc.zip) else (curl -kLO %VLCURL% -f --retry 5 -C -)
    7z x vlc.zip -aoa -ovlc
)

if exist openssl_dist\ (
    echo "OPENSSL already installed"
) else (
    if exist %OPENSSL_DIST_NAME%.7z (curl -kLO %OPENSSL_URI% -f --retry 5 -z %OPENSSL_DIST_NAME%.7z) else (curl -kLO %OPENSSL_URI% -f --retry 5 -C -)
    7z x %OPENSSL_DIST_NAME%.7z -aoa -oopenssl_dist
)

if exist %OBS_VIRTUALCAM%\ (
    echo "virtual cam deps already installed"
) else (
    if exist %OBS_VIRTUALCAM%.zip (curl -kLO %OBS_VIRTUALCAM_URL% -f --retry 5 -z %OBS_VIRTUALCAM%.zip) else (curl -kLO %OBS_VIRTUALCAM_URL% -f --retry 5 -C -)
    7z x %OBS_VIRTUALCAM%.zip -aoa -o%OBS_VIRTUALCAM%
)

if exist CEF\ (
    echo "CEF already installed"
) else (
    if exist %CefFileName%.zip (curl -kLO %CEFURL%/%CefFileName%.zip -f --retry 5 -z %CefFileName%.zip) else (curl -kLO %CEFURL%/%CefFileName%.zip -f --retry 5 -C -)
    7z x %CefFileName%.zip -aoa -o%CefFileName%

    rmdir %CEFPATH%\build /s /q
    mkdir %CEFPATH%\build

    if "%CefBuildConfig%" == "Debug" (
        cmake -G"%CMakeGenerator%" -A x64 -H%CEFPATH% -B%CEFPATH%\build -DCEF_RUNTIME_LIBRARY_FLAG="/MD" -DUSE_SANDBOX=false
    ) else (
        cmake -G"%CMakeGenerator%" -A x64 -H%CEFPATH% -B%CEFPATH%\build -DCEF_RUNTIME_LIBRARY_FLAG="/MD"
    )

    cmake --build %CEFPATH%\build --config %CefBuildConfig% --target libcef_dll_wrapper -v
)

cd "%WORK_DIR%"