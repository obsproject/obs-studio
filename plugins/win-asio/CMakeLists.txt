cmake_minimum_required(VERSION 3.28...3.30)

add_library(win-asio MODULE)
add_library(OBS::asio ALIAS win-asio)
set(MODULE_DESCRIPTION "OBS ASIO module")

target_sources(
  win-asio
  PRIVATE
    asio-common.h
    asio-compat.h
    asio-device.c
    asio-device.h
    asio-format.c
    asio-format.h
    asio-device-list.c
    asio-device-list.h
    asio-callbacks.c
    asio-callbacks.h
    byteorder.h
    iasiodrv.h
    plugin-main.c
    win-asio.c
    win-asio.h
)

target_link_libraries(win-asio PRIVATE OBS::libobs OBS::frontend-api)

configure_file(cmake/windows/obs-module.rc.in win-asio.rc)
target_sources(win-asio PRIVATE win-asio.rc)

set_target_properties_obs(win-asio PROPERTIES FOLDER plugins/win-asio PREFIX "")
