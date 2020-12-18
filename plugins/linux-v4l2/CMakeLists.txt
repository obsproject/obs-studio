project(linux-v4l2)

if(DISABLE_V4L2)
	message(STATUS "v4l2 plugin disabled")
	return()
endif()


if(ENABLE_V4L2)
	find_package(Libv4l2 REQUIRED)
else()
	find_package(Libv4l2)
	if(NOT LIBV4L2_FOUND)
		message(STATUS "libv4l2 not found, disabling v4l2 plugin")
		return()
	endif()
endif()

if(DISABLE_UDEV)
	add_definitions(-DHAVE_UDEV)
else()
	find_package(UDev)
	if(NOT UDEV_FOUND)
		message(STATUS "udev disabled for v4l2 plugin")
	else()
		set(linux-v4l2-udev_SOURCES
			v4l2-udev.c
		)
	endif()
endif()

find_program(MODPROBE_BINARY NAMES modprobe PATHS /usr/sbin/ /sbin/ /usr/local/sbin)
find_program(MODINFO_BINARY  NAMES modinfo  PATHS /usr/sbin/ /sbin/ /usr/local/sbin)
find_program(PKEXEC_BINARY   NAMES pkexec)

if (NOT MODPROBE_BINARY)
  message(STATUS "Falling back to 'modprobe' without a path")
  set(MODPROBE_BINARY modprobe)
endif (NOT MODPROBE_BINARY)

if (NOT MODINFO_BINARY)
  message(STATUS "Falling back to 'modinfo' without a path")
  set(MODINFO_BINARY modinfo)
endif (NOT MODINFO_BINARY)

if (NOT PKEXEC_BINARY)
  message(STATUS "Falling back to 'pkexec' without a path")
  set(PKEXEC_BINARY pkexec)
endif (NOT PKEXEC_BINARY)

# TODO: most of the other cmake files seem to stick to add_definitions instead of add_compile_definitions so lets stick to that
add_definitions(-DMODPROBE_BINARY="${MODPROBE_BINARY}" -DMODINFO_BINARY="${MODINFO_BINARY}" -DPKEXEC_BINARY="${PKEXEC_BINARY}")

include_directories(
	SYSTEM "${CMAKE_SOURCE_DIR}/libobs"
	${LIBV4L2_INCLUDE_DIRS}
)

set(linux-v4l2_SOURCES
	linux-v4l2.c
	v4l2-controls.c
	v4l2-input.c
	v4l2-helpers.c
	v4l2-output.c
	${linux-v4l2-udev_SOURCES}
)

add_library(linux-v4l2 MODULE
	${linux-v4l2_SOURCES}
)
target_link_libraries(linux-v4l2
	libobs
	${LIBV4L2_LIBRARIES}
	${UDEV_LIBRARIES}
)
set_target_properties(linux-v4l2 PROPERTIES FOLDER "plugins")

install_obs_plugin_with_data(linux-v4l2 data)
