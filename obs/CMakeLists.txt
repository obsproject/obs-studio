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

include_directories(SYSTEM ${obs_SOURCE_DIR}/libobs)

link_libraries(${wxWidgets_LIBRARIES} libobs)

if(WIN32)
	set(obs_platform_src
		platform-windows.cpp)
elseif(APPLE)
	set(obs_platform_src
		platform-osx.mm)
elseif(UNIX)
	set(obs_platform_src
		platform-x11.cpp)
endif()

add_executable(obs
	window-basic-main.cpp
	window-basic-settings.cpp
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
endif()
