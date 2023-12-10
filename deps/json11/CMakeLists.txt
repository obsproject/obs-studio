cmake_minimum_required(VERSION 3.22...3.25)

add_library(json11 INTERFACE)
add_library(OBS::json11 ALIAS json11)

target_include_directories(json11 INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(json11 INTERFACE $<$<COMPILE_LANG_AND_ID:CXX,AppleClang,Clang>:-Wno-unqualified-std-cast-call>)

target_sources(json11 INTERFACE json11.cpp json11.hpp)
