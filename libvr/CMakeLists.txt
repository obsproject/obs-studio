cmake_minimum_required(VERSION 3.28...3.30)

project(libvr)

# Define the library
find_package(Vulkan REQUIRED)

add_library(libvr STATIC
    core/vr_core.c
    scene/SceneManager.cpp
    scene/SceneManager.cpp
    render/vulkan/VulkanRenderEngine.cpp
    pipeline/FrameRouter.cpp
)

target_include_directories(libvr PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(libvr PRIVATE
    Vulkan::Vulkan
)

set_target_properties(libvr PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    C_STANDARD 11
)


# Test Executable
add_executable(test_render_init test_render_init.cpp)
target_link_libraries(test_render_init PRIVATE libvr Vulkan::Vulkan)
target_include_directories(test_render_init PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Router Test
add_executable(test_frame_router test_frame_router.cpp)
target_link_libraries(test_frame_router PRIVATE libvr)
target_include_directories(test_frame_router PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

