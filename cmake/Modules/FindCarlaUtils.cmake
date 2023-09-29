# Once done these will be defined:
#
# CarlaUtils_FOUND CarlaUtils_INCLUDE_DIRS CarlaUtils_LIBRARIES

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(PC_CarlaUtils QUIET carla-utils)
endif()

find_path(
  CarlaUtils_INCLUDE_DIR
  NAMES utils/CarlaBridgeUtils.hpp
  HINTS ${PC_CarlaUtils_INCLUDE_DIRS}
  PATHS /usr/include /usr/local/include /app/include
  PATH_SUFFIXES carla)

find_library(
  CarlaUtils_LIBRARY
  NAMES carla_utils libcarla_utils
  HINTS ${PC_CarlaUtils_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /app/lib
  PATH_SUFFIXES carla)

find_program(
  CarlaUtils_BRIDGE_LV2_GTK2
  NAMES carla-bridge-lv2-gtk2
  HINTS ${PC_CarlaUtils_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /app/lib
  PATH_SUFFIXES carla)

find_program(
  CarlaUtils_BRIDGE_LV2_GTK3
  NAMES carla-bridge-lv2-gtk3
  HINTS ${PC_CarlaUtils_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /app/lib
  PATH_SUFFIXES carla)

find_program(
  CarlaUtils_BRIDGE_NATIVE
  NAMES carla-bridge-native
  HINTS ${PC_CarlaUtils_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /app/lib
  PATH_SUFFIXES carla)

find_program(
  CarlaUtils_DISCOVERY_NATIVE
  NAMES carla-discovery-native
  HINTS ${PC_CarlaUtils_LIBRARY_DIRS}
  PATHS /usr/lib /usr/local/lib /app/lib
  PATH_SUFFIXES carla)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  CarlaUtils
  FOUND_VAR CarlaUtils_FOUND
  REQUIRED_VARS CarlaUtils_INCLUDE_DIR CarlaUtils_LIBRARY CarlaUtils_BRIDGE_NATIVE CarlaUtils_DISCOVERY_NATIVE)
mark_as_advanced(CarlaUtils_INCLUDE_DIR CarlaUtils_LIBRARY CarlaUtils_BRIDGE_NATIVE CarlaUtils_DISCOVERY_NATIVE)

if(CarlaUtils_FOUND)
  set(CarlaUtils_INCLUDE_DIRS ${CarlaUtils_INCLUDE_DIR} ${CarlaUtils_INCLUDE_DIR}/includes
                              ${CarlaUtils_INCLUDE_DIR}/utils)
  set(CarlaUtils_LIBRARIES ${CarlaUtils_LIBRARY})

  if(NOT TARGET carla::utils)
    if(IS_ABSOLUTE "${CarlaUtils_LIBRARIES}")
      add_library(carla::utils UNKNOWN IMPORTED GLOBAL)
      set_target_properties(carla::utils PROPERTIES IMPORTED_LOCATION "${CarlaUtils_LIBRARIES}")
    else()
      add_library(carla::utils INTERFACE IMPORTED GLOBAL)
      set_target_properties(carla::utils PROPERTIES IMPORTED_LIBNAME "${CarlaUtils_LIBRARIES}")
    endif()

    if(PC_CarlaUtils_FOUND)
      message("DEBUG: using carla-utils pkg-config | ${PC_CarlaUtils_FOUND} | ${PC_CarlaUtils_LDFLAGS}")
      set_target_properties(carla::utils PROPERTIES INTERFACE_LINK_OPTIONS ${PC_CarlaUtils_LDFLAGS})
    else()
      message("DEBUG: NOT using carla-utils pkg-config | ${PC_CarlaUtils_FOUND} | ${PC_CarlaUtils_LDFLAGS}")
    endif()

    set_target_properties(carla::utils PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CarlaUtils_INCLUDE_DIRS}")
  endif()

  if(NOT TARGET carla::bridge-lv2-gtk2)
    add_executable(carla::bridge-lv2-gtk2 IMPORTED GLOBAL)
    set_target_properties(carla::bridge-lv2-gtk2 PROPERTIES IMPORTED_LOCATION "${CarlaUtils_BRIDGE_LV2_GTK2}")
    add_dependencies(carla::utils carla::bridge-lv2-gtk2)
  endif()

  if(NOT TARGET carla::bridge-lv2-gtk3)
    add_executable(carla::bridge-lv2-gtk3 IMPORTED GLOBAL)
    set_target_properties(carla::bridge-lv2-gtk3 PROPERTIES IMPORTED_LOCATION "${CarlaUtils_BRIDGE_LV2_GTK3}")
    add_dependencies(carla::utils carla::bridge-lv2-gtk3)
  endif()

  if(NOT TARGET carla::bridge-native)
    add_executable(carla::bridge-native IMPORTED GLOBAL)
    set_target_properties(carla::bridge-native PROPERTIES IMPORTED_LOCATION "${CarlaUtils_BRIDGE_NATIVE}")
    add_dependencies(carla::utils carla::bridge-native)
  endif()

  if(NOT TARGET carla::discovery-native)
    add_executable(carla::discovery-native IMPORTED GLOBAL)
    set_target_properties(carla::discovery-native PROPERTIES IMPORTED_LOCATION "${CarlaUtils_DISCOVERY_NATIVE}")
    add_dependencies(carla::utils carla::discovery-native)
  endif()
endif()
