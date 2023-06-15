cmake_minimum_required(VERSION 3.16...3.25)

find_package(CURL)
find_package(nlohmann_json)

if(NOT TARGET OBS::curl-helper)
  add_library(curl-helper INTERFACE)
  add_library(OBS::curl-helper ALIAS curl-helper)
  target_sources(curl-helper INTERFACE "${CMAKE_SOURCE_DIR}/libobs/util/curl/curl-helper.h")
  target_include_directories(curl-helper INTERFACE "${CMAKE_SOURCE_DIR}/libobs/util/curl")
  target_link_libraries(curl-helper INTERFACE CURL::libcurl)
endif()

add_library(oauth INTERFACE)
add_library(OBS::oauth ALIAS oauth)

target_sources(oauth INTERFACE oauth.cpp oauth.hpp)
target_include_directories(oauth INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(oauth INTERFACE OBS::curl-helper CURL::libcurl nlohmann_json::nlohmann_json)
