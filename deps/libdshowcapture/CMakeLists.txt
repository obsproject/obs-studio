add_library(libdshowcapture INTERFACE)
add_library(OBS::libdshowcapture ALIAS libdshowcapture)

target_sources(
  libdshowcapture
  INTERFACE src/dshowcapture.hpp
            src/source/capture-filter.cpp
            src/source/capture-filter.hpp
            src/source/device-vendor.cpp
            src/source/device.cpp
            src/source/device.hpp
            src/source/dshow-base.cpp
            src/source/dshow-base.hpp
            src/source/dshow-demux.cpp
            src/source/dshow-demux.hpp
            src/source/dshow-device-defs.hpp
            src/source/dshow-encoded-device.cpp
            src/source/dshow-enum.cpp
            src/source/dshow-enum.hpp
            src/source/dshow-formats.cpp
            src/source/dshow-formats.hpp
            src/source/dshow-media-type.cpp
            src/source/dshow-media-type.hpp
            src/source/dshowcapture.cpp
            src/source/dshowencode.cpp
            src/source/encoder.cpp
            src/source/encoder.hpp
            src/source/external/IVideoCaptureFilter.h
            src/source/log.cpp
            src/source/log.hpp
            src/source/output-filter.cpp
            src/source/output-filter.hpp
            src/external/capture-device-support/Library/EGAVResult.cpp
            src/external/capture-device-support/Library/ElgatoUVCDevice.cpp
            src/external/capture-device-support/Library/win/EGAVHIDImplementation.cpp
            src/external/capture-device-support/SampleCode/DriverInterface.cpp)

target_include_directories(
  libdshowcapture INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/src"
                            "${CMAKE_CURRENT_SOURCE_DIR}/src/external/capture-device-support/Library")

target_compile_definitions(libdshowcapture INTERFACE _UP_WINDOWS=1)
target_compile_options(libdshowcapture INTERFACE /wd4018)
