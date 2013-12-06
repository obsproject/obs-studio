if(WIN32)
	set(libobs_platform_src
		obs-windows.c
		util/platform-windows.c)
elseif(APPLE AND UNIX)
	set(libobs_platform_src
		obs-cocoa.c
		util/platform-cocoa.m)
	set_source_files_properties(${libobs_platform_src}
		PROPERTIES
			LANGUAGE C
			COMPILE_FLAGS "-fobjc-arc")
	add_definitions("-DHAVE_STRTOLL")
	find_library(COCOA Cocoa)
	mark_as_advanced(COCOA)
	include_directories(${COCOA})
	link_libraries(${COCOA})
else()
endif()

find_package(Libavutil REQUIRED)
include_directories(${LIBAVUTIL_INCLUDE_DIRS})

find_package(Libswresample REQUIRED)
include_directories(${LIBSWRESAMPLE_INCLUDE_DIRS})

add_library(libobs SHARED
		obs.c
		obs-display.c
		obs-module.c
		obs-output.c
		obs-scene.c
		obs-source.c
		obs-video.c
		util/base.c
		util/bmem.c
		util/cf-lexer.c
		util/cf-parser.c
		util/config-file.c
		util/dstr.c
		util/lexer.c
		util/platform.c
		util/text-lookup.c
		util/utf8.c
		media-io/audio-io.c
		media-io/format-conversion.c
		media-io/audio-resampler-ffmpeg.c
		media-io/media-io.c
		media-io/video-io.c
		graphics/axisang.c
		graphics/bounds.c
		graphics/effect.c
		graphics/effect-parser.c
		graphics/graphics.c
		graphics/graphics-imports.c
		graphics/math-extra.c
		graphics/matrix3.c
		graphics/matrix4.c
		graphics/plane.c
		graphics/quat.c
		graphics/shader-parser.c
		graphics/texture-render.c
		graphics/vec2.c
		graphics/vec3.c
		graphics/vec4.c
		${libobs_platform_src})

set_target_properties(libobs
	PROPERTIES OUTPUT_NAME obs)

target_link_libraries(libobs
	${LIBAVUTIL_LIBRARIES}
	${LIBSWRESAMPLE_LIBRARIES})

file(COPY ${obs_SOURCE_DIR}/build/libobs/default.effect DESTINATION
	${obs_BINARY_DIR}/libobs/)
