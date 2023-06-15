cmake_minimum_required(VERSION 3.16...3.25)

if(NOT TARGET OBS::oauth)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/oauth" "${CMAKE_BINARY_DIR}/deps/oauth-service/oauth")
endif()

add_library(oauth-service-base INTERFACE)
add_library(OBS::oauth-service-base ALIAS oauth-service-base)

target_sources(oauth-service-base INTERFACE oauth-service-base.cpp oauth-service-base.hpp)
target_include_directories(oauth-service-base INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(oauth-service-base INTERFACE OBS::oauth OBS::frontend-api)
