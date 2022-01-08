project(aja)

if(DISABLE_AJA)
	message(STATUS "aja plugin disabled")
	return()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	find_package(LibAJANTV2)
	if (NOT LIBAJANTV2_FOUND)
		message(STATUS "aja plugin disabled (deps not found)")
		return()
	else()
		message("aja plugin includes: ${LIBAJANTV2_INCLUDE_DIRS}")
		message("aja plugin libs: ${LIBAJANTV2_LIBRARIES}")
		message("aja plugin debug libs: ${LIBAJANTV2_DEBUG_LIBRARIES}")
	endif()
else()
	message(STATUS "aja plugin disabled (32-bit not supported)")
	return()
endif()

set(aja_INCLUDE_DIRS
	${LIBAJANTV2_INCLUDE_DIRS})
set(aja_LIBRARIES
	$<IF:$<CONFIG:Debug>,${LIBAJANTV2_DEBUG_LIBRARIES},${LIBAJANTV2_LIBRARIES}>
	libobs)

set(aja_SOURCES
	main.cpp
	aja-card-manager.cpp
	aja-common.cpp
	aja-output.cpp
	aja-presets.cpp
	aja-props.cpp
	aja-routing.cpp
	aja-source.cpp
	aja-vpid-data.cpp
	aja-widget-io.cpp)

set(aja_HEADERS
	aja-card-manager.hpp
	aja-common.hpp
	aja-enums.hpp
	aja-ui-props.hpp
	aja-output.hpp
	aja-presets.hpp
	aja-props.hpp
	aja-routing.hpp
	aja-source.hpp
	aja-vpid-data.hpp
	aja-widget-io.hpp)

# macOS
if(APPLE)
	set(aja_COMPILE_DEFS
		AJAMac
		AJA_MAC)

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

	if(CMAKE_BUILD_TYPE STREQUAL Debug)
		list(APPEND aja_COMPILE_DEFS
			_DEBUG)
	else()
		list(APPEND aja_COMPILE_DEFS
			NDEBUG)
	endif()

	list(APPEND aja_LIBRARIES
		ws2_32.lib
		setupapi.lib
		Winmm.lib
		netapi32.lib
		Shlwapi.lib)

	set(MODULE_DESCRIPTION "OBS AJA Windows module")
	configure_file(${CMAKE_SOURCE_DIR}/cmake/winrc/obs-module.rc.in win-aja.rc)
	list(APPEND aja_SOURCES
		win-aja.rc)
# Linux
elseif(UNIX AND NOT APPLE)
	set(aja_COMPILE_DEFS
		AJA_LINUX
		AJALinux)
endif()

add_library(aja MODULE ${aja_SOURCES} ${aja_HEADERS})

target_include_directories(aja PUBLIC ${aja_INCLUDE_DIRS})
target_link_libraries(aja PUBLIC ${aja_LIBRARIES})
target_compile_definitions(aja PUBLIC ${aja_COMPILE_DEFS})

set_target_properties(aja PROPERTIES FOLDER
		"plugins")

install_obs_plugin_with_data(aja data)
