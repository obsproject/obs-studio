cmake_minimum_required(VERSION 3.28...3.30)

add_library(win-mf MODULE)
add_library(OBS::win-mf ALIAS win-mf)

target_sources(
  win-mf
  PRIVATE
    mf-common.cpp
    mf-encoder-descriptor.cpp
    mf-h264-encoder.cpp
    mf-h264.cpp
    mf-hevc-encoder.cpp
    mf-av1.cpp
    mf-av1-encoder.cpp
    mf-hevc.cpp
    mf-plugin.cpp
    mf-common.hpp
    mf-encoder-descriptor.hpp
    mf-h264-encoder.hpp
    mf-hevc-encoder.hpp
    mf-av1-encoder.hpp
)

target_link_libraries(
  win-mf
  PRIVATE d3d9 dxva2 uuid mfplat mfuuid mf wmcodecdspuuid libobs
)

set_target_properties_obs(win-mf PROPERTIES FOLDER "plugins" PREFIX "")
