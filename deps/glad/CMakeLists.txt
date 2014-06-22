project(glad)

find_package(OpenGL)

if(NOT WIN32 AND NOT APPLE)
	find_package(X11)
endif()

set(glad_SOURCES
	src/glad.c)

if(WIN32)
	set(glad_PLATFORM_SOURCES
		src/glad_wgl.c)
elseif(NOT APPLE)
	set(glad_PLATFORM_SOURCES
		src/glad_glx.c)
endif()

add_library(glad SHARED
	${glad_SOURCES}
	${glad_PLATFORM_SOURCES})
set_target_properties(glad PROPERTIES
	OUTPUT_NAME obsglad
	VERSION "0"
	SOVERSION "0")
target_include_directories(glad
	PUBLIC include
	PRIVATE ${X11_X11_INCLUDE_PATH} ${OPENGL_INCLUDE_DIR})
target_compile_definitions(glad
	PRIVATE GLAD_GLAPI_EXPORT_BUILD)

if(NOT MSVC)
	target_compile_options(glad
		PRIVATE -DPIC -fPIC -fvisibility=hidden)
endif()

if(NOT WIN32 AND NOT APPLE)
	set(glad_PLATFORM_DEPS
		-ldl
		${X11_X11_LIB})
endif()

target_link_libraries(glad
	${glad_PLATFORM_DEPS}
	${OPENGL_gl_LIBRARY})

install_obs_core(glad)
