project(aja)

set(aja_INCLUDE_DIRS
	sdk/ajalibraries
	sdk/ajalibraries/ajabase
	sdk/ajalibraries/ajaanc
	sdk/ajalibraries/ajantv2/includes)

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
	set(aja_INCLUDE_DIRS
		${aja_INCLUDE_DIRS}
		sdk/ajalibraries/ajantv2/src/mac)

	find_library(IOKIT_FRAMEWORK Iokit)
	find_library(COREFOUNDATION_LIBRARY CoreFoundation)
	find_library(APPKIT_FRAMEWORK AppKit)

	list(APPEND aja_LIBRARIES
		${IOKIT_FRAMEWORK}
		${COREFOUNDATION_LIBRARY}
		${APPKIT_FRAMEWORK})
# Windows
elseif(WIN32)
	set(aja_INCLUDE_DIRS
		${aja_INCLUDE_DIRS}
		sdk/ajalibraries/ajantv2/src/win)

	list(APPEND aja_LIBRARIES
		ws2_32.lib
		setupapi.lib
		Winmm.lib
		netapi32.lib
		Shlwapi.lib)
# Linux
elseif(UNIX AND NOT APPLE)
	set(aja_INCLUDE_DIRS
		${aja_INCLUDE_DIRS}
		sdk/ajalibraries/ajantv2/src/lin)
endif()

if(DEFINED CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	list(APPEND aja_LIBRARIES
		ajantv2d)
else()
	list(APPEND aja_LIBRARIES
		ajantv2)
endif()

add_library(aja MODULE ${aja_SOURCES} ${aja_HEADERS})
target_include_directories(aja PUBLIC ${aja_INCLUDE_DIRS})
target_link_libraries(aja PUBLIC ${aja_LIBRARIES})

# add aja SDK CMakeLists (libajantv2)
add_subdirectory(sdk)
add_dependencies(aja ajantv2)

# macOS
if(APPLE)
	target_compile_definitions(aja PUBLIC
		AJAMac
		AJA_MAC)
	target_link_directories(aja PUBLIC
		${CMAKE_BINARY_DIR}/plugins/aja/sdk/ajalibraries/ajantv2)
# Windows
elseif(WIN32)
	target_compile_definitions(aja PUBLIC
		AJA_WINDOWS
		_WINDOWS
		WIN32
		MSWindows)

	if(DEFINED CMAKE_BUILD_TYPE AND CMAKE_BUILD_TYPE STREQUAL "Debug")
		target_compile_definitions(aja PUBLIC _DEBUG)
		target_link_directories(aja PUBLIC
			${CMAKE_BINARY_DIR}/plugins/aja/sdk/ajalibraries/ajantv2/Debug)
	else()
		target_compile_definitions(aja PUBLIC NDEBUG)
		target_link_directories(aja PUBLIC
			${CMAKE_BINARY_DIR}/plugins/aja/sdk/ajalibraries/ajantv2/Release)
	endif()
# Linux
elseif(UNIX AND NOT APPLE)
	target_compile_definitions(aja PUBLIC
		AJA_LINUX
		AJALinux)
	target_link_directories(aja PUBLIC
		${CMAKE_BINARY_DIR}/plugins/aja/sdk/ajalibraries/ajantv2)
endif()

set_target_properties(aja PROPERTIES FOLDER
		"plugins")

install_obs_plugin_with_data(aja data)
