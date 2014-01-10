option(USE_STATIC_WX "Link wxWidgets statically" false)
if(USE_STATIC_WX)
	set(wxWidgets_USE_STATIC true)
endif()
find_package(wxWidgets COMPONENTS core base REQUIRED)

include("${wxWidgets_USE_FILE}")

function(wx_required_version)
	include(CheckCXXSourceCompiles)
	set(WX_CHECK_DEFINITIONS "")
	foreach(wxw_def ${wxWidgets_DEFINITIONS})
		set(WX_CHECK_DEFINITIONS "${WX_CHECK_DEFINITIONS} -D${wxw_def}")
	endforeach()
	set(CMAKE_REQUIRED_DEFINITIONS ${WX_CHECK_DEFINITIONS})
	set(CMAKE_REQUIRED_INCLUDES ${wxWidgets_INCLUDE_DIRS})
	set(CMAKE_REQUIRED_FLAGS ${wxWidgets_CXX_FLAGS})
	check_cxx_source_compiles("
		#include <wx/sysopt.h>
		#if (wxMINOR_VERSION < 9 && wxMAJOR_VERSION <= 2) || wxMAJOR_VERSION < 3
		#error
		#endif
		int main() {}"
		WX_REQUIRED_VERSION)

	if(NOT WX_REQUIRED_VERSION)
		message(SEND_ERROR "wxWidgets version 2.9 or later is required")
	endif()
endfunction()
wx_required_version()

if(USE_STATIC_WX)
	#unpack -llib to static library names
	set(WX_LIBRARIES "")
	set(lib_suffixes ${CMAKE_FIND_LIBRARY_SUFFIXES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_STATIC_LIBRARY_SUFFIX}
		${CMAKE_DYNAMIC_LIBRARY_SUFFIX} ${CMAKE_SHARED_OBJECT_SUFFIX})
	foreach(lib ${wxWidgets_LIBRARIES})
		string(SUBSTRING ${lib} 0 2 _l)
		if(_l STREQUAL "-l")
			string(SUBSTRING ${lib} 2 -1 libname)
			find_library(lib_file NAMES ${libname}
				HINTS ${wxWidgets_LIBRARY_DIRS})
			if(lib_file)
				list(APPEND WX_LIBRARIES ${lib_file})
			else()
				list(APPEND WX_LIBRARIES ${lib})
			endif()
			unset(lib_file CACHE)
		else()
			list(APPEND WX_LIBRARIES ${lib})
		endif()
	endforeach()
	if(APPLE)
		find_library(lzma NAMES lzma)
		if(lzma)
			link_libraries(${lzma})
		endif()
	endif()
	set(CMAKE_FIND_LIBRARY_SUFFIXES ${lib_suffixes})
	unset(lib_suffixes)
	link_libraries(${WX_LIBRARIES})
	unset(WX_LIBRARIES)
else()
	link_libraries(${wxWidgets_LIBRARIES})
endif()

include_directories(SYSTEM ${obs_SOURCE_DIR}/libobs)

link_libraries(libobs)

if(WIN32)
	set(obs_platform_src
		platform-windows.cpp)
elseif(APPLE)
	set(obs_platform_src
		platform-osx.mm)
	add_definitions(-fobjc-arc)
	if(BUILD_APP_BUNDLE)
		add_definitions(-DOBS_OSX_BUNDLE=1)
	endif()
elseif(UNIX)
	set(obs_platform_src
		platform-x11.cpp)
endif()

add_executable(obs
	window-basic-main.cpp
	window-basic-settings.cpp
	window-namedialog.cpp
	settings-basic.cpp
	settings-basic-general.cpp
	settings-basic-video.cpp
	wx-subclass.cpp
	wx-wrappers.cpp
	obs-app.cpp
	forms/OBSWindows.cpp
	${obs_platform_src})

if(APPLE)
	set_target_properties(obs PROPERTIES
		MACOSX_BUNDLE ${BUILD_APP_BUNDLE})
	if(BUILD_APP_BUNDLE)
		obs_add_core_lib_target(obs)
		obs_add_plugin_target(obs)
		obs_add_data_dir(/bin/obs.app/Contents/)
	endif()
endif()

obs_add_data_source(/data/obs-studio/locale
	${obs_SOURCE_DIR}/build/data/obs-studio/locale/en.txt
	${obs_SOURCE_DIR}/build/data/obs-studio/locale/ja.txt
	${obs_SOURCE_DIR}/build/data/obs-studio/locale/locale.ini)
