project(libobs-d3d11)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

add_definitions(-DLIBOBS_EXPORTS)

if(NOT DEFINED GPU_PRIORITY_VAL OR "${GPU_PRIORITY_VAL}" STREQUAL "" OR
   "${GPU_PRIORITY_VAL}" STREQUAL "0")
	set(USE_GPU_PRIORITY FALSE)
	set(GPU_PRIORITY_VAL "0")
else()
	set(USE_GPU_PRIORITY TRUE)
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/d3d11-config.h.in"
	"${CMAKE_CURRENT_BINARY_DIR}/d3d11-config.h")

include_directories(${CMAKE_CURRENT_BINARY_DIR})

set(libobs-d3d11_SOURCES
	d3d11-indexbuffer.cpp
	d3d11-samplerstate.cpp
	d3d11-shader.cpp
	d3d11-shaderprocessor.cpp
	d3d11-stagesurf.cpp
	d3d11-subsystem.cpp
	d3d11-texture2d.cpp
	d3d11-vertexbuffer.cpp
	d3d11-duplicator.cpp
	d3d11-rebuild.cpp
	d3d11-zstencilbuffer.cpp)

set(libobs-d3d11_HEADERS
	${CMAKE_CURRENT_BINARY_DIR}/d3d11-config.h
	d3d11-shaderprocessor.hpp
	d3d11-subsystem.hpp)

add_library(libobs-d3d11 MODULE
	${libobs-d3d11_SOURCES}
	${libobs-d3d11_HEADERS})
set_target_properties(libobs-d3d11
	PROPERTIES
		OUTPUT_NAME libobs-d3d11
		PREFIX "")
target_link_libraries(libobs-d3d11
	libobs
	d3d9
	d3d11
	dxgi)

install_obs_core(libobs-d3d11)
