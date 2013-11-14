if(WIN32)
	set(libobs_opengl_platform_src
		gl-windows.c)
elseif(APPLE AND UNIX)
	set(libobs_opengl_platform_objc_src gl-cocoa.m)
	set_source_files_properties(${libobs_opengl_platform_objc_src}
		PROPERTIES LANGUAGE C)
	set(libobs_opengl_platform_src ${libobs_opengl_platform_objc_src})
	find_library(COCOA Cocoa)
	include_directories(${COCOA})
	link_libraries(${COCOA})
else()
endif()

option(USE_STATIC_GLEW "Build using bundled GLEW" ${WIN32})

include_directories(${obs_SOURCE_DIR}/libobs)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

add_definitions(-DGLEW_NO_GLU)

if(NOT USE_STATIC_GLEW)
	find_package(GLEW REQUIRED)
	include_directories(${GLEW_INCLUDE_DIRS})
	set(GLEW_SRC)
else()
	include_directories(glew/include)
	add_definitions(-DGLEW_STATIC)
	set(GLEW_SRC glew/src/glew.c)
endif()

add_library(libobs-opengl MODULE
	gl-helpers.c
	gl-indexbuffer.c
	gl-shader.c
	gl-shaderparser.c
	gl-stagesurf.c
	gl-subsystem.c
	gl-texture2d.c
	gl-texturecube.c
	gl-vertexbuffer.c
	gl-zstencil.c
	${libobs_opengl_platform_src}
	${GLEW_SRC})

set_target_properties(libobs-opengl
	PROPERTIES OUTPUT_NAME obs-opengl)

target_link_libraries(libobs-opengl
	libobs
	${OPENGL_LIBRARIES}
	${GLEW_LIBRARIES})
