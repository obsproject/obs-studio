# Once done these will be defined:
#
#  LIBSWSCALE_FOUND
#  LIBSWSCALE_INCLUDE_DIRS
#  LIBSWSCALE_LIBRARIES
#

if(LIBSWSCALE_INCLUDE_DIRS AND LIBSWSCALE_LIBRARIES)
	set(LIBSWSCALE_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_SWSCALE QUIET libswscale)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()
	
	set(FFMPEG_PATH_ARCH FFmpegPath${_lib_suffix})

	find_path(FFMPEG_INCLUDE_DIR
		NAMES libswscale/swscale.h
		HINTS
			${_SWSCALE_INCLUDE_DIRS}
			"${CMAKE_SOURCE_DIR}/additional_install_files/include"
			"$ENV{obsAdditionalInstallFiles}/include"
			ENV FFmpegPath
			ENV ${FFMPEG_PATH_ARCH}
		PATHS
			/usr/include /usr/local/include /opt/local/include /sw/include
		PATH_SUFFIXES ffmpeg libav)

	find_library(SWSCALE_LIB
		NAMES ${_SWSCALE_LIBRARIES} swscale-ffmpeg swscale
		HINTS
			${_SWSCALE_LIBRARY_DIRS}
			"${FFMPEG_INCLUDE_DIR}/../lib"
			"${FFMPEG_INCLUDE_DIR}/../lib${_lib_suffix}"
			"${FFMPEG_INCLUDE_DIR}/../libs${_lib_suffix}"
			"${FFMPEG_INCLUDE_DIR}/lib"
			"${FFMPEG_INCLUDE_DIR}/lib${_lib_suffix}"
		PATHS
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(LIBSWSCALE_INCLUDE_DIRS ${FFMPEG_INCLUDE_DIR} CACHE PATH "Libswscale include dir")
	set(LIBSWSCALE_LIBRARIES ${SWSCALE_LIB} CACHE STRING "Libswscale libraries")

	find_package_handle_standard_args(Libswscale DEFAULT_MSG SWSCALE_LIB FFMPEG_INCLUDE_DIR)
	mark_as_advanced(FFMPEG_INCLUDE_DIR SWSCALE_LIB)
endif()

