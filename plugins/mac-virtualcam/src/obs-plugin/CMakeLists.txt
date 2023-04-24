cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

add_library(mac-virtualcam MODULE)
add_library(OBS::virtualcam ALIAS mac-virtualcam)

target_sources(mac-virtualcam PRIVATE Defines.h plugin-main.mm OBSDALMachServer.mm OBSDALMachServer.h)
target_compile_options(mac-virtualcam PRIVATE -fmodules -fcxx-modules)

if(NOT XCODE)
  target_compile_options(mac-virtualcam PRIVATE -fobjc-arc -fobjc-weak)
endif()

target_link_libraries(mac-virtualcam PRIVATE OBS::mach-protocol OBS::libobs OBS::frontend-api)

# cmake-format: off
set_target_properties_obs(
  mac-virtualcam
  PROPERTIES FOLDER plugins
             PREFIX ""
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES
             XCODE_ATTRIBUTE_CLANG_ENABLE_MODULES YES
             XCODE_ATTRIBUTE_CLANG_MODULES_AUTOLINK YES)
# cmake-format: on
