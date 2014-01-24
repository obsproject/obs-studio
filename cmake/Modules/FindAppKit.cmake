# Once done these will be defined:
#
#  AppKit_FOUND
#  AppKit_LIBRARIES
#

if(AppKit_INCLUDE_DIR AND Libavutil_LIBRARIES)
	set(AppKit_FOUND TRUE)
else()
	find_library(APPKIT_FRAMEWORK AppKit)

	set(AppKit_LIBRARIES ${APPKIT_FRAMEWORK} CACHE STRING "AppKit framework")

	find_package_handle_standard_args(AppKit DEFAULT_MSG APPKIT_FRAMEWORK)
	mark_as_advanced(APPKIT_FRAMEWORK)
endif()

