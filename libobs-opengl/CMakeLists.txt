cmake_minimum_required(VERSION 3.16...3.25)

legacy_check()

add_library(libobs-opengl SHARED)
add_library(OBS::libobs-opengl ALIAS libobs-opengl)

if(NOT TARGET OBS::glad)
  add_subdirectory("${CMAKE_SOURCE_DIR}/deps/glad" "${CMAKE_BINARY_DIR}/deps/glad")
endif()

target_sources(
  libobs-opengl
  PRIVATE gl-helpers.c
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
          gl-zstencil.c)

target_link_libraries(libobs-opengl PRIVATE OBS::libobs OBS::glad)

if(OS_WINDOWS)
  configure_file(cmake/windows/obs-module.rc.in libobs-opengl.rc)

  target_sources(libobs-opengl PRIVATE gl-windows.c libobs-opengl.rc)
elseif(OS_MACOS)
  find_library(COCOA Cocoa)
  find_library(IOSURF IOSurface)

  target_sources(libobs-opengl PRIVATE gl-cocoa.m)
  target_compile_definitions(libobs-opengl PRIVATE GL_SILENCE_DEPRECATION)

  target_link_libraries(libobs-opengl PRIVATE ${COCOA} ${IOSURF})
elseif(OS_LINUX OR OS_FREEBSD)
  find_package(X11 REQUIRED)
  find_package(
    xcb
    COMPONENTS xcb
    REQUIRED)
  find_package(x11-xcb REQUIRED)

  target_sources(libobs-opengl PRIVATE gl-egl-common.c gl-nix.c gl-x11-egl.c)
  target_link_libraries(libobs-opengl PRIVATE xcb::xcb X11::x11-xcb)

  if(ENABLE_WAYLAND)
    find_package(
      OpenGL
      COMPONENTS EGL
      REQUIRED)

    find_package(Wayland REQUIRED)

    target_sources(libobs-opengl PRIVATE gl-wayland-egl.c)
    target_link_libraries(libobs-opengl PRIVATE OpenGL::EGL Wayland::EGL)
  endif()

endif()

target_enable_feature(libobs "OpenGL renderer")

set_target_properties_obs(
  libobs-opengl
  PROPERTIES FOLDER core
             VERSION 0
             PREFIX ""
             SOVERSION "${OBS_VERSION_MAJOR}")
