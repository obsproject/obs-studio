project(libobs)

find_package(Libswscale REQUIRED)
include_directories(${LIBSWSCALE_INCLUDE_DIRS})
add_definitions(${LIBSWSCALE_DEFINITIONS})

find_package(Libswresample REQUIRED)
include_directories(${LIBSWRESAMPLE_INCLUDE_DIRS})
add_definitions(${LIBSWRESAMPLE_DEFINITIONS})

find_package(Libavutil REQUIRED)
include_directories(${LIBAVUTIL_INCLUDE_DIRS})
add_definitions(${LIBAVUTIL_DEFINITIONS})

find_package(Libavformat REQUIRED)
include_directories(${LIBAVFORMAT_INCLUDE_DIRS})
add_definitions(${LIBAVFORMAT_DEFINITIONS})

find_package(Libavcodec QUIET)
find_package(ImageMagick QUIET COMPONENTS MagickCore)

if(NOT ImageMagick_MagickCore_FOUND AND NOT LIBAVCODEC_FOUND)
	message(FATAL_ERROR "Either MagickCore or Libavcodec is required, but both were not found")
endif()

option(LIBOBS_PREFER_IMAGEMAGICK "Prefer ImageMagick over ffmpeg for image loading" ON)

if(NOT LIBAVCODEC_FOUND OR (ImageMagick_MagickCore_FOUND AND LIBOBS_PREFER_IMAGEMAGICK))
	message(STATUS "Using ImageMagick for image loading in libobs")

	set(libobs_image_loading_SOURCES
		graphics/graphics-magick.c)
	set(libobs_image_loading_LIBRARIES
		${ImageMagick_LIBRARIES})

	include_directories(
		${ImageMagick_INCLUDE_DIRS})
else()
	message(STATUS "Using libavcodec for image loading in libobs")

	set(libobs_image_loading_SOURCES
		graphics/graphics-ffmpeg.c)
	set(libobs_image_loading_LIBRARIES
		${LIBAVCODEC_LIBRARIES})

	include_directories(${LIBAVCODEC_INCLUDE_DIRS})
endif()

add_definitions(-DLIBOBS_EXPORTS)

include_directories(${OBS_JANSSON_INCLUDE_DIRS})

if(WIN32)
	set(libobs_PLATFORM_SOURCES
		obs-windows.c
		util/threading-windows.c
		util/platform-windows.c)
	set(libobs_PLATFORM_DEPS
		w32-pthreads
		winmm.lib)
elseif(APPLE)
	set(libobs_PLATFORM_SOURCES
		obs-cocoa.c
		util/threading-posix.c
		util/platform-nix.c
		util/platform-cocoa.m)

	set_source_files_properties(${libobs_PLATFORM_SOURCES}
		PROPERTIES
			LANGUAGE C
			COMPILE_FLAGS "-fobjc-arc")

	find_library(COCOA Cocoa)
	mark_as_advanced(COCOA)
	include_directories(${COCOA})

	set(libobs_PLATFORM_DEPS
		${COCOA})
elseif(UNIX)
	set(libobs_PLATFORM_SOURCES
		obs-nix.c
		util/threading-posix.c
		util/platform-nix.c)
endif()

if(MSVC)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHc-")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHc-")
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/obsconfig.h.in"
	"${CMAKE_BINARY_DIR}/config/obsconfig.h")

set(libobs_config_HEADERS
	"${CMAKE_BINARY_DIR}/config/obsconfig.h"
	obs-config.h)

set(libobs_callback_SOURCES
	callback/calldata.c
	callback/decl.c
	callback/signal.c
	callback/proc.c)
set(libobs_callback_HEADERS
	callback/calldata.h
	callback/decl.h
	callback/proc.h
	callback/signal.h)

set(libobs_graphics_SOURCES
	${libobs_image_loading_SOURCES}
	graphics/quat.c
	graphics/effect-parser.c
	graphics/axisang.c
	graphics/vec4.c
	graphics/vec2.c
	graphics/texture-render.c
	graphics/bounds.c
	graphics/matrix3.c
	graphics/matrix4.c
	graphics/vec3.c
	graphics/graphics.c
	graphics/shader-parser.c
	graphics/plane.c
	graphics/effect.c
	graphics/math-extra.c
	graphics/graphics-imports.c)
set(libobs_graphics_HEADERS
	graphics/plane.h
	graphics/quat.h
	graphics/input.h
	graphics/axisang.h
	graphics/shader-parser.h
	graphics/effect.h
	graphics/math-defs.h
	graphics/matrix4.h
	graphics/graphics.h
	graphics/graphics-internal.h
	graphics/device-exports.h
	graphics/vec2.h
	graphics/vec4.h
	graphics/matrix3.h
	graphics/vec3.h
	graphics/math-extra.h
	graphics/bounds.h
	graphics/effect-parser.h)

set(libobs_mediaio_SOURCES
	media-io/video-io.c
	media-io/video-fourcc.c
	media-io/video-matrices.c
	media-io/audio-io.c
	media-io/video-frame.c
	media-io/format-conversion.c
	media-io/audio-resampler-ffmpeg.c
	media-io/video-scaler-ffmpeg.c)
set(libobs_mediaio_HEADERS
	media-io/media-io-defs.h
	media-io/video-io.h
	media-io/audio-io.h
	media-io/video-frame.h
	media-io/format-conversion.h
	media-io/audio-resampler.h
	media-io/video-scaler.h)

set(libobs_util_SOURCES
	util/array-serializer.c
	util/base.c
	util/platform.c
	util/cf-lexer.c
	util/bmem.c
	util/config-file.c
	util/lexer.c
	util/dstr.c
	util/utf8.c
	util/text-lookup.c
	util/cf-parser.c)
set(libobs_util_HEADERS
	util/array-serializer.h
	util/utf8.h
	util/base.h
	util/text-lookup.h
	util/vc/vc_inttypes.h
	util/vc/vc_stdbool.h
	util/vc/vc_stdint.h
	util/bmem.h
	util/c99defs.h
	util/cf-parser.h
	util/threading.h
	util/cf-lexer.h
	util/darray.h
	util/circlebuf.h
	util/dstr.h
	util/serializer.h
	util/config-file.h
	util/lexer.h
	util/platform.h)

set(libobs_libobs_SOURCES
	${libobs_PLATFORM_SOURCES}
	obs-avc.c
	obs-encoder.c
	obs-service.c
	obs-source.c
	obs-output.c
	obs.c
	obs-properties.c
	obs-data.c
	obs-module.c
	obs-display.c
	obs-view.c
	obs-scene.c
	obs-video.c)
set(libobs_libobs_HEADERS
	obs-defs.h
	obs-avc.h
	obs-encoder.h
	obs-service.h
	obs-internal.h
	obs.h
	obs-ui.h
	obs-properties.h
	obs-data.h
	obs-module.h
	obs-scene.h
	obs-source.h
	obs-output.h
	obs-ffmpeg-compat.h
	obs.hpp)

set(libobs_SOURCES
	${libobs_callback_SOURCES}
	${libobs_graphics_SOURCES}
	${libobs_mediaio_SOURCES}
	${libobs_util_SOURCES}
	${libobs_libobs_SOURCES})

set(libobs_HEADERS
	${libobs_config_HEADERS}
	${libobs_callback_HEADERS}
	${libobs_graphics_HEADERS}
	${libobs_mediaio_HEADERS}
	${libobs_util_HEADERS}
	${libobs_libobs_HEADERS})

source_group("callback\\Source Files" FILES ${libobs_callback_SOURCES})
source_group("callback\\Header Files" FILES ${libobs_callback_HEADERS})
source_group("graphics\\Source Files" FILES ${libobs_graphics_SOURCES})
source_group("graphics\\Header Files" FILES ${libobs_graphics_HEADERS})
source_group("libobs\\Source Files" FILES ${libobs_libobs_SOURCES})
source_group("libobs\\Header Files" FILES ${libobs_libobs_HEADERS})
source_group("media-io\\Source Files" FILES ${libobs_mediaio_SOURCES})
source_group("media-io\\Header Files" FILES ${libobs_mediaio_HEADERS})
source_group("util\\Source Files" FILES ${libobs_util_SOURCES})
source_group("util\\Header Files" FILES ${libobs_util_HEADERS})


add_library(libobs SHARED ${libobs_SOURCES} ${libobs_HEADERS})

set_target_properties(libobs PROPERTIES
	OUTPUT_NAME obs
	VERSION "0"
	SOVERSION "0")
target_compile_definitions(libobs PUBLIC HAVE_OBSCONFIG_H)

if(NOT MSVC)
	target_compile_options(libobs PUBLIC "-mmmx" "-msse" "-msse2")
endif()

target_include_directories(libobs PUBLIC
	"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
	"$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/config>"
	"$<INSTALL_INTERFACE:${OBS_INCLUDE_DESTINATION}>")


target_link_libraries(libobs
	PRIVATE
		${libobs_PLATFORM_DEPS}
		${libobs_image_loading_LIBRARIES}
		${OBS_JANSSON_IMPORT}
		${LIBSWSCALE_LIBRARIES}
		${LIBSWRESAMPLE_LIBRARIES}
		${LIBAVFORMAT_LIBRARIES}
		${LIBAVUTIL_LIBRARIES})

install_obs_core(libobs EXPORT LibObs)
install_obs_data(libobs data libobs)
install_obs_headers(${libobs_HEADERS})

obs_install_additional(libobs)
