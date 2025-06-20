cmake_minimum_required(VERSION 3.28...3.30)

add_library(libobs-metal SHARED)
add_library(OBS::libobs-metal ALIAS libobs-metal)

target_sources(
  libobs-metal
  PRIVATE
    CVPixelFormat+Extensions.swift
    MTLCullMode+Extensions.swift
    MTLOrigin+Extensions.swift
    MTLPixelFormat+Extensions.swift
    MTLRegion+Extensions.swift
    MTLSize+Extensions.swift
    MTLTexture+Extensions.swift
    MTLTextureDescriptor+Extensions.swift
    MTLTextureType+Extensions.swift
    MTLViewport+Extensions.swift
    MetalBuffer.swift
    MetalDevice.swift
    MetalError.swift
    MetalRenderState.swift
    MetalShader+Extensions.swift
    MetalShader.swift
    MetalStageBuffer.swift
    MetalTexture.swift
    OBSShader.swift
    OBSSwapChain.swift
    Sequence+Hashable.swift
    libobs+Extensions.swift
    libobs+SignalHandlers.swift
    libobs-metal-Bridging-Header.h
    metal-indexbuffer.swift
    metal-samplerstate.swift
    metal-shader.swift
    metal-stagesurf.swift
    metal-subsystem.swift
    metal-swapchain.swift
    metal-texture2d.swift
    metal-texture3d.swift
    metal-unimplemented.swift
    metal-vertexbuffer.swift
    metal-zstencilbuffer.swift
)

target_link_libraries(libobs-metal PRIVATE OBS::libobs)

target_enable_feature(libobs "Metal renderer")

set_property(SOURCE OBSMetalRenderer.swift APPEND PROPERTY COMPILE_FLAGS -emit-objc-header)

set_target_properties_obs(
    libobs-metal
    PROPERTIES FOLDER core
               VERSION 0
               PREFIX ""
)

set_target_xcode_properties(
    libobs-metal
    PROPERTIES SWIFT_VERSION 6.0
               CLANG_ENABLE_OBJC_ARC YES
               CLANG_WARN_SUSPICIOUS_IMPLICIT_CONVERSION YES
               GCC_WARN_SHADOW YES
               CLANG_ENABLE_MODULES YES
               CLANG_MODULES_AUTOLINK YES
               GCC_STRICT_ALIASING YES
               DEFINES_MODULE YES
               SWIFT_OBJC_BRIDGING_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/libobs-metal-Bridging-Header.h"
)
