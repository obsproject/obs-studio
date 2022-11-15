project(mac-virtualcam)

find_library(APPKIT AppKit)
find_library(COREVIDEO CoreVideo)
find_library(IOSURFACE IOSurface)
find_library(COREMEDIA CoreMedia)
find_library(COREMEDIAIO CoreMediaIO)
find_library(SYSTEMEXTENSIONS SystemExtensions)

mark_as_advanced(APPKIT COREVIDEO IOSURFACE COREMEDIA COREMEDIAIO
                 SYSTEMEXTENSIONS)

add_library(mac-virtualcam MODULE)
add_library(OBS::virtualcam ALIAS mac-virtualcam)

target_sources(
  mac-virtualcam PRIVATE Defines.h plugin-main.mm OBSDALMachServer.mm
                         OBSDALMachServer.h ../common/MachProtocol.h)

target_include_directories(
  mac-virtualcam
  PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../common")

target_link_libraries(
  mac-virtualcam
  PRIVATE OBS::libobs
          OBS::frontend-api
          ${APPKIT}
          ${COREVIDEO}
          ${IOSURFACE}
          ${COREMEDIA}
          ${COREMEDIAIO}
          ${SYSTEMEXTENSIONS})

target_compile_features(mac-virtualcam PRIVATE cxx_deleted_functions
                                               cxx_rvalue_references cxx_std_17)

target_compile_options(mac-virtualcam PRIVATE -fobjc-arc -fobjc-weak -fmodules
                                              -fcxx-modules)

target_compile_definitions(
  mac-virtualcam PRIVATE "VIRTUALCAM_DEVICE_UUID=${VIRTUALCAM_DEVICE_UUID}")

set_target_properties(
  mac-virtualcam
  PROPERTIES FOLDER "plugins"
             PREFIX ""
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../")

setup_plugin_target(mac-virtualcam)
