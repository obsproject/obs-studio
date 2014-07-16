# Once done these will be defined:
#
#  LIBX264_FOUND
#  LIBX264_INCLUDE_DIRS
#  LIBX264_LIBRARIES
#

if(LIBX264_INCLUDE_DIRS AND LIBX264_LIBRARIES)
	set(LIBX264_FOUND TRUE)
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
			"${CMAKE_SOURCE_DIR}/additional_install_files/include"
			"$ENV{obsAdditionalInstallFiles}/include"
			ENV x264Path
			ENV FFmpegPath
			"${_X264_INCLUDE_DIRS}"
			/usr/include /usr/local/include /opt/local/include /sw/include)

	find_library(X264_LIB
		NAMES x264 libx264
		HINTS
			"${X264_INCLUDE_DIR}/../lib"
			"${X264_INCLUDE_DIR}/../lib${_lib_suffix}"
			"${X264_INCLUDE_DIR}/../libs${_lib_suffix}"
			"${X264_INCLUDE_DIR}/lib${_lib_suffix}"
			"${_X264_LIBRARY_DIRS}"
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(LIBX264_INCLUDE_DIRS ${X264_INCLUDE_DIR} CACHE PATH "x264 include dir")
	set(LIBX264_LIBRARIES ${X264_LIB} CACHE STRING "x264 libraries")

	find_package_handle_standard_args(Libx264 DEFAULT_MSG X264_LIB X264_INCLUDE_DIR)
	mark_as_advanced(X264_INCLUDE_DIR X264_LIB)
endif()
