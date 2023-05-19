# OBS CMake bootstrap module

# Enable automatic PUSH and POP of policies to parent scope
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif()

# Prohibit in-source builds
if("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  message(
    FATAL_ERROR
      "OBS: In-source builds of OBS are not supported. Specify a build directory via 'cmake -S <SOURCE DIRECTORY> -B <BUILD_DIRECTORY>' instead."
  )
endif()
file(REMOVE_RECURSE "${CMAKE_SOURCE_DIR}/CMakeCache.txt" "${CMAKE_SOURCE_DIR}/CMakeFiles")

# Use folders for source file organization with IDE generators (Visual Studio/Xcode)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set default global project variables
set(OBS_COMPANY_NAME "OBS Project")
set(OBS_PRODUCT_NAME "OBS Studio")
set(OBS_WEBSITE "https://www.obsproject.com")
set(OBS_COMMENTS "Free and open source software for video recording and live streaming")
set(OBS_LEGAL_COPYRIGHT "(C) Lain Bailey")

# Configure default version strings
message(DEBUG "Setting default project version variables...")
set(_obs_default_version "0" "0" "1")
set(_obs_release_candidate "0" "0" "0" "0")
set(_obs_beta "0" "0" "0" "0")

# Add common module directories to default search path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/common" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/finders")

include(versionconfig)
include(buildnumber)
include(osconfig)

# Allow selection of common build types via UI
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "RelWithDebInfo"
      CACHE STRING "OBS build type [Release, RelWithDebInfo, Debug, MinSizeRel]" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release RelWithDebInfo Debug MinSizeRel)
endif()

# Disable exports automatically going into the CMake package registry
set(CMAKE_EXPORT_PACKAGE_REGISTRY FALSE)
# Enable default inclusion of targets' source and binary directory
set(CMAKE_INCLUDE_CURRENT_DIR TRUE)
