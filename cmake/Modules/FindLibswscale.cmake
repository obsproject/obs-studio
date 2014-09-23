# Once done these will be defined:
#
#  LIBSWSCALE_FOUND
#  LIBSWSCALE_INCLUDE_DIRS
#  LIBSWSCALE_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_SWSCALE QUIET libswscale)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(SWSCALE_INCLUDE_DIR
	NAMES libswscale/swscale.h
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_SWSCALE_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES ffmpeg libav)

find_library(SWSCALE_LIB
	NAMES ${_SWSCALE_LIBRARIES} swscale-ffmpeg swscale
	HINTS
		ENV FFmpegPath${_lib_suffix}
		ENV FFmpegPath
		${_SWSCALE_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib
		libs${_lib_suffix} libs
		bin${_lib_suffix} bin)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libswscale DEFAULT_MSG SWSCALE_LIB SWSCALE_INCLUDE_DIR)
mark_as_advanced(SWSCALE_INCLUDE_DIR SWSCALE_LIB)

if(LIBSWSCALE_FOUND)
	set(LIBSWSCALE_INCLUDE_DIRS ${SWSCALE_INCLUDE_DIR})
	set(LIBSWSCALE_LIBRARIES ${SWSCALE_LIB})
endif()
