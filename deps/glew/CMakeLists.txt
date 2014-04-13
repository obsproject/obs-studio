project(glew)

find_package(OpenGL REQUIRED)

set(glew_lib_SOURCES
	src/glew.c)

add_library(glew STATIC
	${glew_lib_SOURCES})
target_compile_definitions(glew
	PUBLIC
		GLEW_STATIC GLEW_NO_GLU)
target_include_directories(glew PUBLIC
	"${CMAKE_CURRENT_SOURCE_DIR}/include"
	"${OPENGL_INCLUDE_DIR}")

if(NOT WIN32)
	target_compile_options(glew PRIVATE
		-fPIC -DPIC)
endif()

target_link_libraries(glew ${OPENGL_gl_LIBRARY})
