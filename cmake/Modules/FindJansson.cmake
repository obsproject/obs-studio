# Once done these will be defined:
#
#  JANSSON_FOUND
#  JANSSON_INCLUDE_DIRS
#  JANSSON_LIBRARIES
#  JANSSON_VERSION
#

if(JANSSON_INCLUDE_DIRS AND JANSSON_LIBRARIES)
	set(JANSSON_FOUND TRUE)
else()
	find_package(PkgConfig QUIET)
	if (PKG_CONFIG_FOUND)
		pkg_check_modules(_JANSSON QUIET jansson)
	endif()

	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(_lib_suffix 64)
	else()
		set(_lib_suffix 32)
	endif()

	set(JANSSON_PATH_ARCH JanssonPath${_lib_suffix})

	find_path(Jansson_INCLUDE_DIR
		NAMES jansson.h
		HINTS
			${_JANSSON_INCLUDE_DIRS}
			ENV JanssonPath
			ENV ${JANSSON_PATH_ARCH}
		PATHS
			/usr/include /usr/local/include /opt/local/include /sw/include)

	find_library(Jansson_LIB
		NAMES ${_JANSSON_LIBRARIES} jansson libjansson
		HINTS
			${_JANSSON_LIBRARY_DIRS}
			"${Jansson_INCLUDE_DIR}/../lib"
			"${Jansson_INCLUDE_DIR}/lib${_lib_suffix}"
		PATHS
			/usr/lib /usr/local/lib /opt/local/lib /sw/lib)
 
	if(JANSSON_VERSION)
		set(_JANSSON_VERSION_STRING "${JANSSON_VERSION}")
	elseif(_JANSSON_FOUND AND _JANSSON_VERSION)
		set(_JANSSON_VERSION_STRING "${_JANSSON_VERSION}")
	elseif(EXISTS "${Jansson_INCLUDE_DIR}/jansson.h")
		file(STRINGS "${Jansson_INCLUDE_DIR}/jansson.h" _jansson_version_parse
			REGEX "#define[ \t]+JANSSON_VERSION[ \t]+.+")
		string(REGEX REPLACE
			".*#define[ \t]+JANSSON_VERSION[ \t]+\"(.+)\".*" "\\1"
			_JANSSON_VERSION_STRING "${_jansson_version_parse}")
	else()
		if(NOT Jansson_FIND_QUIETLY)
			message(WARNING "Failed to find Jansson version")
		endif()
		set(_JANSSON_VERSION_STRING "unknown")
	endif()

	set(JANSSON_INCLUDE_DIRS ${Jansson_INCLUDE_DIR} CACHE PATH "Jansson include dir")
	set(JANSSON_LIBRARIES ${Jansson_LIB} CACHE STRING "Jansson libraries")
	set(JANSSON_VERSION "${_JANSSON_VERSION_STRING}" CACHE STRING "Jansson version")

	find_package_handle_standard_args(Jansson
		FOUND_VAR JANSSON_FOUND
		REQUIRED_VARS Jansson_LIB Jansson_INCLUDE_DIR
		VERSION_VAR JANSSON_VERSION)
	mark_as_advanced(Jansson_INCLUDE_DIR Jansson_LIB)

	if(NOT JANSSON_FOUND)
		unset(JANSSON_INCLUDE_DIRS CACHE)
		unset(JANSSON_LIBRARIES CACHE)
	endif()
endif()

