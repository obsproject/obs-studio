cmake_minimum_required(VERSION 3.28...3.30)

find_package(X11 REQUIRED)

find_package(
  XCB
  REQUIRED XCB XFIXES RANDR SHM XINERAMA COMPOSITE
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
  PRIVATE OBS::libobs OBS::glad X11::X11 XCB::XCB XCB::XFIXES XCB::RANDR XCB::SHM XCB::XINERAMA XCB::COMPOSITE
)

set_target_properties_obs(linux-capture PROPERTIES FOLDER plugins PREFIX "")
