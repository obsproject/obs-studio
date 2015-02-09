project(graphics-hook)

set(graphics-hook_HEADERS
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
	d3d11-capture.cpp)

add_library(graphics-hook MODULE
	${graphics-hook_SOURCES}
	${graphics-hook_HEADERS})

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
