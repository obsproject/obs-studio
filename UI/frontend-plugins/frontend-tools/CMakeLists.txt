project(frontend-tools)

if(APPLE)
	find_library(COCOA Cocoa)
	include_directories(${COCOA})
endif()

if(WIN32 OR APPLE)
	set(frontend-tools_HEADERS
		auto-scene-switcher.hpp
		)
	set(frontend-tools_SOURCES
		auto-scene-switcher.cpp
		)
	set(frontend-tools_UI
		forms/auto-scene-switcher.ui
		)
endif()

configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/frontend-tools-config.h.in"
	"${CMAKE_BINARY_DIR}/config/frontend-tools-config.h")

set(frontend-tools_HEADERS
	${frontend-tools_HEADERS}
	"${CMAKE_BINARY_DIR}/config/frontend-tools-config.h"
	output-timer.hpp
	tool-helpers.hpp
	)
set(frontend-tools_SOURCES
	${frontend-tools_SOURCES}
	frontend-tools.c
	output-timer.cpp
	)
set(frontend-tools_UI
	${frontend-tools_UI}
	forms/output-timer.ui
	)

if(WIN32)
	set(frontend-tools_PLATFORM_SOURCES
		auto-scene-switcher-win.cpp)

	if(BUILD_CAPTIONS)
		set(frontend-tools_PLATFORM_SOURCES
			${frontend-tools_PLATFORM_SOURCES}
			captions.cpp
			captions-handler.cpp
			captions-mssapi.cpp
			captions-mssapi-stream.cpp)
		set(frontend-tools_PLATFORM_HEADERS
			captions.hpp
			captions-handler.hpp
			captions-mssapi.hpp
			captions-mssapi-stream.hpp)
		set(frontend-tools_PLATFORM_UI
			forms/captions.ui)
	endif()
elseif(APPLE)
	set(frontend-tools_PLATFORM_SOURCES
		auto-scene-switcher-osx.mm)
	set_source_files_properties(auto-scene-switcher-osx.mm
		PROPERTIES COMPILE_FLAGS "-fobjc-arc")

	set(frontend-tools_PLATFORM_LIBS
		${COCOA})
endif()

qt5_wrap_ui(frontend-tools_UI_HEADERS
	${frontend-tools_UI}
	${frontend-tools_PLATFORM_UI})

add_library(frontend-tools MODULE
	${frontend-tools_HEADERS}
	${frontend-tools_SOURCES}
	${frontend-tools_UI_HEADERS}
	${frontend-tools_PLATFORM_SOURCES}
	${frontend-tools_PLATFORM_HEADERS}
	)
target_link_libraries(frontend-tools
	${frontend-tools_PLATFORM_LIBS}
	obs-frontend-api
	Qt5::Widgets
	libobs)

install_obs_plugin_with_data(frontend-tools data)
