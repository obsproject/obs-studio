project(libobs)

find_package(Libswresample REQUIRED)
include_directories(${Libswresample_INCLUDE_DIR})
add_definitions(${Libswresample_DEFINITIONS})

find_package(Libavutil REQUIRED)
include_directories(${Libavutil_INCLUDE_DIR})
add_definitions(${Libavutil_DEFINITIONS})

add_definitions(-DLIBOBS_EXPORTS)
add_definitions(-DPTW32_STATIC_LIB)

if(WIN32)
	set(libobs_PLATFORM_SOURCES
		obs-windows.c
		util/platform-windows.c)
	set(libobs_PLATFORM_DEPS
		w32-pthreads)
elseif(APPLE)
	set(libobs_PLATFORM_SOURCES
		obs-cocoa.c
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
		util/platform-nix.c)
endif()

if(MSVC)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHc-")
	set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} /EHc-")
endif()

set(libobs_callback_SOURCES
	callback/calldata.c
	callback/signal.c
	callback/proc.c)
set(libobs_callback_HEADERS
	callback/calldata.h
	callback/proc.h
	callback/signal.h)

set(libobs_graphics_SOURCES
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
	graphics/vec2.h
	graphics/vec4.h
	graphics/matrix3.h
	graphics/vec3.h
	graphics/math-extra.h
	graphics/bounds.h
	graphics/effect-parser.h)

set(libobs_mediaio_SOURCES
	media-io/video-io.c
	media-io/audio-resampler-ffmpeg.c
	media-io/format-conversion.c
	media-io/audio-io.c)
set(libobs_mediaio_HEADERS
	media-io/format-conversion.h
	media-io/video-io.h
	media-io/audio-resampler.h
	media-io/audio-io.h)

set(libobs_util_SOURCES
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
	obs-encoder.c
	obs-source.c
	obs-output.c
	obs.c
	obs-data.c
	obs-module.c
	obs-display.c
	obs-scene.c
	obs-video.c)
set(libobs_libobs_HEADERS
	obs-defs.h
	obs-encoder.h
	obs-service.h
	obs-internal.h
	obs.h
	obs-data.h
	obs-module.h
	obs-scene.h
	obs-source.h
	obs-output.h)

set(libobs_SOURCES
	${libobs_callback_SOURCES}
	${libobs_graphics_SOURCES}
	${libobs_mediaio_SOURCES}
	${libobs_util_SOURCES}
	${libobs_libobs_SOURCES})

set(libobs_HEADERS
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
target_link_libraries(libobs
	${libobs_PLATFORM_DEPS}
	${Libswresample_LIBRARIES}
	${Libavutil_LIBRARIES})

install_obs_core(libobs)
install_obs_data(libobs ../build/data/libobs libobs)
