project(linux-capture)

find_package(X11 REQUIRED)
if(NOT X11_Xcomposite_FOUND)
	message(STATUS "Xcomposite library not found, linux-capture plugin disabled")
	return()
endif()

find_package(XCB COMPONENTS XCB RANDR SHM XFIXES XINERAMA REQUIRED)
find_package(X11_XCB REQUIRED)

set(linux-capture_INCLUDES
	"${CMAKE_SOURCE_DIR}/libobs"
	${X11_Xcomposite_INCLUDE_PATH}
	${X11_X11_INCLUDE_PATH}
	${XCB_INCLUDE_DIRS}
)

set(linux-capture_SOURCES
	linux-capture.c
	xcursor.c
	xcursor-xcb.c
	xhelpers.c
	xshm-input.c
	xcomposite-main.cpp
	xcompcap-main.cpp
	xcompcap-helper.cpp
)
set(linux-capture_HEADERS
	xcursor.h
	xcursor-xcb.h
	xhelpers.h
	xcompcap-main.hpp
	xcompcap-helper.hpp
)

set(linux-capture_LIBRARIES
	libobs
	glad
	${X11_LIBRARIES}
	${X11_Xfixes_LIB}
	${X11_X11_LIB}
	${X11_Xcomposite_LIB}
	${XCB_LIBRARIES}
)

option(ENABLE_PIPEWIRE "Enable PipeWire support" ON)
if(ENABLE_PIPEWIRE)
	find_package(PipeWire QUIET)
	find_package(Gio QUIET)

	if(NOT PIPEWIRE_FOUND)
		message(FATAL_ERROR "PipeWire library not found! Please install PipeWire or set ENABLE_PIPEWIRE=OFF")
	elseif(NOT GIO_FOUND)
		message(FATAL_ERROR "Gio library not found! Please install GLib2 (or Gio) or set ENABLE_PIPEWIRE=OFF")
	endif()

	add_definitions(-DENABLE_PIPEWIRE)

	set(linux-capture_INCLUDES
		${linux-capture_INCLUDES}
		${GIO_INCLUDE_DIRS}
		${PIPEWIRE_INCLUDE_DIRS}
	)

	add_definitions(
		${GIO_DEFINITIONS}
		${PIPEWIRE_DEFINITIONS}
	)

	set(linux-capture_SOURCES
		${linux-capture_SOURCES}
		pipewire.c
		pipewire-capture.c
	)
	set(linux-capture_HEADERS
		${linux-capture_HEADERS}
		pipewire.h
		pipewire-capture.h
	)
	set(linux-capture_LIBRARIES
		${linux-capture_LIBRARIES}
		${GIO_LIBRARIES}
		${PIPEWIRE_LIBRARIES}
	)
endif()

include_directories(SYSTEM
	${linux-capture_INCLUDES}
)
add_library(linux-capture MODULE
	${linux-capture_SOURCES}
	${linux-capture_HEADERS}
)
target_link_libraries(linux-capture
	${linux-capture_LIBRARIES}
)

set_target_properties(linux-capture PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(linux-capture data)
