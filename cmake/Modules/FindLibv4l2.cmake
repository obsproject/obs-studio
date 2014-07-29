# Once done these will be defined:
#
#  LIBV4L2_FOUND
#  LIBV4L2_INCLUDE_DIRS
#  LIBV4L2_LIBRARIES
#

if(LIBV4L2_INCLUDE_DIRS AND LIBV4L2_LIBRARIES)
	set(LIBV4L2_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_V4L2 QUIET v4l-utils)
	endif()

	find_path(V4L2_INCLUDE_DIR
		NAMES libv4l2.h
		HINTS ${_V4L2_INCLUDE_DIRS} /usr/include /usr/local/include
			/opt/local/include)

	find_library(V4L2_LIB
		NAMES v4l2
		HINTS ${_V4L2_LIBRARY_DIRS} /usr/lib /usr/local/lib
			/opt/local/lib)

	set(LIBV4L2_INCLUDE_DIRS ${V4L2_INCLUDE_DIR}
		CACHE PATH "v4l2 include dir")
	set(LIBV4L2_LIBRARIES "${V4L2_LIB}"
		CACHE STRING "v4l2 libraries")

	find_package_handle_standard_args(LibV4L2 DEFAULT_MSG V4L2_LIB
		V4L2_INCLUDE_DIR)
	mark_as_advanced(V4L2_INCLUDE_DIR V4L2_LIB)
endif()
