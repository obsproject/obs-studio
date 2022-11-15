if(NOT VIRTUALCAM_DEVICE_UUID)
  obs_status(
    WARNING
    "macOS Camera Extension - Device UUID not set - specify as 'VIRTUALCAM_DEVICE_UUID' to enable."
  )
  return()
elseif(NOT VIRTUALCAM_STREAM_UUID)
  obs_status(
    WARNING
    "macOS Camera Extension - Stream UUID not set - specify as 'VIRTUALCAM_STREAM_UUID' to enable."
  )
  return()
elseif(NOT VIRTUALCAM_SINK_UUID)
  obs_status(
    WARNING
    "macOS Camera Extension - StreamSink UUID not set - specify as 'VIRTUALCAM_SINK_UUID' to enable."
  )
  return()
else()
  set(VALID_UUID OFF)
  check_uuid(${VIRTUALCAM_DEVICE_UUID} VALID_UUID)

  if(NOT VALID_UUID)
    obs_status(WARNING "macOS Camera Extension - invalid device UUID specified")
    return()
  endif()

  set(VALID_UUID OFF)
  check_uuid(${VIRTUALCAM_STREAM_UUID} VALID_UUID)

  if(NOT VALID_UUID)
    obs_status(WARNING "macOS Camera Extension - invalid stream UUID specified")
    return()
  endif()

  set(VALID_UUID OFF)
  check_uuid(${VIRTUALCAM_SINK_UUID} VALID_UUID)

  if(NOT VALID_UUID)
    obs_status(WARNING
               "macOS Camera Extension - invalid stream sink UUID specified")
    return()
  endif()
endif()

set(_ORIG_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})

set(CMAKE_OSX_DEPLOYMENT_TARGET 12.3)
project(mac-camera-extension LANGUAGES Swift)

find_library(FOUNDATION Foundation)
find_library(COREMEDIAIO CoreMediaIO)
find_library(IOKIT IOKit)

mark_as_advanced(FOUNDATION COREMEDIAIO IOKIT)

add_executable(mac-camera-extension)
add_executable(OBS:mac-camera-extension ALIAS mac-camera-extension)

target_sources(
  mac-camera-extension
  PRIVATE main.swift OBSCameraDeviceSource.swift OBSCameraProviderSource.swift
          OBSCameraStreamSink.swift OBSCameraStreamSource.swift)

target_link_libraries(mac-camera-extension PRIVATE ${FOUNDATION} ${COREMEDIAIO}
                                                   ${IOKIT})

target_sources(
  mac-camera-extension
  PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/placeholder.png")
set_source_files_properties(
  "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/placeholder.png"
  PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
source_group(
  "Resources"
  FILES "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/placeholder.png")

set_target_properties(
  mac-camera-extension
  PROPERTIES
    BUILD_WITH_INSTALL_RPATH ON
    BUNDLE_EXTENSION systemextension
    EXECUTABLE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
    FOLDER plugins
    OUTPUT_NAME "com.obsproject.obs-studio.mac-camera-extension"
    Swift_LANGUAGE_VERSION 5
    MACOSX_BUNDLE ON
    MACOSX_BUNDLE_INFO_PLIST
    "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/mac-camera-extension-Info.plist.in"
    XCODE_PRODUCT_TYPE com.apple.product-type.system-extension
    XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../resources/mac-camera-extension-Entitlements.plist"
    XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
    XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS NO
    XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp"
    XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 13.0
    XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER
    "com.obsproject.obs-studio.mac-camera-extension"
    XCODE_ATTRIBUTE_SWIFT_VERSION 5.0
    XCODE_ATTRIBUTE_SKIP_INSTALL YES)

set(MACOSX_PLUGIN_BUNDLE_TYPE "SYSX")
set(MACOSX_PLUGIN_BUNDLE_NAME "com.obsproject.obs-studio.mac-camera-extension")
set(MACOSX_PLUGIN_GUI_IDENTIFIER
    "com.obsproject.obs-studio.mac-camera-extension")
set(MACOSX_PLUGIN_BUNDLE_VERSION "${MACOSX_BUNDLE_BUNDLE_VERSION}")
set(MACOSX_PLUGIN_SHORT_VERSION_STRING "${MACOSX_BUNDLE_SHORT_VERSION_STRING}")
set(MACOSX_PLUGIN_EXECUTABLE_NAME
    "com.obsproject.obs-studio.mac-camera-extension")

set(CMAKE_OSX_DEPLOYMENT_TARGET "${_ORIG_DEPLOYMENT_TARGET}")
unset(_ORIG_DEPLOYMENT_TARGET)
