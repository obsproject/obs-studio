cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

find_package(X11 REQUIRED)

find_package(
  Xcb
  REQUIRED xcb xcb-xfixes xcb-randr xcb-shm xcb-xinerama xcb-composite
)

add_library(linux-capture MODULE)
add_library(OBS::capture ALIAS linux-capture)

target_sources(
  linux-capture
  PRIVATE
    linux-capture.c
    xcomposite-input.c
    xcomposite-input.h
    xcursor-xcb.c
    xcursor-xcb.h
    xhelpers.c
    xhelpers.h
    xshm-input.c
)

target_link_libraries(
  linux-capture
  PRIVATE
    OBS::libobs
    OBS::glad
    X11::X11
    xcb::xcb
    xcb::xcb-xfixes
    xcb::xcb-randr
    xcb::xcb-shm
    xcb::xcb-xinerama
    xcb::xcb-composite
)

set_target_properties_obs(linux-capture PROPERTIES FOLDER plugins PREFIX "")
