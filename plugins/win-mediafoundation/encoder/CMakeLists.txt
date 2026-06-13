cmake_minimum_required(VERSION 3.28...3.30)

add_library(win-mediafoundation-encoder MODULE)
add_library(OBS::win-mediafoundation-encoder ALIAS win-mediafoundation-encoder)

target_sources(
  win-mediafoundation-encoder
  PRIVATE
    mf-av1-encoder.cpp
    mf-av1-encoder.hpp
    mf-av1.cpp
    mf-common.cpp
    mf-common.hpp
    mf-encoder-descriptor.cpp
    mf-encoder-descriptor.hpp
    mf-h264-encoder.cpp
    mf-h264-encoder.hpp
    mf-h264.cpp
    mf-hevc-encoder.cpp
    mf-hevc-encoder.hpp
    mf-hevc.cpp
    mf-plugin.cpp
)

target_link_libraries(
  win-mediafoundation-encoder
  PRIVATE d3d9 dxva2 uuid mfplat mfuuid mf wmcodecdspuuid libobs
)

set_target_properties_obs(win-mediafoundation-encoder PROPERTIES FOLDER "plugins" PREFIX "")
