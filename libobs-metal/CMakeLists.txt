project(libobs-metal)

find_library(APPKIT AppKit)
mark_as_advanced(APPKIT)
include_directories(${APPKIT})

find_library(QUARTZCORE QuartzCore)
mark_as_advanced(QUARTZCORE)
include_directories(${QUARTZCORE})

find_library(METAL Metal)
mark_as_advanced(METAL)
include_directories(${METAL})

set(libobs-metal_PLATFORM_DEPS
	${APPKIT}
	${QUARTZCORE})

set(libobs-metal_PLATFORM_DEPS_WEAK
	Metal)

include_directories(SYSTEM "${CMAKE_SOURCE_DIR}/libobs")

add_definitions(-DLIBOBS_EXPORTS)

set(libobs-metal_SOURCES
	metal-device.mm
	metal-indexbuffer.mm
	metal-rebuild.mm
	metal-samplerstate.mm
	metal-shader.mm
	metal-shaderbuilder.cpp
	metal-shaderprocessor.mm
	metal-stagesurf.mm
	metal-subsystem.mm
	metal-swapchain.mm
	metal-texture2d.mm
	metal-vertexbuffer.mm
	metal-zstencilbuffer.mm)

set(libobs-metal_HEADERS
	metal-shaderprocessor.hpp
	metal-subsystem.hpp)

add_library(libobs-metal MODULE
	${libobs-metal_SOURCES}
	${libobs-metal_HEADERS})
set_target_properties(libobs-metal
	PROPERTIES
		OUTPUT_NAME libobs-metal
		PREFIX ""
		XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES)
target_compile_options(libobs-metal
	PUBLIC
		-msse
		-msse2
		-msse3
		-msse4.1
		-msse4.2)
target_link_libraries(libobs-metal
	PRIVATE
		${libobs-metal_PLATFORM_DEPS}
	PUBLIC
		libobs)
foreach(FRAMEWORK ${libobs-metal_PLATFORM_DEPS_WEAK})
	target_link_libraries(libobs-metal
		PRIVATE
			"-weak_framework ${FRAMEWORK}")
endforeach()

install_obs_core(libobs-metal)
