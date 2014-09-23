# Once done these will be defined:
#
#  FREETYPE_FOUND
#  FREETYPE_INCLUDE_DIRS
#  FREETYPE_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_FREETYPE QUIET freetype2)
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_lib_suffix 64)
else()
	set(_lib_suffix 32)
endif()

find_path(FREETYPE_INCLUDE_DIR_ft2build
	NAMES
		ft2build.h
	HINTS
		${_FREETYPE_INCLUDE_DIRS}
		"${CMAKE_SOURCE_DIR}/additional_install_files/include"
		"$ENV{obsAdditionalInstallFiles}/include"
		ENV FreetypePath${_lib_suffix}
		ENV FreetypePath
		ENV FREETYPE_DIR
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		freetype2 include/freetype2 include)

find_path(FREETYPE_INCLUDE_DIR_freetype2
	NAMES
		freetype/config/ftheader.h
		config/ftheader.h
	HINTS
		${_FREETYPE_INCLUDE_DIRS}
		"${CMAKE_SOURCE_DIR}/additional_install_files/include"
		"$ENV{obsAdditionalInstallFiles}/include"
		ENV FreetypePath${_lib_suffix}
		ENV FreetypePath
		ENV FREETYPE_DIR
	PATHS
		/usr/include /usr/local/include /opt/local/include /sw/include
	PATH_SUFFIXES
		freetype2 include/freetype2 include)

find_library(FREETYPE_LIB
	NAMES ${_FREETYPE_LIBRARIES} freetype libfreetype
	HINTS
		${_FREETYPE_LIBRARY_DIRS}
		"${CMAKE_SOURCE_DIR}/additional_install_files/lib${_lib_suffix}"
		"$ENV{obsAdditionalInstallFiles}/lib${_lib_suffix}"
		"${FREETYPE_INCLUDE_DIR_ft2build}/../../lib${_lib_suffix}"
		"${FREETYPE_INCLUDE_DIR_ft2build}/../../lib"
		"${FREETYPE_INCLUDE_DIR_ft2build}/../lib${_lib_suffix}"
		"${FREETYPE_INCLUDE_DIR_ft2build}/../lib"
		"${FREETYPE_INCLUDE_DIR_ft2build}/lib${_lib_suffix}"
		"${FREETYPE_INCLUDE_DIR_ft2build}/lib"
		"${FREETYPE_INCLUDE_DIR_ft2build}/bin"
		"${FREETYPE_INCLUDE_DIR_ft2build}/bin${_lib_suffix}"
		"${FREETYPE_INCLUDE_DIR_ft2build}/../bin"
		"${FREETYPE_INCLUDE_DIR_ft2build}/../bin${_lib_suffix}"
		ENV FREETYPE_DIR
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib /sw/lib
	PATH_SUFFIXES
		lib${_lib_suffix} lib)

if(FREETYPE_INCLUDE_DIR_ft2build AND FREETYPE_INCLUDE_DIR_freetype2)
	set(FREETYPE_INCLUDE_DIR "${FREETYPE_INCLUDE_DIR_ft2build};${FREETYPE_INCLUDE_DIR_freetype2}")
	list(REMOVE_DUPLICATES FREETYPE_INCLUDE_DIR)
else()
	unset(FREETYPE_INCLUDE_DIR)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Freetype DEFAULT_MSG FREETYPE_LIB FREETYPE_INCLUDE_DIR_ft2build FREETYPE_INCLUDE_DIR_freetype2)
mark_as_advanced(FREETYPE_INCLUDE_DIR FREETYPE_INCLUDE_DIR_ft2build FREETYPE_INCLUDE_DIR_freetype2 FREETYPE_LIB)

if(FREETYPE_FOUND)
	set(FREETYPE_INCLUDE_DIRS ${FREETYPE_INCLUDE_DIR})
	set(FREETYPE_LIBRARIES ${FREETYPE_LIB})
endif()
