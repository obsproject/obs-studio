project(PluginStore)

if(APPLE)
	find_library(COCOA Cocoa)
	include_directories(${COCOA})
endif()

if(WIN32 OR APPLE)
	set(PluginStore_HEADERS
		PluginStore.h
		)
	set(PluginStore_SOURCES
		PluginStore.cpp
		)
	set(pluginstore_UI
		forms/pluginstore.ui
		)
endif()
set(pluginstore_HEADERS
	${pluginstore_HEADERS}
	PluginStore.h
	webpluginevent.h
	plugindb.h
	)
set(pluginstore_SOURCES
	${PluginStore_SOURCES}
	pluginstoremodule.c
	webpluginevent.cpp
	plugindb.cpp
	)
set(pluginstore_UI
	${pluginstore_UI}
	)
qt5_wrap_ui(pluginstore_UI_HEADERS
	${pluginstore_UI}
	)
find_package(Qt5WebChannel)
find_package(Qt5WebEngineWidgets)
find_package(Qt5Sql)
add_library(pluginstore MODULE
	${pluginstore_HEADERS}
	${pluginstore_SOURCES}
	${pluginstore_UI_HEADERS}
	)

target_link_libraries(pluginstore
	${pluginstore_PLATFORM_LIBS}
	obs-frontend-api
	Qt5::Widgets
	Qt5::WebChannel
	Qt5::WebEngineWidgets
	Qt5::Sql
	libobs)
install_obs_plugin_with_data(pluginstore data)

