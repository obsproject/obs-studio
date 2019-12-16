project(decklink-output-ui)

if(APPLE)
	find_library(COCOA Cocoa)
	include_directories(${COCOA})
endif()

if(UNIX AND NOT APPLE)
	find_package(X11 REQUIRED)
	link_libraries(${X11_LIBRARIES})
	include_directories(${X11_INCLUDE_DIR})
endif()

set(decklink-output-ui_HEADERS
	${decklink-output-ui_HEADERS}
	../../properties-view.hpp
	../../properties-view.moc.hpp
	../../vertical-scroll-area.hpp
	../../double-slider.hpp
	../../slider-ignorewheel.hpp
	../../combobox-ignorewheel.hpp
	../../spinbox-ignorewheel.hpp
	./DecklinkOutputUI.h
	decklink-ui-main.h
	)
set(decklink-output-ui_SOURCES
	${decklink-output-ui_SOURCES}
	../../properties-view.cpp
	../../vertical-scroll-area.cpp
	../../double-slider.cpp
	../../slider-ignorewheel.cpp
	../../combobox-ignorewheel.cpp
	../../spinbox-ignorewheel.cpp
	./DecklinkOutputUI.cpp
	decklink-ui-main.cpp
	)
set(decklink-output-ui_UI
	${decklink-output-ui_UI}
	forms/output.ui
	)

if(APPLE)
	set(decklink-output-ui_PLATFORM_LIBS
		${COCOA})
endif()

qt5_wrap_ui(decklink-output-ui_UI_HEADERS
	${decklink-output-ui_UI})

add_library(decklink-output-ui MODULE
	${decklink-output-ui_HEADERS}
	${decklink-output-ui_SOURCES}
	${decklink-output-ui_UI_HEADERS}
	)
target_link_libraries(decklink-output-ui
	${frontend-tools_PLATFORM_LIBS}
	obs-frontend-api
	Qt5::Widgets
	libobs)

install_obs_plugin_with_data(decklink-output-ui data)
