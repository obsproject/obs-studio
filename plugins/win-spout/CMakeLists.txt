project(win-spout)

find_package(OpenGL REQUIRED)

include_directories(${OPENGL_INCLUDE_DIR})

set(spout_HEADERS
		Spout2/SpoutSDK/Source/Spout.h
		Spout2/SpoutSDK/Source/SpoutSDK.h
		Spout2/SpoutSDK/Source/SpoutSender.h
		Spout2/SpoutSDK/Source/SpoutReceiver.h
		Spout2/SpoutSDK/Source/SpoutSharedMemory.h
		Spout2/SpoutSDK/Source/SpoutGLDXinterop.h
		Spout2/SpoutSDK/Source/SpoutCopy.h
		Spout2/SpoutSDK/Source/SpoutGLextensions.h
		Spout2/SpoutSDK/Source/SpoutMemoryshare.h
		Spout2/SpoutSDK/Source/SpoutDirectX.h
		Spout2/SpoutSDK/Source/SpoutSenderNames.h)

set(spout_SOURCES
		Spout2/SpoutSDK/Source/SpoutSDK.cpp
		Spout2/SpoutSDK/Source/SpoutSender.cpp
		Spout2/SpoutSDK/Source/SpoutReceiver.cpp
		Spout2/SpoutSDK/Source/SpoutSharedMemory.cpp
		Spout2/SpoutSDK/Source/SpoutGLDXinterop.cpp
		Spout2/SpoutSDK/Source/SpoutCopy.cpp
		Spout2/SpoutSDK/Source/SpoutGLextensions.cpp
		Spout2/SpoutSDK/Source/SpoutMemoryshare.cpp
		Spout2/SpoutSDK/Source/SpoutDirectX.cpp
		Spout2/SpoutSDK/Source/SpoutSenderNames.cpp)

set(win-spout_HEADERS
		SpoutPlugin.hpp)

set(win-spout_SOURCES
		plugin-main.cpp
		SpoutPlugin.cpp)

add_library(win-spout MODULE
	${win-spout_SOURCES}
	${win-spout_HEADERS}
	${spout_HEADERS}
	${spout_SOURCES})

target_link_libraries(win-spout
	libobs
	d3d9
	d3d11
	${OPENGL_gl_LIBRARY})

install_obs_plugin_with_data(win-spout data)

