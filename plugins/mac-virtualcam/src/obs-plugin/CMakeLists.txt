cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

find_library(APPKIT AppKit)
find_library(COREVIDEO CoreVideo)
find_library(IOSURFACE IOSurface)
mark_as_advanced(APPKIT COREVIDEO IOSURFACE)

add_library(mac-virtualcam MODULE)
add_library(OBS::virtualcam ALIAS mac-virtualcam)

target_sources(mac-virtualcam PRIVATE Defines.h plugin-main.mm OBSDALMachServer.mm OBSDALMachServer.h)
target_compile_features(mac-virtualcam PRIVATE cxx_deleted_functions cxx_rvalue_references cxx_std_17)
if(NOT XCODE)
  target_compile_options(mac-virtualcam PRIVATE -fobjc-arc -fobjc-weak)
endif()

target_link_libraries(mac-virtualcam PRIVATE OBS::mach-protocol OBS::libobs OBS::frontend-api ${APPKIT} ${COREVIDEO}
                                             ${IOSURFACE})

set_target_properties_obs(
  mac-virtualcam
  PROPERTIES FOLDER plugins
             PREFIX ""
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
             XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
             XCODE_ATTRIBUTE_CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
             XCODE_ATTRIBUTE_GCC_WARN_SHADOW YES)
