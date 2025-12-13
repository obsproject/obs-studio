cmake_minimum_required(VERSION 3.16)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui)

# Rendering module sources
set(RENDERING_SOURCES
    VulkanRenderer.cpp
    StereoRenderer.cpp
    FramebufferManager.cpp
    STMapLoader.cpp
    PreviewRenderer.cpp
    RTXUpscaler.cpp
    HDRProcessor.cpp
)

set(RENDERING_HEADERS
    VulkanRenderer.h
    StereoRenderer.h
    FramebufferManager.h
    STMapLoader.h
    PreviewRenderer.h
    RTXUpscaler.h
    RTXUpscaler.h
    HDRProcessor.h
    IRenderEngine.h
    OpenXRRuntime.h
)

# Create rendering library
add_library(nstudio-rendering STATIC
    ${RENDERING_SOURCES}
    ${RENDERING_HEADERS}
)

target_link_libraries(nstudio-rendering
    PRIVATE
        Qt6::Core
        Qt6::Gui  # Includes QRhi, QVulkan
)

target_include_directories(nstudio-rendering
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Enable C++20
target_compile_features(nstudio-rendering PUBLIC cxx_std_20)

# Install targets
install(TARGETS nstudio-rendering
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
