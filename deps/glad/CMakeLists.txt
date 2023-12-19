cmake_minimum_required(VERSION 3.16...3.25)

find_package(OpenGL REQUIRED)

add_library(obsglad STATIC EXCLUDE_FROM_ALL )
add_library(OBS::glad ALIAS obsglad)

if(OBS_CMAKE_VERSION VERSION_LESS 3.0.0)
  add_library(OBS::obsglad ALIAS obsglad)
endif()

target_sources(obsglad PRIVATE src/glad.c include/glad/glad.h)

target_include_directories(obsglad PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")

target_link_libraries(obsglad PUBLIC OpenGL::GL)

if(OS_WINDOWS)
  target_sources(obsglad PRIVATE src/glad_wgl.c include/glad/glad_wgl.h)
elseif(OS_LINUX OR OS_FREEBSD)
  if(TARGET OpenGL::EGL)
    target_sources(obsglad PRIVATE src/glad_egl.c include/EGL/eglplatform.h include/glad/glad_egl.h)

    target_link_libraries(obsglad PRIVATE OpenGL::EGL)
  endif()
endif()

set_target_properties(obsglad PROPERTIES FOLDER deps POSITION_INDEPENDENT_CODE TRUE)
