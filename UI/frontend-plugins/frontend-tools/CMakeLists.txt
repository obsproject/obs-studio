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

set(frontend-tools_HEADERS
	${frontend-tools_HEADERS}
	output-timer.hpp
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
elseif(APPLE)
	set(frontend-tools_PLATFORM_SOURCES
		auto-scene-switcher-osx.mm)
	set_source_files_properties(auto-scene-switcher-osx.mm
		PROPERTIES COMPILE_FLAGS "-fobjc-arc")

	set(frontend-tools_PLATFORM_LIBS
		${COCOA})
endif()

qt5_wrap_ui(frontend-tools_UI_HEADERS ${frontend-tools_UI})

add_library(frontend-tools MODULE
	${frontend-tools_HEADERS}
	${frontend-tools_SOURCES}
	${frontend-tools_PLATFORM_SOURCES}
	${frontend-tools_UI_HEADERS}
	)
target_link_libraries(frontend-tools
	${frontend-tools_PLATFORM_LIBS}
	obs-frontend-api
	Qt5::Widgets
	libobs)

install_obs_plugin_with_data(frontend-tools data)
