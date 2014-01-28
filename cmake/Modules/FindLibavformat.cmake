# Once done these will be defined:
#
#  Libavformat_FOUND
#  Libavformat_INCLUDE_DIR
#  Libavformat_LIBRARIES
#

if(Libavformat_INCLUDE_DIR AND Libavformat_LIBRARIES)
	set(Libavformat_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_AVFORMAT QUIET libavformat)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	find_path(FFMPEG_INCLUDE_DIR
		NAMES libavformat/avformat.h
		HINTS
			ENV FFmpegPath
			${_AVFORMAT_INCLUDE_DIRS}
			/usr/include /usr/local/include /opt/local/include /sw/include
		PATH_SUFFIXES ffmpeg libav)

	find_library(AVFORMAT_LIB
		NAMES avformat
		HINTS ${FFMPEG_INCLUDE_DIR}/../lib ${FFMPEG_INCLUDE_DIR}/lib${_lib_suffix} ${_AVFORMAT_LIBRARY_DIRS} /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(Libavformat_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} CACHE PATH "Libavformat include dir")
	set(Libavformat_LIBRARIES ${AVFORMAT_LIB} CACHE STRING "Libavformat libraries")

	find_package_handle_standard_args(Libavformat DEFAULT_MSG AVFORMAT_LIB FFMPEG_INCLUDE_DIR)
	mark_as_advanced(FFMPEG_INCLUDE_DIR AVFORMAT_LIB)
endif()

