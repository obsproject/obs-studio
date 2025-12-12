cmake_minimum_required(VERSION 3.28...3.30)

project(libvr)

# Define the library
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil)
pkg_check_modules(SRT srt)
if(SRT_FOUND)
    add_definitions(-DHAS_SRT)
endif()

include(FetchContent)

FetchContent_Declare(
    OpenXR-SDK
    GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
    GIT_TAG        release-1.0.32 # Use a stable recent tag
)

FetchContent_GetProperties(OpenXR-SDK)
if(NOT OpenXR-SDK_POPULATED)
    FetchContent_Populate(OpenXR-SDK)
    
    # Configure OpenXR Loader options (User requested Wayland only)
    set(BUILD_WITH_STD_FILESYSTEM ON CACHE BOOL "" FORCE)
    set(BUILD_WITH_WAYLAND_HEADERS ON CACHE BOOL "" FORCE)
    set(BUILD_WITH_XCB_HEADERS OFF CACHE BOOL "" FORCE)
    set(BUILD_WITH_X11_HEADERS OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_LOADER ON CACHE BOOL "" FORCE)

    add_subdirectory(${openxr-sdk_SOURCE_DIR} ${openxr-sdk_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# OpenXR-SDK provides targets like 'openxr_loader'
# It typically exports 'openxr_loader' target.


add_library(libvr STATIC
    core/vr_core.c
    scene/SceneManager.cpp
    render/vulkan/VulkanRenderEngine.cpp
    pipeline/FrameRouter.cpp
    enc/ffmpeg/FFmpegEncoderAdapter.cpp
    enc/EncoderManager.cpp
    ml/MLRuntime.cpp
    ml/SuperResolutionAdapter.cpp
    gpu/vulkan_utils.cpp
    transport/SRTAdapter.cpp
    transport/WebTransportAdapter.cpp
    vr/openxr/OpenXRRuntime.cpp
)

target_include_directories(libvr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${FFMPEG_INCLUDE_DIRS}
    ${SRT_INCLUDE_DIRS}
)
target_include_directories(libvr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(libvr PUBLIC
    Vulkan::Vulkan
    openxr_loader
)

target_link_libraries(libvr PRIVATE
    ${FFMPEG_LIBRARIES}
    ${SRT_LIBRARIES}
)

set_target_properties(libvr PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 11
)

# Test Executables

# Render Init Test
add_executable(test_render_init test_render_init.cpp)
target_link_libraries(test_render_init PRIVATE libvr Vulkan::Vulkan)
target_include_directories(test_render_init PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Router Test
add_executable(test_frame_router test_frame_router.cpp)
target_link_libraries(test_frame_router PRIVATE libvr)
target_include_directories(test_frame_router PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Encoder Test
add_executable(test_encode test_encode.cpp)
target_link_libraries(test_encode PRIVATE libvr)
target_include_directories(test_encode PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# SRT Test
add_executable(test_srt test_srt.cpp)
target_link_libraries(test_srt PRIVATE libvr)
target_include_directories(test_srt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# ML Test
add_executable(test_ml_init test_ml_init.cpp)
target_link_libraries(test_ml_init PRIVATE libvr)
target_include_directories(test_ml_init PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Scene Test
add_executable(test_scene_draw test_scene_draw.cpp)
target_link_libraries(test_scene_draw PRIVATE libvr)
target_include_directories(test_scene_draw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Interop Test
add_executable(test_interop test_interop.cpp)
target_link_libraries(test_interop PRIVATE libvr)
target_include_directories(test_interop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# OpenXR Test
add_executable(test_openxr_init test_openxr_init.cpp)
target_link_libraries(test_openxr_init PRIVATE libvr openxr_loader)
target_include_directories(test_openxr_init PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})
