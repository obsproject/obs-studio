# OBS CMake Windows defaults module

include_guard(GLOBAL)

set(OBS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(OBS_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/rundir")

set(OBS_PLUGIN_DESTINATION obs-plugins/64bit)
set(OBS_DATA_DESTINATION data)
set(OBS_CMAKE_DESTINATION cmake)
set(OBS_SCRIPT_PLUGIN_DESTINATION "${OBS_DATA_DESTINATION}/obs-scripting/64bit")

set(OBS_EXECUTABLE_DESTINATION bin/64bit)
set(OBS_LIBRARY_DESTINATION lib)
set(OBS_INCLUDE_DESTINATION include)
# Set relative paths used by OBS for self-discovery
set(OBS_PLUGIN_PATH "../../${CMAKE_INSTALL_LIBDIR}/obs-plugins/64bit")
set(OBS_SCRIPT_PLUGIN_PATH "../../${OBS_DATA_DESTINATION}/obs-scripting/64bit")
set(OBS_DATA_PATH "../../${OBS_DATA_DESTINATION}")

# Enable find_package targets to become globally available targets
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)

include(buildspec)
include(cpackconfig)
