project(aja)

if(DISABLE_AJA)
	message(STATUS "aja plugin disabled")
	return()
endif()

set(aja_INCLUDE_DIRS
	sdk
	sdk/ajabase
	sdk/ajaanc
	sdk/ajantv2/includes)

set(aja_SOURCES
	main.cpp
	aja-card-manager.cpp
	aja-common.cpp
	aja-output.cpp
	aja-props.cpp
	aja-routing.cpp
	aja-source.cpp
	aja-widget-io.cpp)

set(aja_HEADERS
	aja-card-manager.hpp
	aja-common.hpp
	aja-enums.hpp
	aja-ui-props.hpp
	aja-output.hpp
	aja-props.hpp
	aja-routing.hpp
	aja-source.hpp
	aja-widget-io.hpp)

set(aja_LIBRARIES
	libobs)

# macOS
if(APPLE)
	set(aja_COMPILE_DEFS
		AJAMac
		AJA_MAC)

	set(aja_INCLUDE_DIRS
		${aja_INCLUDE_DIRS}
		ntv2/ajantv2/src/mac)

	if (CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64")
		if(CMAKE_BUILD_TYPE STREQUAL Debug)
			set(aja_LINK_DIRS
				sdk/lib/Debug/macos/x64)
		else()
			set(aja_LINK_DIRS
				sdk/lib/Release/macos/x64)
		endif()
	elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
		if(CMAKE_BUILD_TYPE STREQUAL Debug)
			set(aja_LINK_DIRS
				sdk/lib/Debug/macos/arm64)
		else()
			set(aja_LINK_DIRS
				sdk/lib/Release/macos/arm64)
		endif()
	elseif(CMAKE_OSX_ARCHITECTURES STREQUAL "x86_64;arm64"|"arm64;x86_64")
		message(WARNING "aja plugin macOS universal build not yet implemented!")
		return()
	endif()

	find_library(IOKIT_FRAMEWORK Iokit)
	find_library(COREFOUNDATION_LIBRARY CoreFoundation)
	find_library(APPKIT_FRAMEWORK AppKit)

	list(APPEND aja_LIBRARIES
		${IOKIT_FRAMEWORK}
		${COREFOUNDATION_LIBRARY}
		${APPKIT_FRAMEWORK})
# Windows
elseif(WIN32)
	set(aja_COMPILE_DEFS
		AJA_WINDOWS
		_WINDOWS
		WIN32
		MSWindows)

	set(aja_INCLUDE_DIRS
		${aja_INCLUDE_DIRS}
		sdk/ajantv2/src/win)

	if(CMAKE_BUILD_TYPE STREQUAL Debug)
		list(APPEND aja_COMPILE_DEFS
			_DEBUG)

		set(aja_LINK_DIRS
			sdk/lib/Debug/windows/x64)
	else()
		list(APPEND aja_COMPILE_DEFS
			NDEBUG)

		set(aja_LINK_DIRS
			sdk/lib/Release/windows/x64)
	endif()

	list(APPEND aja_LIBRARIES
		ws2_32.lib
		setupapi.lib
		Winmm.lib
		netapi32.lib
		Shlwapi.lib)
# Linux
elseif(UNIX AND NOT APPLE)
	if (CMAKE_BUILD_TYPE STREQUAL Debug)
		set(aja_LINK_DIRS
			sdk/lib/Debug/linux/x64)
	else()
		set(aja_LINK_DIRS
			sdk/lib/Release/linux/x64)
	endif()

	set(aja_COMPILE_DEFS
		AJA_LINUX
		AJALinux)

	set(aja_INCLUDE_DIRS
		${aja_INCLUDE_DIRS}
		sdk/ajantv2/src/lin)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
	list(APPEND aja_LIBRARIES
		ajantv2d)
else()
	list(APPEND aja_LIBRARIES
		ajantv2)
endif()

add_library(aja MODULE ${aja_SOURCES} ${aja_HEADERS})

target_include_directories(aja PUBLIC ${aja_INCLUDE_DIRS})
target_link_directories(aja PUBLIC ${aja_LINK_DIRS})
target_link_libraries(aja PUBLIC ${aja_LIBRARIES})
target_compile_definitions(aja PUBLIC ${aja_COMPILE_DEFS})

set_target_properties(aja PROPERTIES FOLDER
		"plugins")

install_obs_plugin_with_data(aja data)
