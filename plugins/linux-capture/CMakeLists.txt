project(linux-capture)

find_package(X11 REQUIRED)
if(NOT TARGET X11::Xcomposite)
  obs_status(FATAL_ERROR "linux-capture - Xcomposite library not found.")
endif()
find_package(XCB COMPONENTS XCB XFIXES RANDR SHM XINERAMA)

add_library(linux-capture MODULE)
add_library(OBS::capture ALIAS linux-capture)

target_sources(
  linux-capture
  PRIVATE linux-capture.c
          xcursor.c
          xcursor.h
          xcursor-xcb.c
          xcursor-xcb.h
          xhelpers.c
          xhelpers.h
          xshm-input.c
          xcomposite-main.cpp
          xcompcap-main.cpp
          xcompcap-main.hpp
          xcompcap-helper.cpp
          xcompcap-helper.hpp)

target_link_libraries(
  linux-capture
  PRIVATE OBS::libobs
          OBS::obsglad
          X11::X11
          X11::Xfixes
          X11::Xcomposite
          XCB::XCB
          XCB::XFIXES
          XCB::RANDR
          XCB::SHM
          XCB::XINERAMA)

set_target_properties(linux-capture PROPERTIES FOLDER "plugins")

setup_plugin_target(linux-capture)
