cmake_minimum_required(VERSION 3.12)

project(win-mf)

add_library(win-mf MODULE)

target_sources(
  win-mf
  PRIVATE
    mf-plugin.cpp
    mf-common.cpp
    mf-encoder-descriptor.cpp
    mf-h264.cpp
    mf-h264-encoder.cpp
    mf-hevc.cpp
    mf-hevc-encoder.cpp
    mf-aac.cpp
    mf-aac-encoder.cpp
    mf-common.hpp
    mf-encoder-descriptor.hpp
    mf-h264-encoder.hpp
    mf-hevc-encoder.hpp
    mf-aac-encoder.hpp
)

target_include_directories(win-mf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  win-mf
  PRIVATE d3d9 dxva2 uuid mfplat mfuuid mf wmcodecdspuuid libobs
)

set_target_properties(win-mf PROPERTIES FOLDER "plugins" PREFIX "")
