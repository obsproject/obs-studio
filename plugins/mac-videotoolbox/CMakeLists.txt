project(mac-videotoolbox)

find_library(AVFOUNDATION AVFoundation)
find_library(COCOA Cocoa)
find_library(COREFOUNDATION CoreFoundation)
find_library(COREVIDEO CoreVideo)
find_library(VIDEOTOOLBOX VideoToolbox)
find_library(COREMEDIA CoreMedia)

mark_as_advanced(AVFOUNDATION COCOA COREFOUNDATION COREVIDEO VIDEOTOOLBOX COREMEDIA)

add_library(mac-videotoolbox MODULE)
add_library(OBS::mac-videotoolbox ALIAS mac-videotoolbox)

target_sources(mac-videotoolbox PRIVATE encoder.c)

target_link_libraries(
  mac-videotoolbox
  PRIVATE OBS::libobs
          ${AVFOUNDATION}
          ${COCOA}
          ${COREFOUNDATION}
          ${COREVIDEO}
          ${VIDEOTOOLBOX}
          ${COREMEDIA})

set_target_properties(mac-videotoolbox PROPERTIES FOLDER "plugins" PREFIX "")

setup_plugin_target(mac-videotoolbox)
