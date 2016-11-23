project(graphics-hook)

set(COMPILE_D3D12_HOOK FALSE CACHE BOOL "Compile D3D12 hook support (required windows 10 SDK)")

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/graphics-hook-config.h.in"
	"${CMAKE_BINARY_DIR}/plugins/win-capture/graphics-hook/config/graphics-hook-config.h")


set(graphics-hook_HEADERS
	"${CMAKE_BINARY_DIR}/plugins/win-capture/graphics-hook/config/graphics-hook-config.h"
	graphics-hook.h
	../graphics-hook-info.h
	../hook-helpers.h
	../funchook.h
	../obfuscate.h
	gl-decs.h
	d3d9-patches.hpp)

set(graphics-hook_SOURCES
	graphics-hook.c
	../funchook.c
	../obfuscate.c
	gl-capture.c
	d3d8-capture.cpp
	d3d9-capture.cpp
	dxgi-capture.cpp
	d3d10-capture.cpp
	d3d11-capture.cpp
	d3d12-capture.cpp)

if(MSVC)
	add_compile_options("$<$<CONFIG:RelWithDebInfo>:/MT>")
endif()

add_library(graphics-hook MODULE
	${graphics-hook_SOURCES}
	${graphics-hook_HEADERS})

target_include_directories(graphics-hook PUBLIC
	"${CMAKE_BINARY_DIR}/plugins/win-capture/graphics-hook/config")

target_link_libraries(graphics-hook
	ipc-util psapi)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_output_suffix "64")
else()
	set(_output_suffix "32")
endif()

set_target_properties(graphics-hook
	PROPERTIES
		OUTPUT_NAME "graphics-hook${_output_suffix}")

install_obs_datatarget(graphics-hook "obs-plugins/win-capture")
