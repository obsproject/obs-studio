# Once done these will be defined:
#
#  LIBAVCODEC_FOUND
#  LIBAVCODEC_INCLUDE_DIRS
#  LIBAVCODEC_LIBRARIES
#
# For use in OBS: 
#
#  FFMPEG_INCLUDE_DIR

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_AVCODEC QUIET libavcodec)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(FFMPEG_INCLUDE_DIR
	NAMES libavcodec/avcodec.h
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_AVCODEC_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES ffmpeg libav)

find_library(AVCODEC_LIB
	NAMES ${_AVCODEC_LIBRARIES} avcodec-ffmpeg avcodec
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_AVCODEC_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libavcodec DEFAULT_MSG AVCODEC_LIB FFMPEG_INCLUDE_DIR)
mark_as_advanced(FFMPEG_INCLUDE_DIR AVCODEC_LIB)

if(LIBAVCODEC_FOUND)
	set(LIBAVCODEC_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR})
	set(LIBAVCODEC_LIBRARIES ${AVCODEC_LIB})
endif()
