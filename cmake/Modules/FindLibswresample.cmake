# Once done these will be defined:
#
#  Libswresample_FOUND
#  Libswresample_INCLUDE_DIR
#  Libswresample_LIBRARIES
#

if(Libswresample_INCLUDE_DIR AND Libswresample_LIBRARIES)
	set(Libswresample_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_SWRESAMPLE QUIET libswresample)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()
	
	find_path(FFMPEG_INCLUDE_DIR
		NAMES libswresample/swresample.h
		HINTS
			ENV FFmpegPath
			${_SWRESAMPLE_INCLUDE_DIRS}
			/usr/include /usr/local/include /opt/local/include /sw/include
		PATH_SUFFIXES ffmpeg libav)

	find_library(SWRESAMPLE_LIB
		NAMES swresample
		HINTS ${_SWRESAMPLE_LIBRARY_DIRS} ${FFMPEG_INCLUDE_DIR}/../lib ${FFMPEG_INCLUDE_DIR}/lib${_lib_suffix} /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(Libswresample_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} CACHE PATH "Libswresample include dir")
	set(Libswresample_LIBRARIES ${SWRESAMPLE_LIB} CACHE STRING "Libswresample libraries")

	find_package_handle_standard_args(Libswresample DEFAULT_MSG SWRESAMPLE_LIB FFMPEG_INCLUDE_DIR)
	mark_as_advanced(FFMPEG_INCLUDE_DIR SWRESAMPLE_LIB)
endif()

