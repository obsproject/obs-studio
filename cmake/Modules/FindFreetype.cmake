# Once done these will be defined:
#
#  LIBFREETYPE_FOUND
#  LIBFREETYPE_INCLUDE_DIRS
#  LIBFREETYPE_LIBRARIES
#
# For use in OBS: 
#
#  FREETYPE_INCLUDE_DIR
#

if(LIBFREETYPE_INCLUDE_DIRS AND LIBFREETYPE_LIBRARIES)
	set(LIBFREETYPE_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_FREETYPE QUIET freetype2)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	set(FREETYPE_PATH_ARCH FreetypePath${_lib_suffix})

	find_path(FREETYPE_INCLUDE_DIR
		NAMES ft2build.h
		HINTS
			${_FREETYPE_INCLUDE_DIRS}
			"${CMAKE_SOURCE_DIR}/additional_install_files/include"
			"$ENV{obsAdditionalInstallFiles}/include"
			ENV FreetypePath
			ENV ${FREETYPE_PATH_ARCH}
		PATHS
			/usr/include /usr/local/include /opt/local/include /sw/include)

	find_library(FREETYPE_LIB
		NAMES ${_FREETYPE_LIBRARIES} freetype libfreetype
		HINTS
			${_FREETYPE_LIBRARY_DIRS}
			"${FREETYPE_INCLUDE_DIR}/../lib"
			"${FREETYPE_INCLUDE_DIR}/../lib${_lib_suffix}"
			"${FREETYPE_INCLUDE_DIR}/../libs${_lib_suffix}"
			"${FREETYPE_INCLUDE_DIR}/lib"
			"${FREETYPE_INCLUDE_DIR}/lib${_lib_suffix}"
		PATHS
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib)

	set(LIBFREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR} CACHE PATH "freetype include dir")
	set(LIBFREETYPE_LIBRARIES ${FREETYPE_LIB} CACHE STRING "freetype libraries")

	find_package_handle_standard_args(Libfreetype DEFAULT_MSG FREETYPE_LIB FREETYPE_INCLUDE_DIR)
	mark_as_advanced(FREETYPE_INCLUDE_DIR FREETYPE_LIB)
endif()
