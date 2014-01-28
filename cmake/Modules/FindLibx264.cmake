# Once done these will be defined:
#
#  Libx264_FOUND
#  Libx264_INCLUDE_DIR
#  Libx264_LIBRARIES
#

if(Libx264_INCLUDE_DIR AND Libx264_LIBRARIES)
	set(Libx264_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_X264 QUIET x264)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	find_path(X264_INCLUDE_DIR
		NAMES x264.h
		HINTS
			ENV x264Path
			${_X264_INCLUDE_DIRS}
			/usr/include /usr/local/include /opt/local/include /sw/include)

	find_library(X264_LIB
		NAMES x264 libx264
		HINTS ${X264_INCLUDE_DIR}/../lib ${X264_INCLUDE_DIR}/lib${_lib_suffix} ${_X264_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(Libx264_INCLUDE_DIR ${X264_INCLUDE_DIR} CACHE PATH "x264 include dir")
	set(Libx264_LIBRARIES ${X264_LIB} CACHE STRING "x264 libraries")

	find_package_handle_standard_args(Libx264 DEFAULT_MSG X264_LIB X264_INCLUDE_DIR)
	mark_as_advanced(X264_INCLUDE_DIR X264_LIB)
endif()
