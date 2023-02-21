# OBS CMake Linux defaults module

include_guard(GLOBAL)

option(ENABLE_PULSEAUDIO "Enable PulseAudio support" ON)
option(ENABLE_WAYLAND "Enable building with support for Wayland" ON)

option(ENABLE_RELOCATABLE "Enable relocatable build" OFF)

if(ENABLE_RELOCATABLE)
  set(OBS_EXECUTABLE_RPATH "$ORIGIN/;$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
  set(OBS_LIBRARY_RPATH "$ORIGIN/;$ORIGIN/..")
  set(OBS_MODULE_RPATH "$ORIGIN/;$ORIGIN/..")
endif()

# Set default installation directories
include(GNUInstallDirs)
set(OBS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rundir")
set(OBS_EXECUTABLE_DESTINATION "${CMAKE_INSTALL_BINDIR}")
set(OBS_INCLUDE_DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/obs")
set(OBS_LIBRARY_DESTINATION "${CMAKE_INSTALL_LIBDIR}")
set(OBS_PLUGIN_DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-plugins")
set(OBS_SCRIPT_PLUGIN_DESTINATION "${CMAKE_INSTALL_LIBDIR}/obs-scripting")
set(OBS_DATA_DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/obs")
set(OBS_CMAKE_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake")

# Set relative paths used by OBS for self-discovery
set(OBS_PLUGIN_PATH "../${CMAKE_INSTALL_LIBDIR}/obs-plugins")
set(OBS_SCRIPT_PLUGIN_PATH "../${CMAKE_INSTALL_LIBDIR}/obs-scripting")
set(OBS_DATA_PATH "../${OBS_DATA_DESTINATION}")

# Enable find_package targets to become globally available targets
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)

include(cpackconfig)
