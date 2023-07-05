cmake_minimum_required(VERSION 3.16...3.25)

find_qt(COMPONENTS Core Widgets)

if(NOT TARGET OBS::oauth)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/oauth-service/oauth" "${CMAKE_BINARY_DIR}/deps/oauth-service/oauth")
endif()

add_library(oauth-obs-browser-login INTERFACE)
add_library(OBS::oauth-obs-browser-login ALIAS oauth-obs-browser-login)

target_sources(oauth-obs-browser-login INTERFACE oauth-obs-browser-login.cpp oauth-obs-browser-login.hpp)
target_include_directories(oauth-obs-browser-login INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(oauth-obs-browser-login INTERFACE OBS::libobs OBS::frontend-api OBS::oauth Qt::Core Qt::Widgets)
