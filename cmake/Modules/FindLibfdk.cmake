# Once done these will be defined:
#
#  LIBFDK_FOUND
#  LIBFDK_INCLUDE_DIRS
#  LIBFDK_LIBRARIES
#
# For use in OBS: 
#
#  Libfdk_INCLUDE_DIR
#

if(LIBFDK_INCLUDE_DIRS AND LIBFDK_LIBRARIES)
	set(LIBFDK_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_LIBFDK QUIET fdk-aac)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	set(LIBFDK_PATH_ARCH LibfdkPath${_lib_suffix})
	set(FFMPEG_PATH_ARCH FFmpegPath${_lib_suffix})

	find_path(Libfdk_INCLUDE_DIR
		NAMES fdk-aac/aacenc_lib.h
		HINTS
			${_LIBFDK_INCLUDE_DIRS}
			"${CMAKE_SOURCE_DIR}/additional_install_files/include"
			"$ENV{obsAdditionalInstallFiles}/include"
			ENV LibfdkPath
			ENV FFmpegPath
			ENV ${LIBFDK_PATH_ARCH}
			ENV ${FFMPEG_PATH_ARCH}
		PATHS
			/usr/include /usr/local/include /opt/local/include /sw/include)

	find_library(Libfdk_LIB
		NAMES ${_LIBFDK_LIBRARIES} fdk-aac libfdk-aac
		HINTS
			${_LIBFDK_LIBRARY_DIRS}
			"${Libfdk_INCLUDE_DIR}/../lib"
			"${Libfdk_INCLUDE_DIR}/../lib${_lib_suffix}"
			"${Libfdk_INCLUDE_DIR}/../libs${_lib_suffix}"
			"${Libfdk_INCLUDE_DIR}/lib"
			"${Libfdk_INCLUDE_DIR}/lib${_lib_suffix}"
		PATHS
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(LIBFDK_INCLUDE_DIRS ${Libfdk_INCLUDE_DIR} CACHE PATH "Libfdk include dir")
	set(LIBFDK_LIBRARIES ${Libfdk_LIB} CACHE STRING "Libfdk libraries")

	find_package_handle_standard_args(Libfdk DEFAULT_MSG Libfdk_LIB Libfdk_INCLUDE_DIR)
	mark_as_advanced(Libfdk_INCLUDE_DIR Libfdk_LIB)
endif()

