# OBS CMake Linux defaults module

include_guard(GLOBAL)

option(ENABLE_PULSEAUDIO "Enable PulseAudio support" ON)
option(ENABLE_WAYLAND "Enable building with support for Wayland" ON)

option(ENABLE_RELOCATABLE "Enable relocatable build" OFF)
option(ENABLE_PORTABLE_CONFIG "Enable support for portable config file location" OFF)

# Set default installation directories
include(GNUInstallDirs)

if(CMAKE_INSTALL_LIBDIR MATCHES "(CMAKE_SYSTEM_PROCESSOR)")
  string(REPLACE "CMAKE_SYSTEM_PROCESSOR" "${CMAKE_SYSTEM_PROCESSOR}" CMAKE_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
endif()

if(ENABLE_RELOCATABLE)
  set(OBS_EXECUTABLE_RPATH "$ORIGIN/;$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
  set(OBS_LIBRARY_RPATH "$ORIGIN/")
  set(OBS_MODULE_RPATH "$ORIGIN/;$ORIGIN/..")
endif()

set(OBS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rundir")
set(OBS_EXECUTABLE_DESTINATION "${CMAKE_INSTALL_BINDIR}")
set(OBS_INCLUDE_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/obs")
set(OBS_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
set(OBS_PLUGIN_DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins")
set(OBS_SCRIPT_PLUGIN_DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-scripting")
set(OBS_DATA_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/obs")
set(OBS_CMAKE_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake")

# Set additional paths used by OBS for self-discovery
set(OBS_PLUGIN_PATH "${CMAKE_INSTALL_LIBDIR}/obs-plugins")
set(OBS_SCRIPT_PLUGIN_PATH "${CMAKE_INSTALL_LIBDIR}/obs-scripting")
set(OBS_DATA_PATH "${OBS_DATA_DESTINATION}")
set(OBS_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Enable find_package targets to become globally available targets
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)

include(cpackconfig)
