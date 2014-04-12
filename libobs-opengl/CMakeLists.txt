project(libobs-opengl)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

add_definitions(-DLIBOBS_EXPORTS)

if(WIN32)
	set(obs-opengl_PLATFORM_SOURCES
		GL/wgl_obs.c)
	set(libobs-opengl_PLATFORM_SOURCES
		gl-windows.c)
elseif(APPLE)
	set(libobs-opengl_PLATFORM_SOURCES
		gl-cocoa.m)

	set_source_files_properties(${libobs-opengl_PLATFORM_SOURCES}
		PROPERTIES
			LANGUAGE C)
	
	find_library(COCOA Cocoa)
	include_directories(${COCOA})
	mark_as_advanced(COCOA)

	find_library(IOSURF IOSurface)
	include_directories(${IOSURF})
	mark_as_advanced(${IOSURF})

	set(libobs-opengl_PLATFORM_DEPS
		${COCOA}
		${IOSURF})
else()
	set(libobs-opengl_PLATFORM_SOURCES
		gl-x11.c)
	set(obs-opengl_PLATFORM_SOURCES
		GL/glx_obs.c)
endif()

set(obs-opengl_SOURCES
	GL/gl_obs.c)

set(libobs-opengl_SOURCES
	${libobs-opengl_PLATFORM_SOURCES}
	gl-helpers.c
	gl-indexbuffer.c
	gl-shader.c
	gl-shaderparser.c
	gl-stagesurf.c
	gl-subsystem.c
	gl-texture2d.c
	gl-texturecube.c
	gl-vertexbuffer.c
	gl-zstencil.c)

set(libobs-opengl_HEADERS
	gl-helpers.h
	gl-shaderparser.h
	gl-subsystem.h)

add_library(obs-opengl STATIC
	${obs-opengl_SOURCES}
	${obs-opengl_PLATFORM_SOURCES})
if(NOT WIN32)
	set_property(TARGET obs-opengl APPEND_STRING
		PROPERTY COMPILE_FLAGS " -fPIC")
endif()
target_link_libraries(obs-opengl
	${OPENGL_gl_LIBRARY})

add_library(libobs-opengl MODULE
	${libobs-opengl_SOURCES}
	${libobs-opengl_HEADERS})
set_target_properties(libobs-opengl
	PROPERTIES
		OUTPUT_NAME libobs-opengl
		PREFIX "")
target_link_libraries(libobs-opengl
	libobs
	obs-opengl
	${libobs-opengl_PLATFORM_DEPS})

install_obs_core(libobs-opengl)
