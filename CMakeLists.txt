cmake_minimum_required(VERSION 3.28)

project(obs-vr VERSION 0.1.0 LANGUAGES C CXX)

# Standard Settings
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Modules
add_subdirectory(libvr)
add_subdirectory(obs-vr)