project(obs)

include_directories(SYSTEM Qt5 "${CMAKE_SOURCE_DIR}/libobs")

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

if(DEFINED ENV{QTDIR${_lib_suffix}})
	list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR${_lib_suffix}}")
elseif(DEFINED ENV{QTDIR})
	list(APPEND CMAKE_PREFIX_PATH "$ENV{QTDIR}")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
set(CMAKE_AUTOMOC TRUE)

find_package(Qt5Widgets REQUIRED)

if(WIN32)
	set(obs_PLATFORM_SOURCES
		platform-windows.cpp)
elseif(APPLE)
	set(obs_PLATFORM_SOURCES
		platform-osx.mm)

	find_package(AppKit REQUIRED)
	set(obs_PLATFORM_LIBRARIES ${AppKit_LIBRARIES})

	add_definitions(-fobjc-arc)
elseif(UNIX)
	set(obs_PLATFORM_SOURCES
		platform-x11.cpp)

	find_package(X11)
	include_directories(${X11_INCLUDE_DIRS} ${X11_Xinerama_INCLUDE_PATH})

	if(NOT X11_Xinerama_FOUND)
		message(FATAL_ERROR "Xinerama not found!")
	endif()

	set(obs_PLATFORM_LIBRARIES
		${X11_LIBRARIES}
		${X11_Xinerama_LIB})
endif()

set(obs_SOURCES
	${obs_PLATFORM_SOURCES}
	obs-app.cpp
	window-basic-main.cpp
	window-basic-settings.cpp
	window-namedialog.cpp
	qt-wrappers.cpp)

set(obs_HEADERS
	obs-app.hpp
	platform.hpp
	window-main.hpp
	window-basic-main.hpp
	window-basic-settings.hpp
	window-namedialog.hpp
	qt-display.hpp
	qt-wrappers.hpp)

set(obs_UI
	forms/NameDialog.ui
	forms/OBSBasic.ui
	forms/OBSBasicSettings.ui)

set(obs_QRC
	forms/obs.qrc)

qt5_wrap_ui(obs_UI_HEADERS ${obs_UI})
qt5_add_resources(obs_QRC_SOURCES ${obs_QRC})

add_executable(obs WIN32
	${obs_SOURCES}
	${obs_HEADERS}
	${obs_UI_HEADERS}
	${obs_QRC_SOURCES})

if(WIN32)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_output_suffix "64")
	else()
		set(_output_suffix "32")
	endif()

	set_target_properties(obs
		PROPERTIES
			OUTPUT_NAME "obs${_output_suffix}")
endif()

target_link_libraries(obs
	libobs
	Qt5::Widgets
	${obs_PLATFORM_LIBRARIES})

install_obs_core(obs)
install_obs_data(obs ../build/data/obs-studio obs-studio)

obs_fixup_install_target(obs TARGET Qt5::Widgets)
obs_fixup_install_target(obs TARGET Qt5::Gui)
obs_fixup_install_target(obs TARGET Qt5::Core)
