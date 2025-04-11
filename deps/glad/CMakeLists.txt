cmake_minimum_required(VERSION 3.28...3.30)

find_package(OpenGL REQUIRED)

add_library(obsglad OBJECT)
add_library(OBS::glad ALIAS obsglad)

find_package(PkgConfig REQUIRED)
pkg_check_modules(EGL QUIET egl)
target_link_libraries(obsglad PUBLIC ${EGL_LIBRARIES})
target_include_directories(obsglad PUBLIC ${EGL_INCLUDE_DIRS})
target_compile_options(obsglad PUBLIC ${EGL_CFLAGS_OTHER})

set(GLAD_SOURCES_PRIVATE
  src/glad.c
  "$<$<PLATFORM_ID:Windows>:src/glad_wgl.c>")
set(GLAD_SOURCES_PUBLIC
  include/glad/glad.h
  "$<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad_wgl.h>")

if(EGL_FOUND)
  list(APPEND GLAD_SOURCES_PRIVATE
    src/glad_egl.c
    include/EGL/eglplatform.h)
  list(APPEND GLAD_SOURCES_PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include/glad/glad_egl.h")
endif()

target_sources(
  obsglad
  PRIVATE
    ${GLAD_SOURCES_PRIVATE}
  PUBLIC
    ${GLAD_SOURCES_PUBLIC}
)

target_compile_options(obsglad PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-strict-prototypes>)

target_include_directories(obsglad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(
  obsglad
  PRIVATE $<$<NOT:$<PLATFORM_ID:Windows,Darwin>>:${CMAKE_DL_LIBS}>
  PUBLIC OpenGL::GL
)

set_target_properties(obsglad PROPERTIES FOLDER deps POSITION_INDEPENDENT_CODE TRUE)
