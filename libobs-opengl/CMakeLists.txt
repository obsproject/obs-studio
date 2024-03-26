cmake_minimum_required(VERSION 3.22...3.25)

legacy_check()

add_library(libobs-opengl SHARED)
add_library(OBS::libobs-opengl ALIAS libobs-opengl)

if(NOT TARGET OBS::glad)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/glad" "${CMAKE_BINARY_DIR}/deps/glad")
endif()

if(OS_LINUX OR OS_FREEBSD OR OS_OPENBSD)
  find_package(X11 REQUIRED)
  find_package(Xcb REQUIRED xcb)
  find_package(X11-xcb REQUIRED)

  if(ENABLE_WAYLAND)
    find_package(OpenGL REQUIRED EGL)
    find_package(Wayland REQUIRED)
  endif()
endif()

target_sources(
  libobs-opengl
  PRIVATE
    $<$<AND:$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>,$<BOOL:${ENABLE_WAYLAND}>>:gl-wayland-egl.c>
    $<$<PLATFORM_ID:Darwin>:gl-cocoa.m>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:gl-egl-common.c>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:gl-nix.c>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:gl-x11-egl.c>
    $<$<PLATFORM_ID:Windows>:gl-windows.c>
    gl-helpers.c
    gl-helpers.h
    gl-indexbuffer.c
    gl-shader.c
    gl-shaderparser.c
    gl-shaderparser.h
    gl-stagesurf.c
    gl-subsystem.c
    gl-subsystem.h
    gl-texture2d.c
    gl-texture3d.c
    gl-texturecube.c
    gl-vertexbuffer.c
    gl-zstencil.c
)

target_compile_options(libobs-opengl PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-strict-prototypes>)

target_compile_definitions(libobs-opengl PRIVATE $<$<PLATFORM_ID:Darwin>:GL_SILENCE_DEPRECATION>)

target_link_libraries(
  libobs-opengl
  PRIVATE
    OBS::libobs
    OBS::glad
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,Cocoa.framework>>"
    "$<$<PLATFORM_ID:Darwin>:$<LINK_LIBRARY:FRAMEWORK,IOSurface.framework>>"
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:xcb::xcb>
    $<$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>:X11::x11-xcb>
    $<$<AND:$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>,$<BOOL:${ENABLE_WAYLAND}>>:OpenGL::EGL>
    $<$<AND:$<PLATFORM_ID:Linux,FreeBSD,OpenBSD>,$<BOOL:${ENABLE_WAYLAND}>>:Wayland::EGL>
)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in libobs-opengl.rc)
  target_sources(libobs-opengl PRIVATE libobs-opengl.rc)
endif()

target_enable_feature(libobs "OpenGL renderer")

set_target_properties_obs(
  libobs-opengl
  PROPERTIES FOLDER core
             VERSION 0
             PREFIX ""
             SOVERSION "${OBS_VERSION_MAJOR}"
)
