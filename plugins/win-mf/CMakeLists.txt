cmake_minimum_required(VERSION 3.24...3.25)

legacy_check()

find_package(FFmpeg REQUIRED avcodec avutil)
find_package(WIL REQUIRED)

add_library(win-mf MODULE)
add_library(OBS::mf ALIAS win-mf)

target_sources(win-mf PUBLIC mf-plugin.cpp win-mf.cpp)

add_library(libmfcapture INTERFACE)
add_library(OBS::libmfcapture ALIAS libmfcapture)

target_include_directories(libmfcapture INTERFACE libmfcapture libmfcapture/source)

target_sources(
  libmfcapture
  INTERFACE
    libmfcapture/source/DeviceControlChangeListener.cpp
    libmfcapture/source/mfcapture.cpp
    libmfcapture/source/PhysicalCamera.cpp
)

configure_file(cmake/windows/obs-module.rc.in win-mf.rc)
target_sources(win-mf PRIVATE win-mf.rc)

target_precompile_headers(win-mf PRIVATE <wil/cppwinrt.h> <wil/result.h> <wil/com.h>)

target_link_libraries(
  win-mf
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

# cmake-format: off
set_target_properties_obs(win-mf PROPERTIES FOLDER plugins/win-mf PREFIX "")
# cmake-format: on
