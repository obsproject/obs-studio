add_library(libdshowcapture INTERFACE)
add_library(OBS::libdshowcapture ALIAS libdshowcapture)

target_sources(
  libdshowcapture
  INTERFACE libdshowcapture/dshowcapture.hpp
            libdshowcapture/source/capture-filter.cpp
            libdshowcapture/source/capture-filter.hpp
            libdshowcapture/source/device-vendor.cpp
            libdshowcapture/source/device.cpp
            libdshowcapture/source/device.hpp
            libdshowcapture/source/dshow-base.cpp
            libdshowcapture/source/dshow-base.hpp
            libdshowcapture/source/dshow-demux.cpp
            libdshowcapture/source/dshow-demux.hpp
            libdshowcapture/source/dshow-device-defs.hpp
            libdshowcapture/source/dshow-encoded-device.cpp
            libdshowcapture/source/dshow-enum.cpp
            libdshowcapture/source/dshow-enum.hpp
            libdshowcapture/source/dshow-formats.cpp
            libdshowcapture/source/dshow-formats.hpp
            libdshowcapture/source/dshow-media-type.cpp
            libdshowcapture/source/dshow-media-type.hpp
            libdshowcapture/source/dshowcapture.cpp
            libdshowcapture/source/dshowencode.cpp
            libdshowcapture/source/encoder.cpp
            libdshowcapture/source/encoder.hpp
            libdshowcapture/source/external/IVideoCaptureFilter.h
            libdshowcapture/source/log.cpp
            libdshowcapture/source/log.hpp
            libdshowcapture/source/output-filter.cpp
            libdshowcapture/source/output-filter.hpp
            libdshowcapture/external/capture-device-support/Library/EGAVResult.cpp
            libdshowcapture/external/capture-device-support/Library/ElgatoUVCDevice.cpp
            libdshowcapture/external/capture-device-support/Library/win/EGAVHIDImplementation.cpp
            libdshowcapture/external/capture-device-support/SampleCode/DriverInterface.cpp)

target_include_directories(
  libdshowcapture INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/libdshowcapture"
                            "${CMAKE_CURRENT_SOURCE_DIR}/libdshowcapture/external/capture-device-support/Library")

target_compile_definitions(libdshowcapture INTERFACE _UP_WINDOWS=1)
target_compile_options(libdshowcapture INTERFACE /wd4018)

get_target_property(target_sources libdshowcapture INTERFACE_SOURCES)
set(target_headers ${target_sources})
set(target_external_sources ${target_sources})

list(FILTER target_external_sources INCLUDE REGEX ".+external/.+/.+\\.cpp")
list(FILTER target_sources EXCLUDE REGEX ".+external/.+/.+\\.cpp")
list(FILTER target_sources INCLUDE REGEX ".*\\.(m|c[cp]?p?|swift)")
list(FILTER target_headers INCLUDE REGEX ".*\\.h(pp)?")

source_group("libdshowcapture-external\\Source Files" FILES ${target_external_sources})
source_group("libdshowcapture\\Source Files" FILES ${target_sources})
source_group("libdshowcapture\\Header Files" FILES ${target_headers})
