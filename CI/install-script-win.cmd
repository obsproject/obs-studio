if exist dependencies2017.zip (curl -kLO https://obsproject.com/downloads/dependencies2017.zip -f --retry 5 -z dependencies2017.zip) else (curl -kLO https://obsproject.com/downloads/dependencies2017.zip -f --retry 5 -C -)
if exist vlc.zip (curl -kLO https://obsproject.com/downloads/vlc.zip -f --retry 5 -z vlc.zip) else (curl -kLO https://obsproject.com/downloads/vlc.zip -f --retry 5 -C -)
if exist cef_binary_%CEF_VERSION%_windows32.zip (curl -kLO https://obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows32.zip -f --retry 5 -z cef_binary_%CEF_VERSION%_windows32.zip) else (curl -kLO https://obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows32.zip -f --retry 5 -C -)
if exist cef_binary_%CEF_VERSION%_windows64.zip (curl -kLO https://obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64.zip -f --retry 5 -z cef_binary_%CEF_VERSION%_windows64.zip) else (curl -kLO https://obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows64.zip -f --retry 5 -C -)
7z x dependencies2017.zip -odependencies2017
7z x vlc.zip -ovlc
7z x cef_binary_%CEF_VERSION%_windows32.zip -oCEF_32
7z x cef_binary_%CEF_VERSION%_windows64.zip -oCEF_64
set DepsPath32=%CD%\dependencies2017\win32
set DepsPath64=%CD%\dependencies2017\win64
set VLCPath=%CD%\vlc
set QTDIR32=C:\Qt\5.11\msvc2015
set QTDIR64=C:\Qt\5.11\msvc2017_64
set CEF_32=%CD%\CEF_32\cef_binary_%CEF_VERSION%_windows32
set CEF_64=%CD%\CEF_64\cef_binary_%CEF_VERSION%_windows64
set build_config=RelWithDebInfo
mkdir build build32 build64
cd ./build32
cmake -G "Visual Studio 15 2017" -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_32% -DTWITCH_CLIENTID="%TWITCH-CLIENTID%" -DTWITCH_HASH="%TWITCH-HASH%" -DMIXER_CLIENTID="%MIXER-CLIENTID%" -DMIXER_HASH="%MIXER-HASH%" ..
cd ../build64
cmake -G "Visual Studio 15 2017 Win64" -DCOPIED_DEPENDENCIES=false -DCOPY_DEPENDENCIES=true -DBUILD_CAPTIONS=true -DCOMPILE_D3D12_HOOK=true -DBUILD_BROWSER=true -DCEF_ROOT_DIR=%CEF_64% -DTWITCH_CLIENTID="%TWITCH-CLIENTID%" -DTWITCH_HASH="%TWITCH-HASH%" -DMIXER_CLIENTID="%MIXER-CLIENTID%" -DMIXER_HASH="%MIXER-HASH%" ..
cd ..
