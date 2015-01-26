# Once done these will be defined:
#
#  XDG_BASEDIR_FOUND
#  XDG_BASEDIR_INCLUDE_DIRS
#  XDG_BASEDIR_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_XDG_BASEDIR QUIET libxdg-basedir)
endif()

find_path(XDG_BASEDIR_INCLUDE_DIR
	NAMES basedir.h
	HINTS
		${_XDG_BASEDIR_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_library(XDG_BASEDIR_LIB
	NAMES xdg-basedir
	HINTS
		${_XDG_BASEDIR_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XDG_BASEDIR DEFAULT_MSG XDG_BASEDIR_LIB XDG_BASEDIR_INCLUDE_DIR)
mark_as_advanced(XDG_BASEDIR_INCLUDE_DIR XDG_BASEDIR_LIB)

if(XDG_BASEDIR_FOUND)
	set(XDG_BASEDIR_INCLUDE_DIRS ${XDG_BASEDIR_INCLUDE_DIR})
	set(XDG_BASEDIR_LIBRARIES ${XDG_BASEDIR_LIB})
endif()
