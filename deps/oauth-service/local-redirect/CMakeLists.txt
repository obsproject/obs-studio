cmake_minimum_required(VERSION 3.16...3.25)

find_qt(COMPONENTS Core Widgets Network)

if(NOT TARGET OBS::oauth)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/oauth" "${CMAKE_BINARY_DIR}/deps/oauth-service/oauth")
endif()

add_library(oauth-local-redirect INTERFACE)
add_library(OBS::oauth-local-redirect ALIAS oauth-local-redirect)

target_sources(oauth-local-redirect INTERFACE oauth-local-redirect.cpp oauth-local-redirect.hpp)
target_include_directories(oauth-local-redirect INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(oauth-local-redirect INTERFACE OBS::libobs OBS::oauth Qt::Core Qt::Widgets Qt::Network)
