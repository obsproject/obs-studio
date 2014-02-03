if(WIN32 OR APPLE)
	cmake_minimum_required(VERSION 2.8.12)
else()
	cmake_minimum_required(VERSION 2.8.11)
endif()

project(obs-studio)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

include(ObsHelpers)
include(ObsCpack)

add_definitions(-DOBS_VERSION="${OBS_VERSION}")

if(MSVC AND NOT EXISTS "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user")
	file(GENERATE
		OUTPUT "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user"
		INPUT "${CMAKE_SOURCE_DIR}/cmake/ALL_BUILD.vcxproj.user.in")
endif()

if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

find_package(CXX11 REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX11_FLAGS}")

if(${CMAKE_C_COMPILER_ID} MATCHES "Clang" OR ${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	set(CMAKE_COMPILER_IS_CLANG TRUE)
endif()

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-function -Wno-unused-parameter -Wall -Wextra")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99 -Wno-unused-function -Wno-unused-parameter -Wall -Wextra")
	
	option(USE_LIBC++ "Use libc++ instead of libstdc++" ${APPLE})
	if(USE_LIBC++)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
	endif()
endif()

if(WIN32)
	add_definitions(-DUNICODE -D_UNICODE)
endif()

if(APPLE)
	set(CMAKE_MACOSX_RPATH TRUE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	list(APPEND CMAKE_INSTALL_RPATH "@loader_path/" "@executable_path/")
elseif(UNIX AND NOT UNIX_STRUCTURE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
	list(APPEND CMAKE_INSTALL_RPATH "$ORIGIN")
endif()

if(NOT INSTALLER_RUN)
	add_subdirectory(deps)
	add_subdirectory(libobs)

	if(WIN32)
		add_subdirectory(libobs-d3d11)
		add_subdirectory(libobs-d3d9)
	endif()

	add_subdirectory(libobs-opengl)
	add_subdirectory(obs)
	add_subdirectory(plugins)
	add_subdirectory(test)
else()
	obs_generate_multiarch_installer()
endif()

obs_install_additional()
