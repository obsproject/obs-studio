cmake_minimum_required(VERSION 3.28...3.30)

project(libvr)

# Define the library
find_package(Vulkan REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavutil libavformat libswscale)
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
    core/GraphicsCompat.cpp 
    scene/SceneManager.cpp
    render/vulkan/VulkanRenderEngine.cpp
    scene/OBJLoader.cpp
    pipeline/FrameRouter.cpp
    enc/ffmpeg/FFmpegEncoderAdapter.cpp
    enc/EncoderManager.cpp
    ml/MLRuntime.cpp
    ml/SuperResolutionAdapter.cpp
    gpu/vulkan_utils.cpp
    gpu/cuda_utils.cpp
    transport/SRTAdapter.cpp
    transport/WebTransportAdapter.cpp
    transport/RTMPAdapter.cpp
    vr/openxr/OpenXRRuntime.cpp
    stitch/Stitcher.cpp
    external/nvidia/NVVideoEffectsProxy.cpp
    external/pipewire/PipeWireProxy.cpp
    input/PipeWireSource.cpp
    input/V4L2Source.cpp
    input/BrowserSource.cpp
    input/CEFBrowserSource.cpp
    input/MediaSource.cpp
    output/VirtualCamOutput.cpp
    audio/AudioManager.cpp
    core/ObsCompat.cpp
    ipc/IPCServer.cpp
    
    # Shared Media Playback
    ../shared/media-playback/media-playback/media.c
    ../shared/media-playback/media-playback/decode.c
    ../shared/media-playback/media-playback/cache.c
    
    # Shared BPM
    ../shared/bpm/bpm.c
    ../deps/libcaption/src/mpeg.c
    ../deps/libcaption/src/caption.c
    ../deps/libcaption/src/cea708.c
    ../deps/libcaption/src/eia608.c
    ../deps/libcaption/src/utf8.c
    ../deps/libcaption/src/eia608_from_utf8.c
    ../deps/libcaption/src/eia608_charmap.c
    ../deps/libcaption/src/xds.c
    ../libobs/util/array-serializer.c
    ../libobs/util/dstr.c
    ../libobs/obs-av1.c
    
    # Shared Happy Eyeballs
    ../shared/happy-eyeballs/happy-eyeballs.c

    # Shared IPC Util
    ../shared/ipc-util/ipc-util/pipe-posix.c

    # Shared Media Playback
    ../shared/media-playback/media-playback/media.c
    ../shared/media-playback/media-playback/decode.c
    ../shared/media-playback/media-playback/media-playback.c
    ../shared/media-playback/media-playback/cache.c

    # Shared Scripting (Core)
    ../shared/obs-scripting/obs-scripting.c
    ../shared/obs-scripting/obs-scripting-logging.c

    # Shared Opts Parser
    ../shared/opts-parser/opts-parser.c

    # Shared Tiny NV12 Scale
    ../shared/obs-tiny-nv12-scale/tiny-nv12-scale.c

    # Shared File Updater
    ../shared/file-updater/file-updater/file-updater.c

    # Plugin: Image Source
    ../plugins/image-source/image-source.c
    ../plugins/image-source/color-source.c
)
set_source_files_properties(../plugins/image-source/image-source.c PROPERTIES COMPILE_DEFINITIONS "obs_module_load=image_source_module_load")

target_include_directories(libvr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${FFMPEG_INCLUDE_DIRS}
    ${SRT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/external/nvidia/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/pipewire
    ${CMAKE_CURRENT_SOURCE_DIR}/external/cef_minimal
    
    # OBS Core Includes
    ${CMAKE_SOURCE_DIR}/libobs
    ${CMAKE_SOURCE_DIR}/shared/media-playback/media-playback
    ${CMAKE_SOURCE_DIR}/shared/bpm
    ${CMAKE_SOURCE_DIR}/deps/libcaption
    ${CMAKE_SOURCE_DIR}/deps/libcaption/caption
    ${CMAKE_SOURCE_DIR}/shared/file-updater/file-updater
    ${CMAKE_SOURCE_DIR}/shared/happy-eyeballs
    ${CMAKE_SOURCE_DIR}/shared/ipc-util/ipc-util
    ${CMAKE_SOURCE_DIR}/shared/media-playback/media-playback
    ${CMAKE_SOURCE_DIR}/shared/obs-scripting
    ${CMAKE_SOURCE_DIR}/shared/opts-parser
    ${CMAKE_SOURCE_DIR}/shared/obs-tiny-nv12-scale
    ${CMAKE_CURRENT_SOURCE_DIR}/shim
)
target_include_directories(libvr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(libvr PUBLIC
    Vulkan::Vulkan
    OpenXR::openxr_loader
    ${FFMPEG_LIBRARIES}
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

# Linux ALSA Integration
find_package(ALSA QUIET)

if(NOT ALSA_FOUND)
    message(STATUS "ALSA not found, using Mock implementation.")
    set(ALSA_FOUND TRUE)
    set(ALSA_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/shim") 
    # Link mock object/lib later
    # We will add alsa-mock.c to the library sources directly 
    # instead of creating a fake target, for simplicity.
    set(USE_ALSA_MOCK TRUE)
endif()

if(ALSA_FOUND)
    target_sources(libvr PRIVATE
        "${CMAKE_SOURCE_DIR}/plugins/linux-alsa/linux-alsa.c"
        "${CMAKE_SOURCE_DIR}/plugins/linux-alsa/alsa-input.c"
    )
    if (USE_ALSA_MOCK)
        target_sources(libvr PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/shim/alsa-mock.c")
    endif()

    if(NOT USE_ALSA_MOCK)
        target_link_libraries(libvr PUBLIC ${ALSA_LIBRARIES})
    endif()
    
    target_include_directories(libvr PUBLIC ${ALSA_INCLUDE_DIRS})
    message(STATUS "ALSA Integration Enabled (Mock: ${USE_ALSA_MOCK})")
endif()
add_executable(test_interop test_interop.cpp)
target_link_libraries(test_interop PRIVATE libvr)
target_include_directories(test_interop PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# OpenXR Test
add_executable(test_openxr_init test_openxr_init.cpp)
target_link_libraries(test_openxr_init PRIVATE libvr openxr_loader)
target_include_directories(test_openxr_init PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Media Test
add_executable(test_media_source test_media_source.cpp)
target_link_libraries(test_media_source PRIVATE libvr)
target_include_directories(test_media_source PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# BPM Test
add_executable(test_bpm test_bpm.cpp)
target_link_libraries(test_bpm PRIVATE libvr)
target_include_directories(test_bpm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# File Updater Test
add_executable(test_file_updater test_file_updater.cpp)
target_link_libraries(test_file_updater PRIVATE libvr)

# Test Happy Eyeballs
add_executable(test_happy_eyeballs test_happy_eyeballs.cpp)
target_link_libraries(test_happy_eyeballs PRIVATE libvr)
target_include_directories(test_happy_eyeballs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Test IPC Util
add_executable(test_ipc_util test_ipc_util.cpp)
target_link_libraries(test_ipc_util PRIVATE libvr)
target_include_directories(test_ipc_util PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Test Media Playback
add_executable(test_media_playback test_media_playback.cpp)
target_link_libraries(test_media_playback PRIVATE libvr)
target_include_directories(test_media_playback PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Test Scripting
add_executable(test_scripting test_scripting.cpp)
target_link_libraries(test_scripting PRIVATE libvr)
target_include_directories(test_scripting PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Test Remaining Shared
add_executable(test_remaining_shared test_remaining_shared.cpp)
target_link_libraries(test_remaining_shared PRIVATE libvr)
target_include_directories(test_remaining_shared PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Test 3D Scene
add_executable(test_3d_scene test_3d_scene.cpp)
target_link_libraries(test_3d_scene PRIVATE libvr)
target_include_directories(test_3d_scene PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

# Test Plugin Image
add_executable(test_plugin_image test_plugin_image.cpp)
target_link_libraries(test_plugin_image PRIVATE libvr)
target_include_directories(test_plugin_image PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR})

if(ALSA_FOUND)
    add_executable(test_plugin_alsa test_plugin_alsa.cpp)
    target_link_libraries(test_plugin_alsa PRIVATE libvr)
endif()
