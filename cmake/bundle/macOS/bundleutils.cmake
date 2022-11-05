if(POLICY CMP0007)
  cmake_policy(SET CMP0007 NEW)
endif()
if(POLICY CMP0009)
  cmake_policy(SET CMP0009 NEW)
endif()
if(POLICY CMP0011)
  cmake_policy(SET CMP0011 NEW)
endif()

# Add additional search paths for dylibbundler
list(APPEND _FIXUP_BUNDLES
     "-s \"${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks\"")
list(APPEND _FIXUP_BUNDLES "-s \"${CMAKE_INSTALL_PREFIX}/lib\"")
list(APPEND _FIXUP_BUNDLES "-s \"${CMAKE_INSTALL_PREFIX}/Frameworks\"")

foreach(_PREFIX_PATH IN LISTS _DEPENDENCY_PREFIX)
  list(APPEND _FIXUP_BUNDLES "-s \"${_PREFIX_PATH}/lib\"")
  file(GLOB _DYLIBS "${_PREFIX_PATH}/lib/*.dylib")
  file(
    COPY ${_DYLIBS}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks
    FOLLOW_SYMLINK_CHAIN)
  unset(_DYLIBS)
endforeach()

# Unlinked modules need to be supplied manually to dylibbundler

# Find all modules (plugin and standalone)
file(GLOB _OBS_PLUGINS
     "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/PlugIns/*.plugin")
file(GLOB _OBS_SCRIPTING_PLUGINS
     "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/PlugIns/*.so")

# Add modules to fixups
foreach(_OBS_PLUGIN IN LISTS _OBS_PLUGINS)
  get_filename_component(PLUGIN_NAME "${_OBS_PLUGIN}" NAME_WLE)
  list(APPEND _FIXUP_BUNDLES
       "-x \"${_OBS_PLUGIN}/Contents/MacOS/${PLUGIN_NAME}\"")
endforeach()

if(EXISTS
   "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/MacOS/obs-ffmpeg-mux")
  list(
    APPEND
    _FIXUP_BUNDLES
    "-x \"${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/MacOS/obs-ffmpeg-mux\""
  )
endif()

# Add scripting modules to fixups
foreach(_OBS_PLUGIN IN LISTS _OBS_SCRIPTING_PLUGINS)
  list(APPEND _FIXUP_BUNDLES "-x \"${_OBS_PLUGIN}\"")
endforeach()

# Run dylibbbundler
if(DEFINED ENV{VERBOSE})
  set(_VERBOSE_FLAG "--verbose")
endif()

if(DEFINED ENV{QUIET})
  set(_QUIET_FLAG "OUTPUT_QUIET")
endif()

list(REMOVE_DUPLICATES _FIXUP_BUNDLES)
string(REPLACE ";" " " _FIXUP_BUNDLES "${_FIXUP_BUNDLES}")
message(STATUS "OBS: Bundle linked libraries and dependencies")
execute_process(
  COMMAND
    /bin/sh -c
    "${_BUNDLER_COMMAND} -a \"${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}\" -cd -of -q -f ${_FIXUP_BUNDLES} ${_VERBOSE_FLAG}"
    ${_QUIET_FLAG})

# Find all dylibs, frameworks and other code elements inside bundle
file(GLOB _DYLIBS
     "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks/*.dylib")
file(GLOB _FRAMEWORKS
     "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks/*.framework")
file(GLOB_RECURSE _QT_PLUGINS
     "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/PlugIns/*.dylib")

if(EXISTS
   "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/MacOS/obs-ffmpeg-mux")
  list(APPEND _OTHER_BINARIES
       "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/MacOS/obs-ffmpeg-mux")
endif()

if(EXISTS
   "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Resources/obs-mac-virtualcam.plugin"
)
  list(
    APPEND
    _OTHER_BINARIES
    "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Resources/obs-mac-virtualcam.plugin"
  )
endif()

# Create libobs symlink for legacy plugin support
execute_process(
  COMMAND
    /bin/sh -c
    "cd \"${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks\" && ln -fs libobs.framework/Versions/Current/libobs libobs.0.dylib && ln -fsv libobs.framework/Versions/Current/libobs libobs.dylib"
    ${_QUIET_FLAG})

# Python potentially leaves __pycache__ directories inside the bundle which will
# break codesigning
if(EXISTS "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/PlugIns/__pycache__")
  file(REMOVE_RECURSE
       "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/PlugIns/__pycache__")
endif()

# Codesign all binaries inside-out
message(STATUS "OBS: Codesign dependencies")
if(EXISTS
   "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks/Chromium Embedded Framework.framework"
)
  set(CEF_HELPER_OUTPUT_NAME "OBS Helper")
  set(CEF_HELPER_APP_SUFFIXES ":" " (GPU):.gpu" " (Plugin):.plugin"
                              " (Renderer):.renderer")

  foreach(_SUFFIXES ${CEF_HELPER_APP_SUFFIXES})
    string(REPLACE ":" ";" _SUFFIXES ${_SUFFIXES})
    list(GET _SUFFIXES 0 _NAME_SUFFIX)
    list(GET _SUFFIXES 1 _PLIST_SUFFIX)

    set(_HELPER_OUTPUT_NAME "${CEF_HELPER_OUTPUT_NAME}${_NAME_SUFFIX}")
    set(_HELPER_ENTITLEMENT_PLIST "entitlements-helper${_PLIST_SUFFIX}.plist")

    execute_process(
      COMMAND
        /usr/bin/codesign --remove-signature
        "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks/OBS Helper${_NAME_SUFFIX}.app"
        ${_VERBOSE_FLAG} ${_QUIET_FLAG})
    execute_process(
      COMMAND
        /usr/bin/codesign --force --sign "${_CODESIGN_IDENTITY}" --deep
        --options runtime --entitlements
        "${_CODESIGN_ENTITLEMENTS}/entitlements-helper${_PLIST_SUFFIX}.plist"
        "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}/Contents/Frameworks/OBS Helper${_NAME_SUFFIX}.app"
        ${_VERBOSE_FLAG} ${_QUIET_FLAG})
  endforeach()
endif()
foreach(_DEPENDENCY IN LISTS _OTHER_BINARIES _DYLIBS _FRAMEWORKS _OBS_PLUGINS
                             _OBS_SCRIPTING_PLUGINS _QT_PLUGINS)
  if(NOT IS_SYMLINK "${_DEPENDENCY}")
    execute_process(COMMAND /usr/bin/codesign --remove-signature
                            "${_DEPENDENCY}" ${_VERBOSE_FLAG} ${_QUIET_FLAG})
    execute_process(
      COMMAND
        /usr/bin/codesign --force --sign "${_CODESIGN_IDENTITY}" --options
        runtime --entitlements "${_CODESIGN_ENTITLEMENTS}/entitlements.plist"
        "${_DEPENDENCY}" ${_VERBOSE_FLAG} ${_QUIET_FLAG})
  endif()
endforeach()

# Codesign main app
message(STATUS "OBS: Codesign main app")
execute_process(
  COMMAND
    /usr/bin/codesign --remove-signature
    "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}" ${_VERBOSE_FLAG} ${_QUIET_FLAG})
execute_process(
  COMMAND
    /usr/bin/codesign --force --sign "${_CODESIGN_IDENTITY}" --options runtime
    --entitlements "${_CODESIGN_ENTITLEMENTS}/entitlements.plist"
    "${CMAKE_INSTALL_PREFIX}/${_BUNDLENAME}" ${_VERBOSE_FLAG} ${_QUIET_FLAG})
