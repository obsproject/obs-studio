# Once done these will be defined:
#
#  LIBAVCODEC_FOUND
#  LIBAVCODEC_INCLUDE_DIRS
#  LIBAVCODEC_LIBRARIES
#
# For use in OBS: 
#
#  FFMPEG_INCLUDE_DIR
#

if(LIBAVCODEC_INCLUDE_DIRS AND LIBAVCODEC_LIBRARIES)
	set(LIBAVCODEC_FOUND TRUE)
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

	set(FFMPEG_PATH_ARCH FFmpegPath${_lib_suffix})

	find_path(FFMPEG_INCLUDE_DIR
		NAMES libavcodec/avcodec.h
		HINTS
			${_AVCODEC_INCLUDE_DIRS}
			"${CMAKE_SOURCE_DIR}/additional_install_files/include"
			"$ENV{obsAdditionalInstallFiles}/include"
			ENV FFmpegPath
			ENV ${FFMPEG_PATH_ARCH}
		PATHS
			/usr/include /usr/local/include /opt/local/include /sw/include
		PATH_SUFFIXES ffmpeg libav)

	find_library(AVCODEC_LIB
		NAMES ${_AVCODEC_LIBRARIES} avcodec-ffmpeg avcodec
		HINTS
			${_AVCODEC_LIBRARY_DIRS}
			"${FFMPEG_INCLUDE_DIR}/../lib"
			"${FFMPEG_INCLUDE_DIR}/../lib${_lib_suffix}"
			"${FFMPEG_INCLUDE_DIR}/../libs${_lib_suffix}"
			"${FFMPEG_INCLUDE_DIR}/lib"
			"${FFMPEG_INCLUDE_DIR}/lib${_lib_suffix}"
		PATHS
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(LIBAVCODEC_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR} CACHE PATH "Libavcodec include dir")
	set(LIBAVCODEC_LIBRARIES ${AVCODEC_LIB} CACHE STRING "Libavcodec libraries")

	find_package_handle_standard_args(Libavcodec DEFAULT_MSG AVCODEC_LIB FFMPEG_INCLUDE_DIR)
	mark_as_advanced(FFMPEG_INCLUDE_DIR AVCODEC_LIB)
endif()

