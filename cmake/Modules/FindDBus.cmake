# Once done these will be defined:
#
#  DBUS_FOUND
#  DBUS_INCLUDE_DIRS
#  DBUS_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_DBUS QUIET dbus-1)
endif()

find_path(DBUS_INCLUDE_DIR
	NAMES dbus/dbus.h
	HINTS
		${_DBUS_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_path(DBUS_ARCH_INCLUDE_DIR
	NAMES dbus/dbus-arch-deps.h
	HINTS
		${_DBUS_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_library(DBUS_LIB
	NAMES dbus-1
	HINTS
		${_DBUS_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DBus DEFAULT_MSG DBUS_LIB DBUS_INCLUDE_DIR DBUS_ARCH_INCLUDE_DIR)
mark_as_advanced(DBUS_INCLUDE_DIR DBUS_ARCH_INCLUDE_DIR DBUS_LIB)

if(DBUS_FOUND)
	set(DBUS_INCLUDE_DIRS ${DBUS_INCLUDE_DIR} ${DBUS_ARCH_INCLUDE_DIR})
	set(DBUS_LIBRARIES ${DBUS_LIB})
	set(HAVE_DBUS "1")
else()
	set(HAVE_DBUS "0")
endif()
