# Once done these will be defined:
#
#  SYSINFO_FOUND
#  SYSINFO_INCLUDE_DIRS
#  SYSINFO_LIBRARIES

find_path(SYSINFO_INCLUDE_DIR
	NAMES sys/sysinfo.h
	PATHS
		/usr/include /usr/local/include /opt/local/include)

find_library(SYSINFO_LIB
	NAMES sysinfo libsysinfo
	PATHS
		/usr/lib /usr/local/lib /opt/local/lib)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(sysinfo DEFAULT_MSG SYSINFO_LIB SYSINFO_INCLUDE_DIR)
mark_as_advanced(SYSINFO_INCLUDE_DIR SYSINFO_LIB)

if(SYSINFO_FOUND)
	set(SYSINFO_INCLUDE_DIRS ${SYSINFO_INCLUDE_DIR})
	set(SYSINFO_LIBRARIES ${SYSINFO_LIB})
endif()
