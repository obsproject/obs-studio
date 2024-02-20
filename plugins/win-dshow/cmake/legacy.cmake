if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/libdshowcapture/dshowcapture.hpp")
  obs_status(FATAL_ERROR "libdshowcapture submodule not found! Please fetch submodules. win-dshow plugin disabled.")
  return()
endif()

option(ENABLE_VIRTUALCAM "Enable building with Virtual Camera (Windows)" ON)

if(NOT ENABLE_VIRTUALCAM)
  obs_status(DISABLED "Windows Virtual Camera")
endif()

if(ENABLE_VIRTUALCAM AND NOT VIRTUALCAM_GUID)
  set(VIRTUALCAM_GUID
      ""
      CACHE STRING "Virtual Camera GUID" FORCE)
  mark_as_advanced(VIRTUALCAM_GUID)
endif()

project(win-dshow)

find_package(FFmpeg REQUIRED COMPONENTS avcodec avutil)

add_library(win-dshow MODULE)
add_library(OBS::dshow ALIAS win-dshow)

target_sources(win-dshow PRIVATE encode-dstr.hpp win-dshow.cpp win-dshow-encoder.cpp dshow-plugin.cpp ffmpeg-decode.c
                                 ffmpeg-decode.h)

add_library(libdshowcapture-external INTERFACE)
add_library(libdshowcapture INTERFACE)
add_library(OBS::libdshowcapture-external ALIAS libdshowcapture-external)
add_library(OBS::libdshowcapture ALIAS libdshowcapture)

target_sources(
  libdshowcapture-external
  INTERFACE libdshowcapture/external/capture-device-support/Library/EGAVResult.cpp
            libdshowcapture/external/capture-device-support/Library/ElgatoUVCDevice.cpp
            libdshowcapture/external/capture-device-support/Library/win/EGAVHIDImplementation.cpp
            libdshowcapture/external/capture-device-support/SampleCode/DriverInterface.cpp)

target_sources(
  libdshowcapture
  INTERFACE libdshowcapture/dshowcapture.hpp
            libdshowcapture/source/capture-filter.cpp
            libdshowcapture/source/capture-filter.hpp
            libdshowcapture/source/output-filter.cpp
            libdshowcapture/source/output-filter.hpp
            libdshowcapture/source/dshowcapture.cpp
            libdshowcapture/source/dshowencode.cpp
            libdshowcapture/source/device.cpp
            libdshowcapture/source/device.hpp
            libdshowcapture/source/device-vendor.cpp
            libdshowcapture/source/encoder.cpp
            libdshowcapture/source/encoder.hpp
            libdshowcapture/source/dshow-base.cpp
            libdshowcapture/source/dshow-base.hpp
            libdshowcapture/source/dshow-demux.cpp
            libdshowcapture/source/dshow-demux.hpp
            libdshowcapture/source/dshow-device-defs.hpp
            libdshowcapture/source/dshow-enum.cpp
            libdshowcapture/source/dshow-enum.hpp
            libdshowcapture/source/dshow-formats.cpp
            libdshowcapture/source/dshow-formats.hpp
            libdshowcapture/source/dshow-media-type.cpp
            libdshowcapture/source/dshow-encoded-device.cpp
            libdshowcapture/source/dshow-media-type.hpp
            libdshowcapture/source/log.cpp
            libdshowcapture/source/log.hpp
            libdshowcapture/source/external/IVideoCaptureFilter.h)

target_include_directories(
  libdshowcapture INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/libdshowcapture
                            ${CMAKE_CURRENT_SOURCE_DIR}/libdshowcapture/external/capture-device-support/Library)

target_compile_definitions(libdshowcapture-external INTERFACE _UP_WINDOWS=1)
target_compile_definitions(libdshowcapture INTERFACE _UP_WINDOWS=1)
target_compile_options(libdshowcapture-external INTERFACE /wd4018)

set(MODULE_DESCRIPTION "OBS DirectShow module")

configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in win-dshow.rc)

target_sources(win-dshow PRIVATE win-dshow.rc)

target_compile_definitions(win-dshow PRIVATE UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS
                                             OBS_LEGACY)

set(VIRTUALCAM_AVAILABLE OFF)
if(ENABLE_VIRTUALCAM)
  if(VIRTUALCAM_GUID STREQUAL "")
    obs_status(WARNING "Windows Virtual Camera - GUID not set. Specify as 'VIRTUALCAM_GUID' to enable.")
  else()
    set(INVALID_GUID ON)

    string(REPLACE "-" ";" GUID_VALS ${VIRTUALCAM_GUID})

    list(LENGTH GUID_VALS GUID_VAL_COUNT)
    if(GUID_VAL_COUNT EQUAL 5)
      string(REPLACE ";" "0" GUID_HEX ${GUID_VALS})
      string(REGEX MATCH "[0-9a-fA-F]+" GUID_ACTUAL_HEX ${GUID_HEX})
      if(GUID_ACTUAL_HEX STREQUAL GUID_HEX)
        list(GET GUID_VALS 0 GUID_VALS_DATA1)
        list(GET GUID_VALS 1 GUID_VALS_DATA2)
        list(GET GUID_VALS 2 GUID_VALS_DATA3)
        list(GET GUID_VALS 3 GUID_VALS_DATA4)
        list(GET GUID_VALS 4 GUID_VALS_DATA5)
        string(LENGTH ${GUID_VALS_DATA1} GUID_VALS_DATA1_LENGTH)
        string(LENGTH ${GUID_VALS_DATA2} GUID_VALS_DATA2_LENGTH)
        string(LENGTH ${GUID_VALS_DATA3} GUID_VALS_DATA3_LENGTH)
        string(LENGTH ${GUID_VALS_DATA4} GUID_VALS_DATA4_LENGTH)
        string(LENGTH ${GUID_VALS_DATA5} GUID_VALS_DATA5_LENGTH)
        if(GUID_VALS_DATA1_LENGTH EQUAL 8
           AND GUID_VALS_DATA2_LENGTH EQUAL 4
           AND GUID_VALS_DATA3_LENGTH EQUAL 4
           AND GUID_VALS_DATA4_LENGTH EQUAL 4
           AND GUID_VALS_DATA5_LENGTH EQUAL 12)
          set(GUID_VAL01 ${GUID_VALS_DATA1})
          set(GUID_VAL02 ${GUID_VALS_DATA2})
          set(GUID_VAL03 ${GUID_VALS_DATA3})
          string(SUBSTRING ${GUID_VALS_DATA4} 0 2 GUID_VAL04)
          string(SUBSTRING ${GUID_VALS_DATA4} 2 2 GUID_VAL05)
          string(SUBSTRING ${GUID_VALS_DATA5} 0 2 GUID_VAL06)
          string(SUBSTRING ${GUID_VALS_DATA5} 2 2 GUID_VAL07)
          string(SUBSTRING ${GUID_VALS_DATA5} 4 2 GUID_VAL08)
          string(SUBSTRING ${GUID_VALS_DATA5} 6 2 GUID_VAL09)
          string(SUBSTRING ${GUID_VALS_DATA5} 8 2 GUID_VAL10)
          string(SUBSTRING ${GUID_VALS_DATA5} 10 2 GUID_VAL11)
          set(VIRTUALCAM_AVAILABLE ON)
          set(INVALID_GUID OFF)
        endif()
      endif()
    endif()
  endif()

  if(INVALID_GUID)
    obs_status(WARNING "Windows Virtual Camera - invalid GUID supplied")
  endif()
endif()

target_link_libraries(
  win-dshow
  PRIVATE OBS::libobs
          OBS::w32-pthreads
          OBS::libdshowcapture-external
          OBS::libdshowcapture
          setupapi
          strmiids
          ksuser
          winmm
          wmcodecdspuuid
          FFmpeg::avcodec
          FFmpeg::avutil)

source_group(
  "libdshowcapture-external\\Source Files"
  FILES libdshowcapture/external/capture-device-support/Library/EGAVResult.cpp
        libdshowcapture/external/capture-device-support/Library/ElgatoUVCDevice.cpp
        libdshowcapture/external/capture-device-support/Library/win/EGAVHIDImplementation.cpp
        libdshowcapture/external/capture-device-support/SampleCode/DriverInterface.cpp)
source_group(
  "libdshowcapture\\Source Files"
  FILES libdshowcapture/source/capture-filter.cpp
        libdshowcapture/source/output-filter.cpp
        libdshowcapture/source/dshowcapture.cpp
        libdshowcapture/source/dshowencode.cpp
        libdshowcapture/source/device.cpp
        libdshowcapture/source/device-vendor.cpp
        libdshowcapture/source/encoder.cpp
        libdshowcapture/source/dshow-base.cpp
        libdshowcapture/source/dshow-demux.cpp
        libdshowcapture/source/dshow-enum.cpp
        libdshowcapture/source/dshow-formats.cpp
        libdshowcapture/source/dshow-media-type.cpp
        libdshowcapture/source/dshow-encoded-device.cpp
        libdshowcapture/source/log.cpp)
source_group(
  "libdshowcapture\\Header Files"
  FILES libdshowcapture/dshowcapture.hpp
        libdshowcapture/source/capture-filter.hpp
        libdshowcapture/source/output-filter.hpp
        libdshowcapture/source/device.hpp
        libdshowcapture/source/encoder.hpp
        libdshowcapture/source/dshow-base.hpp
        libdshowcapture/source/dshow-demux.hpp
        libdshowcapture/source/dshow-device-defs.hpp
        libdshowcapture/source/dshow-enum.hpp
        libdshowcapture/source/dshow-formats.hpp
        libdshowcapture/source/dshow-media-type.hpp
        libdshowcapture/source/log.hpp
        libdshowcapture/source/external/IVideoCaptureFilter.h)

set_target_properties(win-dshow PROPERTIES FOLDER "plugins/win-dshow")

setup_plugin_target(win-dshow)

if(ENABLE_VIRTUALCAM AND VIRTUALCAM_AVAILABLE)
  target_sources(win-dshow PRIVATE tiny-nv12-scale.c tiny-nv12-scale.h shared-memory-queue.c shared-memory-queue.h
                                   virtualcam.c)

  target_compile_definitions(win-dshow PRIVATE VIRTUALCAM_AVAILABLE)

  target_include_directories(win-dshow PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/config)

  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/virtualcam-module/virtualcam-guid.h.in
                 ${CMAKE_CURRENT_BINARY_DIR}/config/virtualcam-guid.h)

  target_sources(win-dshow PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/config/virtualcam-guid.h)

  configure_file(virtualcam-module/virtualcam-install.bat.in "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-install.bat")

  configure_file(virtualcam-module/virtualcam-uninstall.bat.in "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-uninstall.bat")

  add_target_resource(win-dshow "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-install.bat" "obs-plugins/win-dshow/")
  add_target_resource(win-dshow "${CMAKE_CURRENT_BINARY_DIR}/virtualcam-uninstall.bat" "obs-plugins/win-dshow/")

  add_subdirectory(virtualcam-module)
endif()
