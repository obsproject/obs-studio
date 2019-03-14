project(linux-capture)

find_package(X11 REQUIRED)
if(NOT X11_Xcomposite_FOUND)
	message(STATUS "Xcomposite library not found, linux-capture plugin disabled")
	return()
endif()

find_package(XCB COMPONENTS XCB SHM XFIXES XINERAMA REQUIRED)
find_package(X11_XCB REQUIRED)

include_directories(SYSTEM
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

if(EGL_FOUND)
	find_package(PkgConfig)
	pkg_check_modules(DRM libdrm)

	if(NOT DRM_FOUND)
		message(WARNING "libdrm not found, dmabuf soure will be disabled")
	else()
		set(linux-capture_SOURCES
			${linux-capture_SOURCES}
			dmabuf.c)

		add_executable(obs-drmsend drmsend.c)
		target_include_directories(obs-drmsend PRIVATE ${DRM_INCLUDE_DIRS})
		target_link_libraries(obs-drmsend PRIVATE ${DRM_LIBRARIES})

	# this breaks building OBS under a regular user
	# FIXME Make it a part of install process, which should run under root
	#
	#	add_custom_command(TARGET drmsend POST_BUILD
	#		COMMAND setcap cap_sys_admin+ep "$<TARGET_FILE:drmsend>")
		install_obs_core(obs-drmsend)
	endif(NOT DRM_FOUND)
endif(EGL_FOUND)

add_library(linux-capture MODULE
	${linux-capture_SOURCES}
	${linux-capture_HEADERS}
)
target_link_libraries(linux-capture
	libobs
	glad
	${X11_LIBRARIES}
	${X11_Xfixes_LIB}
	${X11_X11_LIB}
	${X11_Xcomposite_LIB}
	${XCB_LIBRARIES}
)

install_obs_plugin_with_data(linux-capture data)
