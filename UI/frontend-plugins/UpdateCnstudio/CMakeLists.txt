project(UpdateCnOBS)

if(APPLE)
	find_library(COCOA Cocoa)
	include_directories(${COCOA})
endif()

set(UpdateCnOBS_HEADERS
	caboutdlg.h
	cupdateobcn.h
	json11.hpp
	UrlHttp.h
	uimd5.h
	)
set(UpdateCnOBS_SOURCES
	caboutdlg.cpp
	pluginstoremodule.c
	cupdateobcn.cpp
	json11.cpp
	UrlHttp.cpp
	uimd5.cpp
	)
set(UpdateCnOBS_UI
	forms/caboutdlg.ui
	forms/cupdateobcn.ui
	)
set(UpdateCnOBS_QRC
	forms/ui.qrc)
qt5_wrap_ui(UpdateCnOBS_UI_HEADERS
	${UpdateCnOBS_UI}
	)
qt5_add_resources(UpdateCnOBS_QRC_SOURCES ${UpdateCnOBS_QRC})
find_package(Libcurl REQUIRED)
include_directories(${LIBCURL_INCLUDE_DIRS})
add_definitions(${LIBCURL_DEFINITIONS})
add_library(UpdateCnOBS MODULE
	${UpdateCnOBS_HEADERS}
	${UpdateCnOBS_SOURCES}
	${UpdateCnOBS_UI_HEADERS}
	${UpdateCnOBS_QRC_SOURCES}
	)

target_link_libraries(UpdateCnOBS
	${UpdateCnOBS_PLATFORM_LIBS}
	obs-frontend-api
	Qt5::Widgets
	libobs
	${LIBCURL_LIBRARIES})
install_obs_plugin_with_data(UpdateCnOBS data)

