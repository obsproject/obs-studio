cmake_minimum_required(VERSION 3.28)
project(vr_ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick Widgets)

add_subdirectory(shared)
add_subdirectory(blueprint)
add_subdirectory(active)
