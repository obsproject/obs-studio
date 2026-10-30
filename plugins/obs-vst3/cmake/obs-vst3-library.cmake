cmake_minimum_required(VERSION 3.28...3.30)

add_library(obs-vst3-library INTERFACE)
add_library(OBS::vst3-library ALIAS obs-vst3-library)

find_package(SteinbergVST3 REQUIRED)

target_sources(obs-vst3-library INTERFACE VST3Scanner.cpp VST3Scanner.h)

target_include_directories(obs-vst3-library INTERFACE "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")

target_link_libraries(obs-vst3-library INTERFACE Steinberg::VST3)
