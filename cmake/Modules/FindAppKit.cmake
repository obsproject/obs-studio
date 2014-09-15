# Once done these will be defined:
#
#  APPKIT_FOUND
#  APPKIT_LIBRARIES

find_library(APPKIT_FRAMEWORK AppKit)

set(APPKIT_LIBRARIES ${APPKIT_FRAMEWORK})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(AppKit DEFAULT_MSG APPKIT_FRAMEWORK)
mark_as_advanced(APPKIT_FRAMEWORK)
