# Once done these will be defined:
#
#  LIBAVUTIL_FOUND
#  LIBAVUTIL_INCLUDE_DIRS
#  LIBAVUTIL_LIBRARIES
#

if(LIBAVUTIL_INCLUDE_DIRS AND LIBAVUTIL_LIBRARIES)
	set(LIBAVUTIL_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_AVUTIL QUIET libavutil)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	find_path(FFMPEG_INCLUDE_DIR
		NAMES libavutil/avutil.h
		HINTS
			ENV FFmpegPath
			${_AVUTIL_INCLUDE_DIRS}
			/usr/include /usr/local/include /opt/local/include /sw/include
		PATH_SUFFIXES ffmpeg libav)

	find_library(AVUTIL_LIB
		NAMES avutil
		HINTS ${FFMPEG_INCLUDE_DIR}/../lib ${FFMPEG_INCLUDE_DIR}/lib${_lib_suffix} ${_AVUTIL_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(LIBAVUTIL_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR} CACHE PATH "Libavutil include dir")
	set(LIBAVUTIL_LIBRARIES ${AVUTIL_LIB} CACHE STRING "Libavutil libraries")

	find_package_handle_standard_args(Libavutil DEFAULT_MSG AVUTIL_LIB FFMPEG_INCLUDE_DIR)
	mark_as_advanced(FFMPEG_INCLUDE_DIR AVUTIL_LIB)
endif()

