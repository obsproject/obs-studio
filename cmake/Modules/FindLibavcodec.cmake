# Once done these will be defined:
#
#  Libavcodec_FOUND
#  Libavcodec_INCLUDE_DIR
#  Libavcodec_LIBRARIES
#

if(Libavcodec_INCLUDE_DIR AND Libavcodec_LIBRARIES)
	set(Libavcodec_FOUND TRUE)
else()
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
			ENV FFmpegPath
			${_AVCODEC_INCLUDE_DIRS}
			/usr/include /usr/local/include /opt/local/include /sw/include
		PATH_SUFFIXES ffmpeg libav)

	find_library(AVCODEC_LIB
		NAMES avcodec
		HINTS ${_AVCODEC_LIBRARY_DIRS} ${FFMPEG_INCLUDE_DIR}/../lib ${FFMPEG_INCLUDE_DIR}/lib${_lib_suffix} /usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(Libavcodec_INCLUDE_DIR ${FFMPEG_INCLUDE_DIR} CACHE PATH "Libavcodec include dir")
	set(Libavcodec_LIBRARIES ${AVCODEC_LIB} CACHE STRING "Libavcodec libraries")

	find_package_handle_standard_args(Libavcodec DEFAULT_MSG AVCODEC_LIB FFMPEG_INCLUDE_DIR)
	mark_as_advanced(FFMPEG_INCLUDE_DIR AVCODEC_LIB)
endif()

