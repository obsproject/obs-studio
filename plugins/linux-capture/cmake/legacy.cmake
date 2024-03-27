project(linux-capture)

find_package(X11 REQUIRED)
find_package(XCB COMPONENTS XCB XFIXES RANDR SHM XINERAMA COMPOSITE)
if(NOT TARGET XCB::COMPOSITE)
  obs_status(FATAL_ERROR "xcb composite library not found")
endif()

add_library(linux-capture MODULE)
add_library(OBS::capture ALIAS linux-capture)

target_sources(
  linux-capture
  PRIVATE linux-capture.c
          xcursor-xcb.c
          xcursor-xcb.h
          xhelpers.c
          xhelpers.h
          xshm-input.c
          xcomposite-input.c
          xcomposite-input.h)

target_link_libraries(
  linux-capture
  PRIVATE OBS::libobs
          OBS::obsglad
          X11::X11
          XCB::XCB
          XCB::XFIXES
          XCB::RANDR
          XCB::SHM
          XCB::XINERAMA
          XCB::COMPOSITE)

set_target_properties(linux-capture PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-capture)
