cmake_minimum_required(VERSION 3.22...3.25)

find_package(OpenGL REQUIRED)

add_library(obsglad OBJECT)
add_library(OBS::glad ALIAS obsglad)

if(OBS_CMAKE_VERSION VERSION_LESS 3.0.0)
  add_library(OBS::obsglad ALIAS obsglad)
endif()

target_sources(
  obsglad
  PRIVATE
    src/glad.c
    $<$<PLATFORM_ID:Windows>:src/glad_wgl.c>
    $<$<TARGET_EXISTS:OpenGL::EGL>:src/glad_egl.c>
    $<$<TARGET_EXISTS:OpenGL::EGL>:include/EGL/eglplatform.h>
  PUBLIC
    include/glad/glad.h
    "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad_wgl.h>"
    "$<$<TARGET_EXISTS:OpenGL::EGL>:${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad_egl.h>"
)

target_compile_options(obsglad PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-strict-prototypes>)

target_include_directories(obsglad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(
  obsglad
  PRIVATE $<$<NOT:$<PLATFORM_ID:Windows,Darwin>>:${CMAKE_DL_LIBS}>
  PUBLIC OpenGL::GL $<$<TARGET_EXISTS:OpenGL::EGL>:OpenGL::EGL>
)

set_target_properties(obsglad PROPERTIES FOLDER deps POSITION_INDEPENDENT_CODE TRUE)
