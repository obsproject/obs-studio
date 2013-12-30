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
	
	find_library(IOSURF IOSurface)
	include_directories(${IOSURF})
	link_libraries(${IOSURF})
else()
endif()

option(USE_BUNDLED_GLEW "Build using bundled GLEW" ${WIN32})

option(USE_STATIC_GLEW "Link GLEW statically (ignored with USE_BUNDLED_GLEW)"
	false)

include_directories(SYSTEM ${obs_SOURCE_DIR}/libobs)

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIRS})

add_definitions(-DGLEW_NO_GLU)

if(USE_BUNDLED_GLEW)
	include_directories(SYSTEM glew/include)
	add_definitions(-DGLEW_STATIC)
	set(GLEW_SRC glew/src/glew.c)
else()
	if(USE_STATIC_GLEW)
		set(lib_prefixes ${CMAKE_FIND_LIBRARY_PREFIXES})
		set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_STATIC_LIBRARY_PREFIX}
			${CMAKE_DYNAMIC_LIBRARY_PREFIX}
			${CMAKE_SHARED_OBJECT_PREFIX})
		set(glew_static "static")
	endif()
	find_package(GLEW REQUIRED ${glew_static})
	include_directories(${GLEW_INCLUDE_DIRS})
	if(USE_STATIC_GLEW)
		unset(glew_static)
		set(CMAKE_FIND_LIBRARY_PREFIXES ${lib_prefixes})
		unset(lib_prefixes)
	endif()
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

obs_add_core_lib(libobs-opengl)
