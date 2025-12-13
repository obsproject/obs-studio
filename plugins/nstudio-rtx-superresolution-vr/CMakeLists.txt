cmake_minimum_required(VERSION 3.16...3.26)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/common/bootstrap.cmake" NO_POLICY_SCOPE)

project(${_name} VERSION ${_version})

option(ENABLE_FRONTEND_API "Use obs-frontend-api for UI functionality" OFF)
option(ENABLE_QT "Use Qt functionality" OFF)

include(compilerconfig)
include(defaults)
include(helpers)

add_library(${CMAKE_PROJECT_NAME} MODULE)

find_package(libobs REQUIRED)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::libobs)

if(ENABLE_FRONTEND_API)
  find_package(obs-frontend-api REQUIRED)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE OBS::obs-frontend-api)
endif()

if(ENABLE_QT)
  find_package(Qt6 COMPONENTS Widgets Core)
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets)
  target_compile_options(
    ${CMAKE_PROJECT_NAME} PRIVATE $<$<C_COMPILER_ID:Clang,AppleClang>:-Wno-quoted-include-in-framework-header
                                  -Wno-comma>)
  set_target_properties(
    ${CMAKE_PROJECT_NAME}
    PROPERTIES AUTOMOC ON
               AUTOUIC ON
               AUTORCC ON)
endif()

target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/plugin-main.c src/nvidia-superresolution-filter.c src/NVVideoEffectsProxy.cpp src/nvCVImageProxy.cpp)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE src/include/nvCVImage.h src/include/nvCVImageProxy.h src/include/nvCVStatus.h src/include/nvTransferD3D.h src/include/nvTransferD3D11.h src/include/nvvfx.h src/include/nvVideoEffects.h)

if(UNIX AND NOT APPLE)
  find_package(OpenGL REQUIRED)
  find_package(CUDAToolkit REQUIRED)
  
  target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE 
    OpenGL::GL
    CUDA::cudart
    ${CMAKE_DL_LIBS}
  )
  
  target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE 
    ${CUDAToolkit_INCLUDE_DIRS}
  )
endif()

set_target_properties_plugin(${CMAKE_PROJECT_NAME} PROPERTIES OUTPUT_NAME ${_name})
