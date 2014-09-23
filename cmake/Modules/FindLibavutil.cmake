# Once done these will be defined:
#
#  LIBAVUTIL_FOUND
#  LIBAVUTIL_INCLUDE_DIRS
#  LIBAVUTIL_LIBRARIES

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
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_AVUTIL_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES ffmpeg libav)

find_library(AVUTIL_LIB
	NAMES ${_AVUTIL_LIBRARIES} avutil-ffmpeg avutil
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_AVUTIL_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libavutil DEFAULT_MSG AVUTIL_LIB FFMPEG_INCLUDE_DIR)
mark_as_advanced(FFMPEG_INCLUDE_DIR AVUTIL_LIB)

if(LIBAVUTIL_FOUND)
	set(LIBAVUTIL_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
	set(LIBAVUTIL_LIBRARIES ${AVUTIL_LIB})
endif()
