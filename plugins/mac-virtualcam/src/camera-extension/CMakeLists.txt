foreach(_uuid IN ITEMS VIRTUALCAM_DEVICE_UUID VIRTUALCAM_SOURCE_UUID VIRTUALCAM_SINK_UUID)
  set(VALID_UUID FALSE)
  if(NOT ${_uuid})
    message(AUTHOR_WARNING "macOS Camera Extension UUID '${_uuid}' is not set, but required for extension.")
    return()
  else()
    check_uuid(${${_uuid}} VALID_UUID)

    if(NOT VALID_UUID)
      message(AUTHOR_WARNING "macos Camera Extension UUID '${_uuid}' is not a valid UUID.")
      return()
    endif()
  endif()
endforeach()

project(mac-camera-extension LANGUAGES Swift)

set(_ORIG_DEPLOYMENT_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})

set(CMAKE_OSX_DEPLOYMENT_TARGET 13.0)

add_executable(mac-camera-extension)
add_executable(OBS:mac-camera-extension ALIAS mac-camera-extension)

set(_placeholder_location "${CMAKE_CURRENT_SOURCE_DIR}/../common/data/placeholder.png")
target_sources(
  mac-camera-extension PRIVATE main.swift OBSCameraDeviceSource.swift OBSCameraProviderSource.swift
                               OBSCameraStreamSink.swift OBSCameraStreamSource.swift "${_placeholder_location}")

set_property(SOURCE "${_placeholder_location}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources")
source_group("Resources" FILES "${_placeholder_location}")

set_target_properties(
  mac-camera-extension
  PROPERTIES BUNDLE_EXTENSION systemextension
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
             MACOSX_BUNDLE ON
             MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
             XCODE_PRODUCT_TYPE com.apple.product-type.system-extension
             XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/entitlements.plist"
             XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME YES
             XCODE_ATTRIBUTE_CODE_SIGN_INJECT_BASE_ENTITLEMENTS NO
             XCODE_ATTRIBUTE_OTHER_CODE_SIGN_FLAGS "--timestamp"
             XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET 13.0
             XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.obsproject.obs-studio.mac-camera-extension"
             XCODE_ATTRIBUTE_SWIFT_VERSION 5
             XCODE_ATTRIBUTE_SKIP_INSTALL YES)

set_target_properties_obs(mac-camera-extension PROPERTIES FOLDER plugins
                                                          OUTPUT_NAME "com.obsproject.obs-studio.mac-camera-extension")
