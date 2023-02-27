# Stripped down version of
# https://gitlab.kitware.com/cmake/cmake/blob/e1409101c99f7a3487990e9927e8bd0e275f564f/Source/Modules/FindLibUUID.cmake
#
# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.
#
# Once done these will be defined:
#
# LibUUID_FOUND LibUUID_INCLUDE_DIRS LibUUID_LIBRARIES

find_library(LibUUID_LIBRARY NAMES uuid)
mark_as_advanced(LibUUID_LIBRARY)

find_path(LibUUID_INCLUDE_DIR NAMES uuid/uuid.h)
mark_as_advanced(LibUUID_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  LibUUID
  FOUND_VAR LibUUID_FOUND
  REQUIRED_VARS LibUUID_LIBRARY LibUUID_INCLUDE_DIR)
set(LIBUUID_FOUND ${LibUUID_FOUND})

if(LibUUID_FOUND)
  set(LibUUID_INCLUDE_DIRS ${LibUUID_INCLUDE_DIR})
  set(LibUUID_LIBRARIES ${LibUUID_LIBRARY})
  if(NOT TARGET LibUUID::LibUUID)
    add_library(LibUUID::LibUUID UNKNOWN IMPORTED)
    set_target_properties(
      LibUUID::LibUUID
      PROPERTIES IMPORTED_LOCATION "${LibUUID_LIBRARY}"
                 INTERFACE_INCLUDE_DIRECTORIES "${LibUUID_INCLUDE_DIRS}")
  endif()
endif()
