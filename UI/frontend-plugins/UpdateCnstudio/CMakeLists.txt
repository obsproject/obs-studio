project(UpdateCnOBS)

if(APPLE)
	find_library(COCOA Cocoa)
	include_directories(${COCOA})
endif()

set(UpdateCnOBS_HEADERS
	caboutdlg.h
	cupdateobcn.h
	)
set(UpdateCnOBS_SOURCES
	caboutdlg.cpp
	pluginstoremodule.c
	cupdateobcn.cpp
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
	libobs)
install_obs_plugin_with_data(UpdateCnOBS data)

