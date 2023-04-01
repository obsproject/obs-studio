cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

add_library(libobs SHARED)
add_library(OBS::libobs ALIAS libobs)

find_package(Threads REQUIRED)
find_package(FFmpeg REQUIRED avformat avutil swscale swresample OPTIONAL_COMPONENTS avcodec)
find_package(ZLIB REQUIRED)

if(ENABLE_UI)
  find_qt(COMPONENTS Core)
endif()

find_package(jansson REQUIRED)
add_subdirectory("${CMAKE_SOURCE_DIR}/deps/libcaption" "${CMAKE_BINARY_DIR}/deps/libcaption")
add_subdirectory("${CMAKE_SOURCE_DIR}/deps/uthash" "${CMAKE_BINARY_DIR}/deps/uthash")

target_sources(
  libobs
  PRIVATE obs.c
          obs.h
          obs.hpp
          obs-audio.c
          obs-audio-controls.c
          obs-audio-controls.h
          obs-avc.c
          obs-avc.h
          obs-data.c
          obs-data.h
          obs-defs.h
          obs-display.c
          obs-encoder.c
          obs-encoder.h
          obs-ffmpeg-compat.h
          obs-hotkey.c
          obs-hotkey.h
          obs-hotkeys.h
          obs-missing-files.c
          obs-missing-files.h
          obs-nal.c
          obs-nal.h
          obs-hotkey-name-map.c
          obs-interaction.h
          obs-internal.h
          obs-module.c
          obs-module.h
          obs-output.c
          obs-output.h
          obs-output-delay.c
          obs-properties.c
          obs-properties.h
          obs-service.c
          obs-service.h
          obs-scene.c
          obs-scene.h
          obs-source.c
          obs-source.h
          obs-source-deinterlace.c
          obs-source-transition.c
          obs-ui.h
          obs-video.c
          obs-video-gpu-encode.c
          obs-view.c
          $<$<BOOL:${ENABLE_HEVC}>:obs-hevc.c>
          $<$<BOOL:${ENABLE_HEVC}>:obs-hevc.h>)

target_sources(
  libobs
  PRIVATE util/array-serializer.c
          util/array-serializer.h
          util/base.c
          util/base.h
          util/bitstream.c
          util/bitstream.h
          util/bmem.h
          util/bmem.c
          util/c99defs.h
          util/cf-lexer.c
          util/cf-lexer.h
          util/cf-parser.c
          util/cf-parser.h
          util/circlebuf.h
          util/config-file.c
          util/config-file.h
          util/crc32.c
          util/crc32.h
          util/darray.h
          util/dstr.c
          util/dstr.h
          util/file-serializer.c
          util/file-serializer.h
          util/lexer.c
          util/lexer.h
          util/pipe.h
          util/platform.c
          util/platform.h
          util/profiler.c
          util/profiler.h
          util/profiler.hpp
          util/serializer.h
          util/sse-intrin.h
          util/task.c
          util/task.h
          util/text-lookup.c
          util/text-lookup.h
          util/threading.h
          util/utf8.c
          util/utf8.h
          util/util.hpp
          util/uthash.h
          util/util_uint64.h
          util/util_uint128.h
          util/curl/curl-helper.h)

target_sources(
  libobs
  PRIVATE util/simde/check.h
          util/simde/debug-trap.h
          util/simde/hedley.h
          util/simde/simde-align.h
          util/simde/simde-arch.h
          util/simde/simde-common.h
          util/simde/simde-constify.h
          util/simde/simde-detect-clang.h
          util/simde/simde-diagnostic.h
          util/simde/simde-features.h
          util/simde/simde-math.h
          util/simde/x86/mmx.h
          util/simde/x86/sse2.h
          util/simde/x86/sse.h)

target_sources(
  libobs
  PRIVATE callback/calldata.c
          callback/calldata.h
          callback/decl.c
          callback/decl.h
          callback/signal.c
          callback/signal.h
          callback/proc.c
          callback/proc.h)

target_sources(
  libobs
  PRIVATE media-io/audio-io.c
          media-io/audio-io.h
          media-io/audio-math.h
          media-io/audio-resampler.h
          media-io/audio-resampler-ffmpeg.c
          media-io/format-conversion.c
          media-io/format-conversion.h
          media-io/frame-rate.h
          media-io/media-remux.c
          media-io/media-remux.h
          media-io/video-fourcc.c
          media-io/video-frame.c
          media-io/video-frame.h
          media-io/video-io.c
          media-io/video-io.h
          media-io/media-io-defs.h
          media-io/video-matrices.c
          media-io/video-scaler-ffmpeg.c
          media-io/video-scaler.h)

target_sources(
  libobs
  PRIVATE graphics/axisang.c
          graphics/axisang.h
          graphics/bounds.c
          graphics/bounds.h
          graphics/device-exports.h
          graphics/effect.c
          graphics/effect.h
          graphics/effect-parser.c
          graphics/effect-parser.h
          graphics/image-file.c
          graphics/graphics.c
          graphics/graphics.h
          graphics/graphics-ffmpeg.c
          graphics/graphics-imports.c
          graphics/graphics-internal.h
          graphics/half.h
          graphics/image-file.h
          graphics/math-extra.c
          graphics/math-extra.h
          graphics/matrix3.c
          graphics/matrix3.h
          graphics/matrix4.c
          graphics/matrix4.h
          graphics/plane.c
          graphics/plane.h
          graphics/quat.c
          graphics/quat.h
          graphics/shader-parser.c
          graphics/shader-parser.h
          graphics/srgb.h
          graphics/texture-render.c
          graphics/vec2.c
          graphics/vec2.h
          graphics/vec3.c
          graphics/vec3.h
          graphics/vec4.c
          graphics/vec4.h
          graphics/libnsgif/libnsgif.c
          graphics/libnsgif/libnsgif.h)

target_compile_features(libobs PUBLIC $<INSTALL_INTERFACE:cxx_std_17>)
target_compile_options(libobs PUBLIC ${ARCH_SIMD_FLAGS})
target_compile_definitions(
  libobs
  PRIVATE IS_LIBOBS
  PUBLIC ${ARCH_SIMD_DEFINES} $<BUILD_INTERFACE:$<$<BOOL:${ENABLE_HEVC}>:ENABLE_HEVC>>
         $<BUILD_INTERFACE:$<$<BOOL:${ENABLE_FFMPEG_MUX_DEBUG}>:SHOW_SUBPROCESSES>>)

target_link_libraries(
  libobs
  PRIVATE OBS::caption
          OBS::uthash
          FFmpeg::avcodec
          FFmpeg::avformat
          FFmpeg::avutil
          FFmpeg::swscale
          FFmpeg::swresample
          jansson::jansson
          ZLIB::ZLIB
  PUBLIC Threads::Threads)

if(OS_WINDOWS)
  include(cmake/os-windows.cmake)
elseif(OS_MACOS)
  include(cmake/os-macos.cmake)
elseif(OS_LINUX)
  include(cmake/os-linux.cmake)
elseif(OS_FREEBSD)
  include(cmake/os-freebsd.cmake)
endif()

configure_file(obsconfig.h.in "${CMAKE_BINARY_DIR}/config/obsconfig.h" @ONLY)
target_include_directories(libobs PUBLIC "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/config>"
                                         "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_definitions(libobs PUBLIC HAVE_OBSCONFIG_H)

list(
  APPEND
  public_headers
  obs.h
  obs-config.h
  obs-defs.h
  obs-data.h
  obs-interaction.h
  obs-internal.h
  obs-module.h
  obs-properties.h
  obs-ui.h
  callback/calldata.h
  callback/proc.h
  callback/signal.h
  graphics/graphics.h
  graphics/vec2.h
  graphics/vec3.h
  graphics/vec4.h
  media-io/audio-io.h
  media-io/frame-rate.h
  media-io/video-io.h
  media-io/media-io-defs.h
  util/base.h
  util/bmem.h
  util/c99defs.h
  util/darray.h
  util/profiler.h
  util/text-lookup.h
  util/util_uint64.h)

if(ENABLE_HEVC)
  list(APPEND public_headers obs-hevc.h)
endif()

if(ARCH_SIMD_FLAGS)
  list(
    APPEND
    public_headers
    util/simde/check.h
    util/simde/debug-trap.h
    util/simde/hedley.h
    util/simde/simde-align.h
    util/simde/simde-arch.h
    util/simde/simde-common.h
    util/simde/simde-constify.h
    util/simde/simde-detect-clang.h
    util/simde/simde-diagnostic.h
    util/simde/simde-features.h
    util/simde/simde-math.h
    util/simde/x86/mmx.h
    util/simde/x86/sse2.h
    util/simde/x86/sse.h)
endif()

set_property(
  TARGET libobs
  APPEND
  PROPERTY OBS_PUBLIC_HEADERS ${public_headers})

set_target_properties_obs(
  libobs
  PROPERTIES FOLDER core
             VERSION 0
             SOVERSION "${OBS_VERSION_MAJOR}")

target_export(libobs)
