cmake_minimum_required(VERSION 3.16...3.25)

option(ENABLE_CARLA "Enable building OBS with carla plugin host" ON)

if(NOT ENABLE_CARLA)
  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    target_disable(carla)
  else()
    message(STATUS "OBS:  DISABLED   carla")
  endif()
  return()
endif()

# Find carla utils
find_package(CarlaUtils)
if(NOT CarlaUtils_FOUND)
  if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
    target_disable(carla)
  else()
    message(STATUS "OBS:  DISABLED   carla (carla-utils library not found)")
  endif()
  return()
endif()

# Find Qt
find_qt(COMPONENTS Core Widgets)

# Setup carla-bridge target
add_library(carla-bridge MODULE)
add_library(OBS::carla-bridge ALIAS carla-bridge)

if(OS_MACOS)
  # needed for using Qt as framework
  target_compile_options(carla-bridge PRIVATE -Wno-quoted-include-in-framework-header)
endif()

target_compile_definitions(carla-bridge PRIVATE CARLA_MODULE_ID="carla-bridge" CARLA_MODULE_NAME="Audio Plugin"
                                                CARLA_UTILS_USE_QT)

target_link_libraries(carla-bridge PRIVATE carla::utils OBS::libobs Qt::Core Qt::Widgets $<$<C_COMPILER_ID:GNU>:dl>)

target_sources(
  carla-bridge
  PRIVATE carla.c
          carla-bridge.cpp
          carla-bridge-wrapper.cpp
          common.c
          pluginlistdialog.cpp
          pluginrefreshdialog.hpp
          qtutils.cpp)

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  set_target_properties_obs(
    carla-bridge
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON
               FOLDER plugins
               PREFIX "")
else()
  set_target_properties(
    carla-bridge
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON
               FOLDER plugins
               PREFIX "")
  setup_plugin_target(carla-bridge)
endif()

# Setup carla-patchbay target (only available for certain systems)
if(PKGCONFIG_FOUND AND NOT (OS_MACOS OR OS_WINDOWS))
  pkg_check_modules(carla-host-plugin IMPORTED_TARGET QUIET carla-host-plugin)
  if(carla-host-plugin_FOUND)
    add_library(carla-patchbay MODULE)
    add_library(OBS::carla-patchbay ALIAS carla-patchbay)

    target_compile_definitions(carla-patchbay PRIVATE CARLA_MODULE_ID="carla-patchbay"
                                                      CARLA_MODULE_NAME="Carla Patchbay")

    target_link_libraries(carla-patchbay PRIVATE OBS::libobs Qt::Core Qt::Widgets PkgConfig::carla-host-plugin)

    target_sources(carla-patchbay PRIVATE carla.c carla-patchbay-wrapper.c common.c qtutils.cpp)

    if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
      set_target_properties_obs(carla-patchbay PROPERTIES FOLDER plugins PREFIX "")
    else()
      set_target_properties(carla-patchbay PROPERTIES FOLDER plugins PREFIX "")
      setup_plugin_target(carla-patchbay)
    endif()
  endif()
endif()
