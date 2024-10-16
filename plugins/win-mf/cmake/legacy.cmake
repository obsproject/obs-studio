project(win-mf)

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)
find_package(WIL REQUIRED)

add_library(win-mf MODULE)
add_library(OBS::mf ALIAS win-mf)

target_sources(win-mf PRIVATE mf-plugin.cpp plugin-macros.generated.h win-mf.cpp)

add_library(libmfcapture INTERFACE)
add_library(OBS::libmfcapture ALIAS libmfcapture)

target_sources(
  libmfcapture
  INTERFACE libmfcapture/mfcapture.hpp
            libmfcapture/source/DeviceControlChangeListener.cpp
            libmfcapture/source/DeviceControlChangeListener.hpp
            libmfcapture/source/mfcapture.cpp
            libmfcapture/source/framework.hpp
            libmfcapture/source/mfcapture.rc
            libmfcapture/source/PhysicalCamera.cpp
            libmfcapture/source/PhysicalCamera.hpp
            libmfcapture/source/resource.hpp)

target_include_directories(libmfcapture INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libmfcapture)

target_compile_definitions(libmfcapture INTERFACE _UP_WINDOWS=1)

set(MODULE_DESCRIPTION "OBS MediaFoundation module")

configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in win-mf.rc)

target_sources(win-mf PRIVATE win-mf.rc)

target_compile_definitions(win-mf PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS OBS_LEGACY)

target_link_libraries(
  win-mf
  PRIVATE OBS::libobs
          OBS::w32-pthreads
          setupapi
          strmiids
          ksuser
          winmm
          wmcodecdspuuid
          FFmpeg::avcodec
          FFmpeg::avutil
          dxcore
          dxguid
          libmfcapture)

source_group(
  "libmfcapture\\Source Files"
  FILES libmfcapture/source/DeviceControlChangeListener.cpp libmfcapture/source/mfcapture.cpp
        libmfcapture/source/mfcapture.rc libmfcapture/source/PhysicalCamera.cpp)
source_group(
  "libmfcapture\\Header Files"
  FILES libmfcapture/source/DeviceControlChangeListener.hpp libmfcapture/source/framework.hpp
        libmfcapture/source/mfcapture.hpp libmfcapture/source/PhysicalCamera.hpp libmfcapture/source/resource.hpp)

set_target_properties(win-mf PROPERTIES FOLDER "plugins/win-mf")

setup_plugin_target(win-mf)
