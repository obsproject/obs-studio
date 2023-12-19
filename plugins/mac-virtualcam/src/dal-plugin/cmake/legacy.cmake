project(mac-dal-plugin)

find_library(COCOA Cocoa)
find_library(COREMEDIA CoreMedia)
find_library(COREMEDIAIO CoreMediaIO)
find_library(COREVIDEO CoreVideo)
find_library(IOSURFACE IOSurface)

mark_as_advanced(COCOA COREMEDIA COREMEDIAIO COREVIDEO IOSURFACE)

add_library(mac-dal-plugin MODULE)
add_library(OBS::mac-dal-plugin ALIAS mac-dal-plugin)

target_sources(
  mac-dal-plugin
  PRIVATE OBSDALPlugIn.mm
          OBSDALPlugIn.h
          OBSDALPlugInMain.mm
          OBSDALPlugInInterface.mm
          OBSDALPlugInInterface.h
          OBSDALObjectStore.mm
          OBSDALObjectStore.h
          OBSDALDevice.mm
          OBSDALDevice.h
          OBSDALMachClient.mm
          OBSDALMachClient.h
          CMSampleBufferUtils.mm
          OBSDALStream.mm
          OBSDALStream.h
          CMSampleBufferUtils.h
          Defines.h
          Logging.h
          ${CMAKE_CURRENT_SOURCE_DIR}/../common/MachProtocol.h)

target_include_directories(mac-dal-plugin PRIVATE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>/../common")

target_compile_options(mac-dal-plugin PRIVATE -fobjc-arc -fobjc-weak)

target_link_libraries(mac-dal-plugin PRIVATE ${COCOA} ${COREMEDIA} ${COREMEDIAIO} ${COREVIDEO} ${IOSURFACE})

set(MACOSX_PLUGIN_BUNDLE_TYPE "BNDL")
target_sources(mac-dal-plugin PRIVATE placeholder.png)
set_source_files_properties(placeholder.png PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
source_group("Resources" FILES placeholder.png)

set_target_properties(
  mac-dal-plugin
  PROPERTIES BUNDLE ON
             BUNDLE_EXTENSION "plugin"
             OUTPUT_NAME "obs-mac-virtualcam"
             FOLDER "plugins"
             VERSION "0"
             SOVERSION "0"
             # Force the DAL plugin to be built for arm64e as well. Note that
             # we cannot build OBS for arm64e, since its libraries are not
             # built for this architecture at the moment.
             OSX_ARCHITECTURES "x86_64;arm64;arm64e"
             LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../"
             MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/bundle/macOS/Virtualcam-Info.plist.in")

setup_binary_target(mac-dal-plugin)
