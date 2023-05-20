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

# Use pkg-config to find carla libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(carla REQUIRED IMPORTED_TARGET carla-host-plugin)

# Find Qt
find_qt(COMPONENTS Core Widgets)

add_library(carla-patchbay MODULE)
add_library(OBS::carla-patchbay ALIAS carla-patchbay)

target_compile_definitions(
  carla-patchbay
  PRIVATE CARLA_MODULE_ID="carla-patchbay" CARLA_MODULE_NAME="Carla Patchbay"
          # FIXME remove macro in next Carla stable release
          REAL_BUILD)

target_link_libraries(carla-patchbay PRIVATE OBS::libobs Qt::Core Qt::Widgets PkgConfig::carla)

target_sources(carla-patchbay PRIVATE carla.c carla-patchbay-wrapper.c common.c qtutils.cpp)

if(OBS_CMAKE_VERSION VERSION_GREATER_EQUAL 3.0.0)
  set_target_properties_obs(carla-patchbay PROPERTIES FOLDER plugins PREFIX "")
else()
  set_target_properties(carla-patchbay PROPERTIES FOLDER plugins PREFIX "")
  setup_plugin_target(carla-patchbay)
endif()
