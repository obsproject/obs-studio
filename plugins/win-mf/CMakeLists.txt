cmake_minimum_required(VERSION 3.12)

project(win-mf)

add_library(win-mf MODULE)

target_sources(
  win-mf
  PRIVATE
    mf-plugin.cpp
    mf-common.cpp
    mf-encoder-descriptor.cpp
    mf-common.hpp
    mf-encoder-descriptor.hpp
)

target_include_directories(win-mf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(
  win-mf
  PRIVATE d3d9 dxva2 uuid mfplat mfuuid mf wmcodecdspuuid libobs
)

set_target_properties(win-mf PROPERTIES FOLDER "plugins" PREFIX "")
