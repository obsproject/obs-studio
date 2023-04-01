# OBS CMake macOS defaults module

# Enable selection between arm64 and x86_64 targets
if(NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES
      arm64
      CACHE STRING "Build architectures for macOS" FORCE)
endif()
set_property(CACHE CMAKE_OSX_ARCHITECTURES PROPERTY STRINGS arm64 x86_64)

# Set empty codesigning team if not specified as cache variable
if(NOT OBS_CODESIGN_TEAM)
  set(OBS_CODESIGN_TEAM
      ""
      CACHE STRING "OBS code signing team for macOS" FORCE)

  # Set ad-hoc codesigning identity if not specified as cache variable
  if(NOT OBS_CODESIGN_IDENTITY)
    set(OBS_CODESIGN_IDENTITY
        "-"
        CACHE STRING "OBS code signing identity for macOS" FORCE)
  endif()
endif()

if(XCODE)
  include(xcode)
endif()

include(buildspec)

# Set default deployment target to 11.0 if not set and enable selection in GUI up to 13.0
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
  set(CMAKE_OSX_DEPLOYMENT_TARGET
      11.0
      CACHE STRING "Minimum macOS version to target for deployment (at runtime). Newer APIs will be weak-linked." FORCE)
endif()
set_property(CACHE CMAKE_OSX_DEPLOYMENT_TARGET PROPERTY STRINGS 13.0 12.0 11.0)

# Use Applications directory as default install destination
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX
      "/Applications"
      CACHE STRING "Directory to install OBS after building" FORCE)
endif()

# Show warning about missing CMAKE_PREFIX_PATH, usually required for successful builds
if(NOT DEFINED CMAKE_PREFIX_PATH)
  message(
    WARNING "No CMAKE_PREFIX_PATH set: OBS supplies pre-built dependencies for building on macOS.\n"
            "While OBS can be built using packages installed via Homebrew, pre-built dependencies "
            "contain beneficial patches and fixes for use within OBS and is the suggested source "
            "of these dependencies.\n"
            "You can download the appropriate obs-deps package for your "
            "architecture and set CMAKE_PREFIX_PATH to this directory:\n"
            "CMAKE_PREFIX_PATH=\"<PATH_TO_OBS_DEPS>\"\n"
            "Download pre-built OBS dependencies at https://github.com/obsproject/obs-deps/releases\n")
endif()

# SWIG hard codes the directory to its library directory at compile time. As obs-deps need to be relocatable, we need to
# force SWIG to look for its files in a directory relative to the PREFIX_PATH. The best way to ensure this is to set the
# SWIG_LIB environment variable.

foreach(path IN LISTS CMAKE_PREFIX_PATH)
  if(NOT DEFINED ENV{SWIG_LIB} AND EXISTS "${path}/bin/swig")
    set(ENV{SWIG_LIB} "${path}/share/swig/CURRENT")
    break()
  endif()
endforeach()

# Set default values for CMake's bundle generator and created Info.plist files
set(MACOSX_BUNDLE_EXECUTABLE_NAME OBS)
set(MACOSX_BUNDLE_BUNDLE_NAME "${OBS_PRODUCT_NAME}")
set(MACOSX_BUNDLE_BUNDLE_VERSION ${OBS_BUILD_NUMBER})
set(MACOSX_BUNDLE_COPYRIGHT "${OBS_LEGAL_COPYRIGHT}")
set(MACOSX_BUNDLE_GUI_IDENTIFIER com.obsproject.obs-studio)
set(MACOSX_BUNDLE_ICON_FILE AppIcon)
set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${OBS_VERSION_CANONICAL})
string(TIMESTAMP CURRENT_YEAR "%Y")

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

include(cpackconfig)
