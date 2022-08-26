project(obs-webrtc)

include(FetchContent)

FetchContent_Declare(
  Corrosion
  GIT_REPOSITORY https://github.com/AndrewGaspar/corrosion.git
  GIT_TAG origin/master # Optionally specify a version tag or branch here
)

FetchContent_MakeAvailable(Corrosion)

# find_package(Libcurl REQUIRED)

include_directories(${LIBCURL_INCLUDE_DIRS})

include_directories(${OBS_JANSSON_INCLUDE_DIRS})

option(ENABLE_FFMPEG_LOGGING "Enables obs-ffmpeg logging" OFF)

find_package(
  FFmpeg REQUIRED
  COMPONENTS avcodec
             avfilter
             avdevice
             avutil
             swscale
             avformat
             swresample)
include_directories(${FFMPEG_INCLUDE_DIRS})

set(obs-webrtc_HEADERS webrtc-stream.h webrtc-source.h webrtc-call.h)
set(obs-webrtc_SOURCES whip.c webrtc-stream.c webrtc-source.c webrtc.c
                       webrtc-call.c)

if(WIN32)
  set(MODULE_DESCRIPTION "OBS webrtc module")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/bundle/windows/obs-module.rc.in
                 obs-webrtc.rc)
  list(APPEND obs-webrtc_SOURCES obs-webrtc.rc)
endif()

add_library(obs-webrtc MODULE ${obs-webrtc_SOURCES} ${obs-webrtc_HEADERS})

corrosion_import_crate(MANIFEST_PATH webrtc/Cargo.toml)

target_link_libraries(obs-webrtc libobs webrtc-rs-lib ${OBS_JANSSON_IMPORT}
                      ${LIBCURL_LIBRARIES} ${FFMPEG_LIBRARIES})
set_target_properties(obs-webrtc PROPERTIES FOLDER "plugins")

setup_plugin_target(obs-webrtc data)
