# Once done these will be defined:
#
#  FONTCONFIG_FOUND
#  FONTCONFIG_INCLUDE_DIRS
#  FONTCONFIG_LIBRARIES

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
	pkg_check_modules(_FONTCONFIG QUIET fontconfig)
endif()

find_path(FONTCONFIG_INCLUDE_DIR
	NAMES fontconfig/fontconfig.h
	HINTS
		${_FONTCONFIG_INCLUDE_DIRS}
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_library(FONTCONFIG_LIB
	NAMES fontconfig
	HINTS
		${_FONTCONFIG_LIBRARY_DIRS}
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Fontconfig DEFAULT_MSG FONTCONFIG_LIB
	FONTCONFIG_INCLUDE_DIR)
mark_as_advanced(FONTCONFIG_INCLUDE_DIR FONTCONFIG_LIB)

if(FONTCONFIG_FOUND)
	set(FONTCONFIG_INCLUDE_DIRS ${FONTCONFIG_INCLUDE_DIR})
	set(FONTCONFIG_LIBRARIES "${FONTCONFIG_LIB}")
endif()
