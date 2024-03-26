# OBS CMake macOS defaults module

include_guard(GLOBAL)

# Set empty codesigning team if not specified as cache variable
if(NOT OBS_CODESIGN_TEAM)
  set(OBS_CODESIGN_TEAM "" CACHE STRING "OBS code signing team for macOS" FORCE)

  # Set ad-hoc codesigning identity if not specified as cache variable
  if(NOT OBS_CODESIGN_IDENTITY)
    set(OBS_CODESIGN_IDENTITY "-" CACHE STRING "OBS code signing identity for macOS" FORCE)
  endif()
endif()

include(xcode)

include(buildspec)

# SWIG hard codes the directory to its library directory at compile time. As obs-deps need to be relocatable, we need to
# force SWIG to look for its files in a directory relative to the PREFIX_PATH. The best way to ensure this is to set the
# SWIG_LIB environment variable.
foreach(path IN LISTS CMAKE_PREFIX_PATH)
  if(NOT DEFINED ENV{SWIG_LIB} AND EXISTS "${path}/bin/swig")
    set(ENV{SWIG_LIB} "${path}/share/swig/CURRENT")
    break()
  endif()
endforeach()

# Enable find_package targets to become globally available targets
set(CMAKE_FIND_PACKAGE_TARGETS_GLOBAL TRUE)
# Enable RPATH support for generated binaries
set(CMAKE_MACOSX_RPATH TRUE)
# Use RPATHs from build tree _in_ the build tree
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
# Do not add default linker search paths to RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)
# Use common bundle-relative RPATH for installed targets
set(CMAKE_INSTALL_RPATH "@executable_path/../Frameworks")

# Used for library exports only (obs-frontend-api)
set(OBS_LIBRARY_DESTINATION "lib")
set(OBS_INCLUDE_DESTINATION "include/obs")
set(OBS_CMAKE_DESTINATION "lib/cmake")

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/resources/package.applescript"
  "${CMAKE_CURRENT_BINARY_DIR}/package.applescript"
  @ONLY
)
