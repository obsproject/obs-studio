# Once done these will be defined:
#
#  LIBAVFORMAT_FOUND
#  LIBAVFORMAT_INCLUDE_DIRS
#  LIBAVFORMAT_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_AVFORMAT QUIET libavformat)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(AVFORMAT_INCLUDE_DIR
	NAMES libavformat/avformat.h
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_AVFORMAT_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES ffmpeg libav)

find_library(AVFORMAT_LIB
	NAMES ${_AVFORMAT_LIBRARIES} avformat-ffmpeg avformat
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_AVFORMAT_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libavformat DEFAULT_MSG AVFORMAT_LIB AVFORMAT_INCLUDE_DIR)
mark_as_advanced(AVFORMAT_INCLUDE_DIR AVFORMAT_LIB)

if(LIBAVFORMAT_FOUND)
	set(LIBAVFORMAT_INCLUDE_DIRS ${AVFORMAT_INCLUDE_DIR})
	set(LIBAVFORMAT_LIBRARIES ${AVFORMAT_LIB})
endif()
