cmake_minimum_required(VERSION 3.28...3.30)

find_package(FFmpeg REQUIRED avcodec avutil)
find_package(WIL REQUIRED)

add_library(win-mediafoundation-capture MODULE)
add_library(OBS::mf ALIAS win-mediafoundation-capture)

target_sources(win-mediafoundation-capture PUBLIC mf-plugin.cpp win-mf.cpp)

add_library(libmfcapture INTERFACE)
add_library(OBS::libmfcapture ALIAS libmfcapture)

target_include_directories(libmfcapture INTERFACE libmfcapture libmfcapture/source)

target_sources(
  libmfcapture
  INTERFACE
    libmfcapture/source/CriticalSection.cpp
    libmfcapture/source/DeviceControlChangeListener.cpp
    libmfcapture/source/MediaFoundationSourceInput.cpp
    libmfcapture/source/mfcapture.cpp
    libmfcapture/source/PhysicalCamera.cpp
    libmfcapture/source/VideoBufferLock.cpp
)

configure_file(cmake/windows/obs-module.rc.in win-mf.rc)
configure_file(libmfcapture/source/mfcapture.rc.in win-mf.rc)
target_sources(win-mediafoundation-capture PRIVATE win-mf.rc)

target_precompile_headers(win-mediafoundation-capture PRIVATE <wil/cppwinrt.h> <wil/result.h> <wil/com.h>)

target_link_libraries(
  win-mediafoundation-capture
  PRIVATE
    OBS::libobs
    OBS::w32-pthreads
    OBS::winrt-headers
    FFmpeg::avcodec
    FFmpeg::avutil
    strmiids
    winmm
    dxcore
    dxguid
    libmfcapture
)

set_target_properties_obs(win-mediafoundation-capture PROPERTIES FOLDER plugins/win-mediafoundation/capture PREFIX "")
