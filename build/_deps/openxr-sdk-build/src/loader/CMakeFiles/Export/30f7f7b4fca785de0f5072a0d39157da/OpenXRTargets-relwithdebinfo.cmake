#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "OpenXR::openxr_loader" for configuration "RelWithDebInfo"
set_property(TARGET OpenXR::openxr_loader APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(OpenXR::openxr_loader PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/libopenxr_loader.so.1.0.32"
  IMPORTED_SONAME_RELWITHDEBINFO "libopenxr_loader.so.1"
  )

list(APPEND _cmake_import_check_targets OpenXR::openxr_loader )
list(APPEND _cmake_import_check_files_for_OpenXR::openxr_loader "${_IMPORT_PREFIX}/lib/libopenxr_loader.so.1.0.32" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
