cmake_minimum_required(VERSION 2.8)

project(obs)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

set(CMAKE_COMPILER_IS_CLANG ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
	option(USE_LIBC++ "Use libc++ instead of libstdc++" ${APPLE})
	if(USE_LIBC++)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	add_definitions(-Wall -Wmissing-prototypes -Wshadow)
endif()

if(UNIX)
	if(APPLE)
		set(CMAKE_MACOSX_RPATH TRUE)
		set(CMAKE_INSTALL_RPATH
			"${CMAKE_INSTALL_RPATH};@loader_path/;@executable_path/")
		option(BUILD_APP_BUNDLE "Build app bundle" false)
	endif()
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

include(obs_helpers)

obs_add_data_dir(/)

set(LIBRARY_OUTPUT_PATH ${obs_BINARY_DIR}/bin)
set(RUNTIME_OUTPUT_PATH ${obs_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${obs_BINARY_DIR}/bin)
add_subdirectory(libobs)
add_subdirectory(libobs-opengl)
if(WIN32)
	add_subdirectory(libobs-d3d11)
	add_subdirectory(libobs-d3d9)
endif()

set(LIBRARY_OUTPUT_PATH ${obs_BINARY_DIR}/plugins)
set(RUNTIME_OUTPUT_PATH ${obs_BINARY_DIR}/plugins)
add_subdirectory(obs)

add_subdirectory(test)
