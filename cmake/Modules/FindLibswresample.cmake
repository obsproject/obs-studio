# Once done these will be defined:
#
#  LIBSWRESAMPLE_FOUND
#  LIBSWRESAMPLE_INCLUDE_DIRS
#  LIBSWRESAMPLE_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_SWRESAMPLE QUIET libswresample)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(SWRESAMPLE_INCLUDE_DIR
	NAMES libswresample/swresample.h
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_SWRESAMPLE_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES ffmpeg libav)

find_library(SWRESAMPLE_LIB
	NAMES ${_SWRESAMPLE_LIBRARIES} swresample-ffmpeg swresample
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_SWRESAMPLE_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libswresample DEFAULT_MSG SWRESAMPLE_LIB SWRESAMPLE_INCLUDE_DIR)
mark_as_advanced(SWRESAMPLE_INCLUDE_DIR SWRESAMPLE_LIB)

if(LIBSWRESAMPLE_FOUND)
	set(LIBSWRESAMPLE_INCLUDE_DIRS ${SWRESAMPLE_INCLUDE_DIR})
	set(LIBSWRESAMPLE_LIBRARIES ${SWRESAMPLE_LIB})
endif()
