/******************************************************************************
 Copyright (C) 2024 by Patrick Heyer <PatTheMav@users.noreply.github.com>

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

import AppKit
import Foundation

/// Creates a ``OBSSwapChain`` instance for use as a pseudo swap chain implementation to be shared with `libobs`
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - data: Pointer to platform-specific `gs_init_data` struct
/// - Returns: Opaque pointer to a new ``OBSSwapChain`` on success or `nil` on error
///
/// As interaction with UI elements needs to happen on the main thread of macOS, this function is marked with
/// `@MainActor`. This is also necessary because ``OBSSwapChain/updateView`` itself interacts with the ``NSView``
/// instance passed via the `data` argument and also has to occur on the main thread.
///
/// As applications cannot manage their own swap chain on macOS, the ``OBSSwapChain`` class merely wraps the
/// management of the ``CAMetalLayer`` that will be associated with the ``NSView`` and handles the drawables used to
/// render their contents.
///
/// > Important: This function can only be called from the main thread.
@MainActor
@_cdecl("device_swapchain_create")
public func device_swapchain_create(device: UnsafeMutableRawPointer, data: UnsafePointer<gs_init_data>)
    -> OpaquePointer?
{
    let device: MetalDevice = unretained(device)

    let view = data.pointee.window.view.takeUnretainedValue() as! NSView
    let size = MTLSize(
        width: Int(data.pointee.cx),
        height: Int(data.pointee.cy),
        depth: 0
    )

    guard let swapChain = OBSSwapChain(device: device, size: size, colorSpace: data.pointee.format) else { return nil }

    swapChain.updateView(view)

    device.swapChainQueue.sync {
        device.swapChains.append(swapChain)
    }

    return swapChain.getRetained()
}

/// Updates the internal size parameter and dimension of the ``CAMetalLayer`` managed by the ``OBSSwapChain`` instance
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - width: Width to update the layer's dimensions to
///   - height: Height to update the layer's dimensions to
///
/// As the relationship between the ``CAMetalLayer`` and the ``NSView`` it is associated with is managed indirectly,
/// the metal layer cannot directly react to size changes (even though it would be possible to do so). Instead
/// ``AppKit`` will report a size change to the application, which will be picked up by Qt, who will emit a size
/// change event on the main loop, which will update internal state of the ``OBSQTDisplay`` class. These changes are
/// asynchronously picked up by `libobs` render loop, which will then call this function.
@_cdecl("device_resize")
public func device_resize(device: UnsafeMutableRawPointer, width: UInt32, height: UInt32) {
    let device: MetalDevice = unretained(device)

    guard let swapChain = device.renderState.swapChain else {
        return
    }

    swapChain.resize(.init(width: Int(width), height: Int(height), depth: 0))
}

/// This function does nothing on Metal
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// The intended purpose of this function is to update the render target in the "current" swap chain with the color
/// space of its "display" and thus pick up changes in color spaces between different screens.
///
/// On macOS this just requires updating the EDR headroom for the screen the view might be associated with, as the
/// actual color space and EDR capabilities are evaluated on every render loop.
///
/// > Important: This function can only be called from the main thread.
@_cdecl("device_update_color_space")
public func device_update_color_space(device: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)

    guard device.renderState.swapChain != nil else {
        return
    }

    nonisolated(unsafe) let swapChain = device.renderState.swapChain!

    Task { @MainActor in
        swapChain.updateEdrHeadroom()
    }
}

/// Gets the dimensions of the ``CAMetalLayer`` managed by the ``OBSSwapChain`` instance set up in the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - cx: Pointer to memory for the width of the layer
///   - cy: Pointer to memory for the height of the layer
@_cdecl("device_get_size")
public func device_get_size(
    device: UnsafeMutableRawPointer, cx: UnsafeMutablePointer<UInt32>, cy: UnsafeMutablePointer<UInt32>
) {
    let device: MetalDevice = unretained(device)

    guard let swapChain = device.renderState.swapChain else {
        cx.pointee = 0
        cy.pointee = 0
        return
    }

    cx.pointee = UInt32(swapChain.viewSize.width)
    cy.pointee = UInt32(swapChain.viewSize.height)
}

/// Gets the width of the ``CAMetalLayer`` managed by the ``OBSSwapChain`` instance set up in the current pipeline
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Width of the layer
@_cdecl("device_get_width")
public func device_get_width(device: UnsafeRawPointer) -> UInt32 {
    let device: MetalDevice = unretained(device)

    guard let swapChain = device.renderState.swapChain else {
        return 0
    }

    return UInt32(swapChain.viewSize.width)
}

/// Gets the height of the ``CAMetalLayer`` managed by the ``OBSSwapChain`` instance set up in the current pipeline
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Height of the layer
@_cdecl("device_get_height")
public func device_get_height(device: UnsafeRawPointer) -> UInt32 {
    let device: MetalDevice = unretained(device)

    guard let swapChain = device.renderState.swapChain else {
        return 0
    }

    return UInt32(swapChain.viewSize.height)
}

/// Sets up the ``OBSSwapChain`` for use in the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - swap: Opaque pointer to ``OBSSwapChain`` instance shared with `libobs`
///
/// The first call of this function in any render loop marks the "begin" of OBS Studio's display render stage. There
/// will only ever be one "current" swap chain in use by `libobs` and there is no dedicated call to "reset" or unload
/// the current swap chain, instead a new swap chain is loaded or the "scene end" function is called.
@_cdecl("device_load_swapchain")
public func device_load_swapchain(device: UnsafeRawPointer, swap: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)
    let swapChain: OBSSwapChain = unretained(swap)

    if swapChain.edrHeadroom > 1.0 {
        var videoInfo: obs_video_info = obs_video_info()
        obs_get_video_info(&videoInfo)

        let videoColorSpace = videoInfo.colorspace

        switch videoColorSpace {
        case VIDEO_CS_2100_PQ:
            if swapChain.colorRange != .hdrPQ {
                // TODO: Investigate whether it's viable to use PQ or HLG tone mapping for the preview
                // Use the following code to enable it for either:
                // 2100 PQ:
                //  let maxLuminance = obs_get_video_hdr_nominal_peak_level()
                //  swapChain.layer.edrMetadata = .hdr10(
                //    minLuminance: 0.0001, maxLuminance: maxLuminance, opticalOutputScale: 10000)
                // HLG:
                //  swapChain.layer.edrMetadata = .hlg
                swapChain.layer.pixelFormat = .rgba16Float
                swapChain.layer.colorspace = CGColorSpace(name: CGColorSpace.extendedLinearSRGB)
                swapChain.layer.wantsExtendedDynamicRangeContent = true
                swapChain.layer.edrMetadata = nil
                swapChain.colorRange = .hdrPQ
                swapChain.renderTarget = nil
            }
        case VIDEO_CS_2100_HLG:
            if swapChain.colorRange != .hdrHLG {
                swapChain.layer.pixelFormat = .rgba16Float
                swapChain.layer.colorspace = CGColorSpace(name: CGColorSpace.extendedLinearSRGB)
                swapChain.layer.wantsExtendedDynamicRangeContent = true
                swapChain.layer.edrMetadata = nil
                swapChain.colorRange = .hdrHLG
                swapChain.renderTarget = nil
            }
        default:
            if swapChain.colorRange != .sdr {
                swapChain.layer.pixelFormat = .bgra8Unorm_srgb
                swapChain.layer.colorspace = CGColorSpace(name: CGColorSpace.sRGB)
                swapChain.layer.wantsExtendedDynamicRangeContent = false
                swapChain.layer.edrMetadata = nil
                swapChain.colorRange = .sdr
                swapChain.renderTarget = nil
            }
        }
    } else {
        if swapChain.colorRange != .sdr {
            swapChain.layer.pixelFormat = .bgra8Unorm_srgb
            swapChain.layer.colorspace = CGColorSpace(name: CGColorSpace.sRGB)
            swapChain.layer.wantsExtendedDynamicRangeContent = false
            swapChain.layer.edrMetadata = nil
            swapChain.colorRange = .sdr
            swapChain.renderTarget = nil
        }
    }

    switch swapChain.colorRange {
    case .hdrHLG, .hdrPQ:
        device.renderState.gsColorSpace = GS_CS_709_EXTENDED
        device.renderState.useSRGBGamma = false
    case .sdr:
        device.renderState.gsColorSpace = GS_CS_SRGB
        device.renderState.useSRGBGamma = true
    }

    if let renderTarget = swapChain.renderTarget {
        device.renderState.renderTarget = renderTarget
    } else {
        let descriptor = MTLTextureDescriptor.texture2DDescriptor(
            pixelFormat: swapChain.layer.pixelFormat,
            width: Int(swapChain.layer.drawableSize.width),
            height: Int(swapChain.layer.drawableSize.height),
            mipmapped: false)

        descriptor.usage = [.renderTarget]

        guard let renderTarget = MetalTexture(device: device, descriptor: descriptor) else {
            return
        }

        swapChain.renderTarget = renderTarget
        device.renderState.renderTarget = renderTarget
    }

    device.renderState.depthStencilAttachment = nil
    device.renderState.isRendertargetChanged = true
    device.renderState.isInDisplaysRenderStage = true

    device.renderState.swapChain = swapChain
}

/// Requests deinitialization of the ``OBSSwapChain`` instance shared with `libobs`
/// - Parameter texture: Opaque pointer to ``OBSSwapChain`` instance shared with `libobs`
///
/// The ownership of the shared pointer is transferred into this function and the instance is placed under Swift's
/// memory management again.
@_cdecl("gs_swapchain_destroy")
public func gs_swapchain_destroy(swapChain: UnsafeMutableRawPointer) {
    let swapChain = retained(swapChain) as OBSSwapChain

    swapChain.discard = true
}
