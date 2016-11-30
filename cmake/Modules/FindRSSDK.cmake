# Once done these will be defined:
#
#  RSSDK_FOUND
#  RSSDK_INCLUDE_DIRS
#  RSSDK_LIBRARIES
#
# For use in OBS:
#
#  RSSDK_INCLUDE_DIR

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(_RSSDK_lib_dir "x64")
else()
	set(_RSSDK_lib_dir "Win32")
endif()

find_path(RSSDK_INCLUDE_DIR
	NAMES pxcsession.h
	HINTS
		ENV RSSDK_DIR
	PATH_SUFFIXES
		include)

find_library(RSSDK_LIB
	NAMES libpxc
	HINTS
		ENV RSSDK_DIR
	PATH_SUFFIXES
		lib/${_RSSDK_lib_dir}
	)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(RSSDK DEFAULT_MSG RSSDK_LIB RSSDK_INCLUDE_DIR)
mark_as_advanced(RSSDK_INCLUDE_DIR RSSDK_LIB)

if(RSSDK_FOUND)
	set(RSSDK_INCLUDE_DIRS ${RSSDK_INCLUDE_DIR})
	set(RSSDK_LIBRARIES ${RSSDK_LIB})
endif()

