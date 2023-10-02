cmake_minimum_required(VERSION 3.22...3.25)

add_library(json11 OBJECT)
add_library(OBS::json11 ALIAS json11)

target_sources(
  json11
  PRIVATE json11.cpp
  PUBLIC json11.hpp)

target_include_directories(json11 PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

if(CMAKE_CXX_COMPILER_ID MATCHES "(AppleClang|Clang)" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14)
  target_compile_options(json11 PUBLIC -Wno-unqualified-std-cast-call)
endif()

set_target_properties(json11 PROPERTIES FOLDER deps)
