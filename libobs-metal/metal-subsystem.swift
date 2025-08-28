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

import Foundation
import Metal
import simd

@inlinable
public func unretained<Instance>(_ pointer: UnsafeRawPointer) -> Instance where Instance: AnyObject {
    Unmanaged<Instance>.fromOpaque(pointer).takeUnretainedValue()
}

@inlinable
public func retained<Instance>(_ pointer: UnsafeRawPointer) -> Instance where Instance: AnyObject {
    Unmanaged<Instance>.fromOpaque(pointer).takeRetainedValue()
}

@inlinable
public func OBSLog(_ level: OBSLogLevel, _ format: String, _ args: CVarArg...) {
    let logMessage = String.localizedStringWithFormat(format, args)

    logMessage.withCString { cMessage in
        withVaList([cMessage]) { arguments in
            blogva(level.rawValue, "%s", arguments)
        }
    }
}

/// Returns the graphics API name implemented by the "device".
/// - Returns: Constant pointer to a C string with the API name
///
@_cdecl("device_get_name")
public func device_get_name() -> UnsafePointer<CChar> {
    return device_name
}

/// Gets the graphics API identifier number for the "device".
/// - Returns: Numerical identifier
///
@_cdecl("device_get_type")
public func device_get_type() -> Int32 {
    return GS_DEVICE_METAL
}

/// Returns a string to be used as a suffix for libobs' shader preprocessor, which will be used as part of a shaders
/// identifying information.
/// - Returns: Constant pointer to a C string with the suffix text
@_cdecl("device_preprocessor_name")
public func device_preprocessor_name() -> UnsafePointer<CChar> {
    return preprocessor_name
}

/// Creates a new Metal device instance and stores an opaque pointer to a ``MetalDevice`` instance in the provided
/// pointer.
///
/// - Parameters:
///   - devicePointer: Pointer to memory allocated by the caller to receive the pointer of the create device instance
///   - adapter: Numerical identifier of a graphics display adaptor to create the device on.
/// - Returns: Device creation result value defined as preprocessor macro in libobs' graphics API header
///
/// This method will increment the reference count on the created ``MetalDevice`` instance to ensure it will not be
/// deallocated until `libobs` actively relinquishes ownership of it via a call of  `device_destroy`.
///
/// > Important: As the Metal API is only supported on Apple Silicon devices, the adapter argument is effectively
/// ignored (there is only ever one "adapter" in an Apple Silicon machine and thus only the "default" device is used.
@_cdecl("device_create")
public func device_create(devicePointer: UnsafeMutableRawPointer, adapter: UInt32) -> Int32 {
    guard NSProtocolFromString("MTLDevice") != nil else {
        OBSLog(.error, "This Mac does not support Metal.")
        return GS_ERROR_NOT_SUPPORTED
    }

    OBSLog(.info, "---------------------------------")

    guard let metalDevice = MTLCreateSystemDefaultDevice() else {
        OBSLog(.error, "Unable to initialize Metal device.")
        return GS_ERROR_FAIL
    }

    var descriptions: [String] = []

    descriptions.append("Initializing Metal...")
    descriptions.append("\t- Name               : \(metalDevice.name)")
    descriptions.append("\t- Unified Memory     : \(metalDevice.hasUnifiedMemory ? "Yes" : "No")")
    descriptions.append("\t- Raytracing Support : \(metalDevice.supportsRaytracing ? "Yes" : "No")")

    if #available(macOS 14.0, *) {
        descriptions.append("\t- Architecture       : \(metalDevice.architecture.name)")
    }

    OBSLog(.info, descriptions.joined(separator: "\n"))

    do {
        let device = try MetalDevice(device: metalDevice)

        let retained = Unmanaged.passRetained(device).toOpaque()

        let signalName = MetalSignalType.videoReset.rawValue
        let signalHandler = obs_get_signal_handler()
        signalName.withCString {
            signal_handler_connect(signalHandler, $0, metal_video_reset_handler, retained)
        }

        devicePointer.storeBytes(of: OpaquePointer(retained), as: OpaquePointer.self)
    } catch {
        OBSLog(.error, "Unable to create MetalDevice wrapper instance")
        return GS_ERROR_FAIL
    }

    return GS_SUCCESS
}

/// Uninitializes the Metal device instance created for libobs.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// This method will take ownership of the reference shared with `libobs` and thus return all strong references to the
/// shared ``MetalDevice`` instance to pure Swift code (and thus its own memory managed). The active call to
/// ``MetalDevice/shutdown()`` is necessary to ensure that internal clean up code runs _before_ `libobs` runs any of
/// its own clean up code (which is not memory safe).
@_cdecl("device_destroy")
public func device_destroy(device: UnsafeMutableRawPointer) {
    let signalName = MetalSignalType.videoReset.rawValue
    let signalHandler = obs_get_signal_handler()

    signalName.withCString {
        signal_handler_disconnect(signalHandler, $0, metal_video_reset_handler, device)
    }

    let device: MetalDevice = retained(device)

    device.shutdown()
}

/// Returns opaque pointer to actual (wrapped) API-specific device object
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Opaque pointer to ``MTLDevice`` object wrapped by ``MetalDevice`` instance
///
/// The pointer shared by this function is unretained and is thus unsafe. It doesn't seem that anything in OBS Studio's
/// codebase actually uses this function, but it is part of the graphics API and thus has to be implemented.
@_cdecl("device_get_device_obj")
public func device_get_device_obj(device: UnsafeMutableRawPointer) -> OpaquePointer? {
    let metalDevice: MetalDevice = unretained(device)
    let mtlDevice = metalDevice.device

    return OpaquePointer(Unmanaged.passUnretained(mtlDevice).toOpaque())
}

/// Sets up the blend factor to be used by the current pipeline.
///
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - src: `libobs` blend type for the source
///   - dest: `libobs` blend type for the destination
///
/// This function uses the same blend factor for color and alpha channel. The enum values provided by `libobs` are
/// converted into their appropriate ``MTLBlendFactor``variants automatically (if possible).
///
/// > Important: Calling this function can trigger the creation of an entirely new render pipeline state, which is a
/// costly operation.
@_cdecl("device_blend_function")
public func device_blend_function(device: UnsafeRawPointer, src: gs_blend_type, dest: gs_blend_type) {
    device_blend_function_separate(
        device: device,
        src_c: src,
        dest_c: dest,
        src_a: src,
        dest_a: dest
    )
}

/// Sets up the color and alpha blend factors to be used by the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - src_c: `libobs` blend factor for the source color
///   - dest_c: `libobs` blend factor for the destination color
///   - src_a: `libobs` blend factor for the source alpha channel
///   - dest_a: `libobs` blend factor for the destination alpha channel
///
/// This function uses different blend factors for color and alpha channel. The enum values provided by `libobs` are
/// converted into their appropriate ``MTLBlendFactor`` variants automatically  (if possible).
///
/// > Important: Calling this function can trigger the creation of an entirely new render pipeline state, which is a
/// costly operation.
@_cdecl("device_blend_function_separate")
public func device_blend_function_separate(
    device: UnsafeRawPointer, src_c: gs_blend_type, dest_c: gs_blend_type, src_a: gs_blend_type, dest_a: gs_blend_type
) {
    let device: MetalDevice = unretained(device)

    let pipelineDescriptor = device.renderState.pipelineDescriptor
    guard let sourceRGBFactor = src_c.blendFactor,
        let sourceAlphaFactor = src_a.blendFactor,
        let destinationRGBFactor = dest_c.blendFactor,
        let destinationAlphaFactor = dest_a.blendFactor
    else {
        assertionFailure(
            """
            device_blend_function_separate: Incompatible blend factors used. Values:
                - Source RGB        : \(src_c)
                - Source Alpha      : \(src_a)
                - Destination RGB   : \(dest_c)
                - Destination Alpha : \(dest_a)
            """)
        return
    }

    pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = sourceRGBFactor
    pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = sourceAlphaFactor
    pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = destinationRGBFactor
    pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = destinationAlphaFactor
}

/// Sets the blend operation to be used by the current pipeline.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - op: `libobs` blend operation name
///
/// This function converts the provided `libobs` value into its appropriate ``MTLBlendOperation`` variant automatically
/// (if possible).
///
/// > Important: Calling this function can trigger the creation of an entirely new render pipeline state, which is a
/// costly operation.
@_cdecl("device_blend_op")
public func device_blend_op(device: UnsafeRawPointer, op: gs_blend_op_type) {
    let device: MetalDevice = unretained(device)

    let pipelineDescriptor = device.renderState.pipelineDescriptor

    guard let blendOperation = op.mtlOperation else {
        assertionFailure("device_blend_op: Incompatible blend operation provided. Value: \(op)")
        return
    }

    pipelineDescriptor.colorAttachments[0].rgbBlendOperation = blendOperation
}

/// Returns the _current_ color space as set up by any preceding calls of the `libobs` renderer.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Color space  enum value as defined by `libobs`
///
/// This color space value is commonly set by `libobs`' renderer to check the "current state", and make necessary
/// switches to ensure color-correct rendering
/// (e.g., to check if the renderer uses an SDR color space but the current source might provide HDR image data). This
/// value is effectively just retained as a state variable for `libobs`.
@_cdecl("device_get_color_space")
public func device_get_color_space(device: UnsafeRawPointer) -> gs_color_space {
    let device: MetalDevice = unretained(device)

    return device.renderState.gsColorSpace
}

/// Signals the beginning of a new render loop iteration by `libobs` renderer.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// This function is the first graphics API-specific function called by `libobs` render loop and can be used as a
/// signal to reset any lingering state of the prior loop iteration.
///
/// For the Metal renderer this ensures that the current render target, current swap chain, as well as the list of
/// active swap chains is reset. As the Metal renderer also needs to keep track of whether `libobs` is rendering any
/// "displays", the associated state variable is also reset here.
@_cdecl("device_begin_frame")
public func device_begin_frame(device: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)

    device.renderState.useSRGBGamma = false
    device.renderState.renderTarget = nil

    device.renderState.swapChain = nil
    device.renderState.isInDisplaysRenderStage = false

    return
}

/// Gets a pointer to the current render target
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Opaque pointer to ``MetalTexture`` object representing the render target
///
/// OBS Studio's renderer only ever uses a single render target at the same time and switches them out if it needs to
/// render a different output. Due to this single  state approach, it needs to retain any "current" values before
/// replacing them with (temporary) new values. It does so by retrieving pointers to the current objects set up within
/// the graphics API's opaque implementation and storing them for later use.
@_cdecl("device_get_render_target")
public func device_get_render_target(device: UnsafeRawPointer) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    guard let renderTarget = device.renderState.renderTarget else {
        return nil
    }

    return renderTarget.getUnretained()
}

/// Replaces the "current" render target and zstencil attachment with the objects associated by any provided non-`nil`
/// pointers.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - tex: Opaque (optional) pointer to ``MetalTexture`` instance shared with `libobs`
///   - zstencil: Opaque (optional) pointer to ``MetalTexture`` instance shared with `libobs`
///
/// This setter function is often used in conjunction with its associated getter function to temporarily "switch state"
/// of the renderer by retaining a pointer to the "current" render target, setting up a new one, issuing a draw call,
/// before restoring the original render target.
///
/// This is regularly used for "texrender" instances, such as combining the chroma and luma components of a video frame
/// (and uploaded as single- and dual-channel textures respectively) back into an RGB texture. This texture is then
/// used as the "output" of its corresponding source in the "actual" render pass, which will use the original render
/// target again.
@_cdecl("device_set_render_target")
public func device_set_render_target(device: UnsafeRawPointer, tex: UnsafeRawPointer?, zstencil: UnsafeRawPointer?) {
    device_set_render_target_with_color_space(
        device: device,
        tex: tex,
        zstencil: zstencil,
        space: GS_CS_SRGB
    )
}

/// Replaces the "current" render target and zstencil attachment with the objects associated by any provided non-`nil`
/// pointers and also updated the "current" color space used by the renderer.

/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - tex: Opaque (optional) pointer to ``MetalTexture`` instance shared with `libobs`
///   - zstencil: Opaque (optional) pointer to ``MetalTexture`` instance shared with `libobs`
///   - space: `libobs`-based color space value
///
/// This setter function is often used in conjunction with its associated getter function to temporarily "switch state"
/// of the renderer by retaining a pointer to the "current" render target, setting up a new one, issuing a draw call,
/// before restoring the original render target.
///
/// This is regularly used for "texrender" instances, such as combining the chroma and luma components of a video frame
/// (and uploaded as single- and dual-channel textures respectively) back into an RGB texture. This texture is then
/// used as the "output" of its corresponding source in the "actual" render pass, which will use the original render
/// target again.
///
/// A `nil` pointer provided for either the render target or zstencil attachment means that the "current" value for
/// either should be removed, leaving the renderer in an "invalid" state at least for the render target (using no
/// zstencil attachment is a valid state however).
///
/// > Important: Use this variant if you need to also update the "current" color space which might be checked by
/// sources' render function to check whether linear gamma or sRGB's gamma will be used to encode color values.
@_cdecl("device_set_render_target_with_color_space")
public func device_set_render_target_with_color_space(
    device: UnsafeRawPointer, tex: UnsafeRawPointer?, zstencil: UnsafeRawPointer?, space: gs_color_space
) {
    let device: MetalDevice = unretained(device)

    if let tex {
        let metalTexture: MetalTexture = unretained(tex)

        device.renderState.renderTarget = metalTexture
        device.renderState.isRendertargetChanged = true
    } else {
        device.renderState.renderTarget = nil
    }

    if let zstencil {
        let zstencilAttachment: MetalTexture = unretained(zstencil)

        device.renderState.depthStencilAttachment = zstencilAttachment
        device.renderState.isRendertargetChanged = true
    } else {
        device.renderState.depthStencilAttachment = nil
    }

    device.renderState.gsColorSpace = space
}

/// Switches the current render state to use sRGB gamma encoding and decoding when reading from textures and writing
/// into render targets
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - enable: Boolean to enable or disable the automatic sRGB gamma encoding and decoding
///
/// OBS Studio's renderer has been retroactively updated to use sRGB color primaries _and_ gamma encoding by
/// preference, but not by default. Any source has to opt-in to the use of automatic sRGB gamma encoding and decoding,
/// while the default is still to use linear gamma.
///
/// This method is thus used by sources to enable or disable the associated behavior and control the way color values
/// generated by fragment shaders are written into the render target.
@_cdecl("device_enable_framebuffer_srgb")
public func device_enable_framebuffer_srgb(device: UnsafeRawPointer, enable: Bool) {
    let device: MetalDevice = unretained(device)

    if device.renderState.useSRGBGamma != enable {
        device.renderState.useSRGBGamma = enable
        device.renderState.isRendertargetChanged = true
    }
}

/// Retrieves the current render state's setting for using automatic encoding and decoding of color values using sRGB
/// gamma.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Boolean value of the sRGB gamma setting
///
/// This function is used to check the current state which might have possibly been explicitly changed by calls of
/// ``device_enable_framebuffer_srgb``.
///
/// A source which might only be able to work with color values that have sRGB gamma already applied to them and thus
/// might want to ensure that the color values provided by the fragment shader will not have the sRGB gamma curve
/// encoded on them again.
///
/// By calling this function, a source can check if automatic gamma encoding is enabled and then turn it off
/// explicitly, which will ensure that color data is written as-is and no additional encoding will take place.
@_cdecl("device_framebuffer_srgb_enabled")
public func device_framebuffer_srgb_enabled(device: UnsafeRawPointer) -> Bool {
    let device: MetalDevice = unretained(device)

    return device.renderState.useSRGBGamma
}

/// Signals the beginning of a new scene.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// OBS Studio's renderer signals a new scene for each "display" and for every "video mix", which implicitly signals a
/// change of output format. This usually also implies that all current textures that might have been set up for
/// fragment shaders should be reset. For Metal this also requires creating a new "current" command buffer which should
/// contain all GPU commands necessary to render the "scene".
@_cdecl("device_begin_scene")
public func device_begin_scene(device: UnsafeMutableRawPointer) {
    let device: MetalDevice = unretained(device)

    for index in 0..<GS_MAX_TEXTURES {
        device.renderState.textures[Int(index)] = nil
        device.renderState.samplers[Int(index)] = nil
    }
}

/// Signals the end of a scene.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// OBS Studio's renderer signals the end of a scene for each "display" and for every "video mix", which implicitly
/// marks the end of the output at a different format. As the Metal renderer needs a way to detect if all draw commands
/// for a given "display" have ended (and there is no bespoke signal for that in the API), it uses an internal state
/// variable to track if a display had been loaded for the "current" pipeline state and resets it at the "end of scene"
/// signal.
@_cdecl("device_end_scene")
public func device_end_scene(device: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)

    if device.renderState.isInDisplaysRenderStage {
        device.finishDisplayRenderStage()
        device.renderState.isInDisplaysRenderStage = false
    }
}

/// Schedules a draw command on the GPU using all "state" variables set up by OBS Studio's renderer up to this point.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - drawMode: Primitive type to draw as specified by `libobs`
///   - startVertex: Start index of vertex to begin drawing with
///   - numVertices: Count of vertices to draw
///
///  Due to OBS Studio's design this function will usually render only a very low amount of vertices (commonly only 4
/// of them) and very often those vertices  are already loaded up as vertex buffers for use by the vertex shader. In
/// those cases `libobs` does not seem to provide a vertex count and implicitly expects the graphics API implementation
/// to deduct the vertex count from the amount of vertices available in its vertex data struct.
///
/// In other cases a vertex shader will not use any buffers but calculate the vertex positions based on vertex ID and
/// a non-null vertex count has to be provided.
@_cdecl("device_draw")
public func device_draw(device: UnsafeRawPointer, drawMode: gs_draw_mode, startVertex: UInt32, numVertices: UInt32) {
    let device: MetalDevice = unretained(device)

    guard let primitiveType = drawMode.mtlPrimitive else {
        OBSLog(.error, "device_draw: Unsupported draw mode provided: \(drawMode)")
        return
    }

    do {
        try device.draw(primitiveType: primitiveType, vertexStart: Int(startVertex), vertexCount: Int(numVertices))
    } catch let error as MetalError.MTLDeviceError {
        OBSLog(.error, "device_draw: \(error.description)")
    } catch {
        OBSLog(.error, "device_draw: Unknown error occurred")
    }
}

/// Sets up a load action for the "current" frame buffer and depth stencil attachment to simulate the "clear" behavior
/// of other graphics APIs.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - clearFlags: Bit field provided by `libobs` to mark the clear operations to handle
///   - color: The RGBA color to use for clearing the frame buffer
///   - depth: The depth to clear from the depth stencil attachment
///   - stencil: The stencil to clear from the depth stencil attachment
///
/// In APIs like OpenGL or Direct3D11 render targets have to be explicitly cleared. In OpenGL this is achieved by
/// calling `glClear()` which will schedule a clear operation. Similarly Direct3D11 requires a call to
/// `ClearRenderTargetView` with a specific `ID3D11RenderTargetView` to do the same.
///
/// Metal does not provide an explicit command to "clear the screen" (as one does not render directly to screens
/// anymore with these APIs). Instead Metal provides "load commands" and "store commands" which describe what should
/// happen to a render target when it is loaded for rendering and unloaded after rendering.
///
/// Thus a "clear" is a "load command" for a render target or depth stencil attachment that is automatically executed
/// by Metal when it loads or stores them and thus requires Metal to do an explicit (empty) draw call to ensure that
/// the load and store commands are executed even when no other draw calls will follow.
@_cdecl("device_clear")
public func device_clear(
    device: UnsafeRawPointer, clearFlags: UInt32, color: UnsafePointer<vec4>, depth: Float, stencil: UInt8
) {
    let device: MetalDevice = unretained(device)

    var clearState = ClearState()

    if (Int32(clearFlags) & GS_CLEAR_COLOR) == 1 {
        clearState.colorAction = .clear
        clearState.clearColor = MTLClearColor(
            red: Double(color.pointee.x),
            green: Double(color.pointee.y),
            blue: Double(color.pointee.z),
            alpha: Double(color.pointee.w)
        )
    } else {
        clearState.colorAction = .load
    }

    if (Int32(clearFlags) & GS_CLEAR_DEPTH) == 1 {
        clearState.clearDepth = Double(depth)
        clearState.depthAction = .clear
    } else {
        clearState.depthAction = .load
    }

    if (Int32(clearFlags) & GS_CLEAR_STENCIL) == 1 {
        clearState.clearStencil = UInt32(stencil)
        clearState.stencilAction = .clear
    } else {
        clearState.stencilAction = .load
    }

    do {
        try device.clear(state: clearState)

    } catch let error as MetalError.MTLDeviceError {
        OBSLog(.error, "device_clear: \(error.description)")
    } catch {
        OBSLog(.error, "device_clear: Unknown error occurred")
    }
}

/// Returns whether the current display is ready to preset a frame generated the renderer
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Boolean value to state whether a frame generated by the renderer could actually be displayed
///
/// As OBS Studio's renderer is not synced with the operating system's compositor, situations could arise where the
/// renderer needs to be able to "hand off" a generated display output to the compositor but might not be able to
/// because it's not "ready" to receive such a frame. If that is the case, the graphics API can check for such a state
/// and return `false` here, allowing `libobs` to skip rendering the output for the "current" display entirely.
///
/// In Direct3D11 the `DXGI_SWAP_EFFECT_FLIP_DISCARD` flip effect is used, which allows OBS Studio to render a preview
/// into a buffer without having to care about the compositor. This is not possible in Metal as it's not the
/// application that provides the output buffer, it's the compositor which provides a "drawable" surface. For each
/// display there can only be a maximum of 3 drawables "in flight", a request for any consecutive drawable will stall
/// the renderer.
///
/// There is currently no way to check for the amount of available drawables, which could be used to return `false`
/// here and would allow `libobs` to skip output rendering on its current frame and try again on the next.
///
/// > Note: This check applies to the display associated with whichever "swap chain" might be "current" and is thus
/// depends on swap chain state.
@_cdecl("device_is_present_ready")
public func device_is_present_ready(device: UnsafeRawPointer) -> Bool {
    return true
}

/// Commits the current command buffer to schedule and execute the GPU commands encoded within it and waits until they
/// have been scheduled.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// OBS Studio's renderer will call this function when it has set up all draw commands for a given "display". It is
/// usually accompanied by a call to end the current scene just before and thus marks the end of commands for the
/// current command buffer.
@_cdecl("device_present")
public func device_present(device: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)

    device.finishPendingCommands()
}

/// Commits the current command buffer to schedule and execute the GPU commands encoded within it and waits until they
/// have been scheduled.
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// OBS Studio's renderer will call this function when it is finished setting up all draw commands for the video output
/// texture, and also after it has used the GPU to encode a video output frame.
@_cdecl("device_flush")
public func device_flush(device: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)

    device.finishPendingCommands()
}

/// Sets the "current" cull mode to be used by the next draw call
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - mode: `libobs` cull mode identifier
///
/// Converts the cull mode provided by `libobs` into its appropriate ``MTLCullMode`` variant.
@_cdecl("device_set_cull_mode")
public func device_set_cull_mode(device: UnsafeRawPointer, mode: gs_cull_mode) {
    let device: MetalDevice = unretained(device)

    device.renderState.cullMode = mode.mtlMode
}

/// Gets the "current" cull mode that was set up for the next draw call
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: `libobs` cull mode
///
/// Converts the ``MTLCullMode`` set up currently into its `libobs` variation
@_cdecl("device_get_cull_mode")
public func device_get_cull_mode(device: UnsafeRawPointer) -> gs_cull_mode {
    let device: MetalDevice = unretained(device)

    return device.renderState.cullMode.obsMode
}

/// Switches blending of the next draw operation with the contents of the "current" framebuffer.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - enable: `true` if contents should be blended, `false` otherwise
///
/// This function directly enables or disables blending for the first render target set up in the current pipeline.
@_cdecl("device_enable_blending")
public func device_enable_blending(device: UnsafeRawPointer, enable: Bool) {
    let device: MetalDevice = unretained(device)

    device.renderState.pipelineDescriptor.colorAttachments[0].isBlendingEnabled = enable
}

/// Switches depth testing on the next draw operation with the contents of the current depth stencil buffer.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - enable: `true` if depth testing should be enabled, `false` otherwise
///
/// This function directly enables or disables depth texting for the depth stencil attachment set up in the current pipeline
@_cdecl("device_enable_depth_test")
public func device_enable_depth_test(device: UnsafeRawPointer, enable: Bool) {
    let device: MetalDevice = unretained(device)

    device.renderState.depthStencilDescriptor.isDepthWriteEnabled = enable
}

/// Sets the read mask in the depth stencil descriptor set up in the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - enable: `true` if the read mask should be `1`, `false` for a read mask of `0`
///
/// The `MTLDepthStencilDescriptor` can differentiate between a front facing stencil and a back facing stencil. As
/// `libobs` does not make this distinction, both values will be set to the same value.
@_cdecl("device_enable_stencil_test")
public func device_enable_stencil_test(device: UnsafeRawPointer, enable: Bool) {
    let device: MetalDevice = unretained(device)

    device.renderState.depthStencilDescriptor.frontFaceStencil.readMask = enable ? 1 : 0
    device.renderState.depthStencilDescriptor.backFaceStencil.readMask = enable ? 1 : 0
}

/// Sets the write mask in the depth stencil descriptor set up in the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - enable: `true` if the write mask should be `1`, `false` for a write mask of `0`
///
/// The `MTLDepthStencilDescriptor` can differentiate between a front facing stencil and a back facing stencil. As
/// `libobs` does not make this distinction, both values will be set to the same value.
@_cdecl("device_enable_stencil_write")
public func device_enable_stencil_write(device: UnsafeRawPointer, enable: Bool) {
    let device: MetalDevice = unretained(device)

    device.renderState.depthStencilDescriptor.frontFaceStencil.writeMask = enable ? 1 : 0
    device.renderState.depthStencilDescriptor.backFaceStencil.writeMask = enable ? 1 : 0
}

/// Sets the color write mask for the render target set up in the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - red: `true` if the red color channel should be written, `false` otherwise
///   - green: `true` if the green color channel should be written, `false` otherwise
///   - blue: `true` if the blue color channel should be written, `false` otherwise
///   - alpha: `true` if the alpha channel should be written, `false` otherwise
///
/// The separate `bool` values are converted into an ``MTLColorWriteMask`` which is then set up on the first render
/// target of the current pipeline.
@_cdecl("device_enable_color")
public func device_enable_color(device: UnsafeRawPointer, red: Bool, green: Bool, blue: Bool, alpha: Bool) {
    let device: MetalDevice = unretained(device)

    var colorMask = MTLColorWriteMask()

    if red {
        colorMask.insert(.red)
    }

    if green {
        colorMask.insert(.green)
    }

    if blue {
        colorMask.insert(.blue)
    }

    if alpha {
        colorMask.insert(.alpha)
    }

    device.renderState.pipelineDescriptor.colorAttachments[0].writeMask = colorMask
}

/// Sets the depth compare function for the depth stencil descriptor to be used in the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - test: `libobs` enum describing the depth compare function to use
///
/// The enum value provided by `libobs` is converted into a ``MTLCompareFunction``, which is then set directly as the
/// compare function on the depth stencil descriptor.
@_cdecl("device_depth_function")
public func device_depth_function(device: UnsafeRawPointer, test: gs_depth_test) {
    let device: MetalDevice = unretained(device)

    device.renderState.depthStencilDescriptor.depthCompareFunction = test.mtlFunction
}

/// Sets the stencil compare functions for the specified stencil side(s) on the depth stencil descriptor in the current
/// pipeline.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - side: The stencil side(s) for which the compare function should be set up
///   - test: `libobs` enum describing the stencil test function to use
///
/// The enum values provided by `libobs` are first checked for the stencil side, after which the compare function value
/// itself is converted into a ``MTLCompareFunction``, which is then set directly as the compare function on the depth
/// stencil descriptor.
@_cdecl("device_stencil_function")
public func device_stencil_function(device: UnsafeRawPointer, side: gs_stencil_side, test: gs_depth_test) {
    let device: MetalDevice = unretained(device)

    let stencilCompareFunction: (MTLCompareFunction, MTLCompareFunction)

    if side == GS_STENCIL_FRONT {
        stencilCompareFunction = (test.mtlFunction, .never)
    } else if side == GS_STENCIL_BACK {
        stencilCompareFunction = (.never, test.mtlFunction)
    } else {
        stencilCompareFunction = (test.mtlFunction, test.mtlFunction)
    }

    device.renderState.depthStencilDescriptor.frontFaceStencil.stencilCompareFunction = stencilCompareFunction.0
    device.renderState.depthStencilDescriptor.backFaceStencil.stencilCompareFunction = stencilCompareFunction.1
}

/// Sets the stencil fail, depth fail, and depth pass operations for the specified stencil side(s) on the depth stencil
/// descriptor for the current pipeline.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - side: The stencil side(s) for which the fail and pass functions should be set up
///   - fail: `libobs` enum value describing the stencil fail operation
///   - zfail: `libobs` enum value describing the depth fail operation
///   - zpass: `libobs` enum value describing the depth pass operation
///
/// The enum values provided by `libobs` are first checked for the stencil side, after which the fail function values
/// themselves are converted into their ``MTLCompareFunction`` variants, which are then set directly on the depth
/// stencil descriptor.
@_cdecl("device_stencil_op")
public func device_stencil_op(
    device: UnsafeRawPointer, side: gs_stencil_side, fail: gs_stencil_op_type, zfail: gs_stencil_op_type,
    zpass: gs_stencil_op_type
) {
    let device: MetalDevice = unretained(device)

    let stencilFailOperation: (MTLStencilOperation, MTLStencilOperation)
    let depthFailOperation: (MTLStencilOperation, MTLStencilOperation)
    let depthPassOperation: (MTLStencilOperation, MTLStencilOperation)

    if side == GS_STENCIL_FRONT {
        stencilFailOperation = (fail.mtlOperation, .keep)
        depthFailOperation = (zfail.mtlOperation, .keep)
        depthPassOperation = (zpass.mtlOperation, .keep)
    } else if side == GS_STENCIL_BACK {
        stencilFailOperation = (.keep, fail.mtlOperation)
        depthFailOperation = (.keep, zfail.mtlOperation)
        depthPassOperation = (.keep, zpass.mtlOperation)
    } else {
        stencilFailOperation = (fail.mtlOperation, fail.mtlOperation)
        depthFailOperation = (zfail.mtlOperation, zfail.mtlOperation)
        depthPassOperation = (zpass.mtlOperation, zpass.mtlOperation)
    }

    device.renderState.depthStencilDescriptor.frontFaceStencil.stencilFailureOperation = stencilFailOperation.0
    device.renderState.depthStencilDescriptor.frontFaceStencil.depthFailureOperation = depthFailOperation.0
    device.renderState.depthStencilDescriptor.frontFaceStencil.depthStencilPassOperation = depthPassOperation.0

    device.renderState.depthStencilDescriptor.backFaceStencil.stencilFailureOperation = stencilFailOperation.1
    device.renderState.depthStencilDescriptor.backFaceStencil.depthFailureOperation = depthFailOperation.1
    device.renderState.depthStencilDescriptor.backFaceStencil.depthStencilPassOperation = depthPassOperation.1
}

/// Sets up the viewport for use in the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - x: Origin X coordinate for the viewport
///   - y: Origin Y coordinate for the viewport
///   - width: Width of the viewport
///   - height: Height of the viewport
///
/// The separate values for origin and dimension are converted into an ``MTLViewport`` which is then retained as the
/// "current" viewport for later use when the pipeline is actually set up.
@_cdecl("device_set_viewport")
public func device_set_viewport(device: UnsafeRawPointer, x: Int32, y: Int32, width: Int32, height: Int32) {
    let device: MetalDevice = unretained(device)

    let viewPort = MTLViewport(
        originX: Double(x),
        originY: Double(y),
        width: Double(width),
        height: Double(height),
        znear: 0.0,
        zfar: 1.0
    )

    device.renderState.viewPort = viewPort
}

/// Gets the origin and dimensions of the viewport currently set up for use by the pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - rect: A pointer to a ``gs_rect`` struct in memory
///
/// The function is provided a pointer to a ``gs_struct`` instance in memory which can hold the x and y values for the
/// origin and dimension of the viewport.
///
/// This function is usually called when some source needs to retain the current "state" of the pipeline (of which
/// there can ever only be one) and overwrite the state with its own (in this case its own viewport). To be able to
/// restore the prior state, the "current" state needs to be retrieved from the pipeline.
@_cdecl("device_get_viewport")
public func device_get_viewport(device: UnsafeRawPointer, rect: UnsafeMutablePointer<gs_rect>) {
    let device: MetalDevice = unretained(device)

    rect.pointee.x = Int32(device.renderState.viewPort.originX)
    rect.pointee.y = Int32(device.renderState.viewPort.originY)
    rect.pointee.cx = Int32(device.renderState.viewPort.width)
    rect.pointee.cy = Int32(device.renderState.viewPort.height)
}

/// Sets up a scissor rect to be used by the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - rect: Pointer to a ``gs_rect`` struct in memory that contains origin and dimension of the scissor rect
///
/// The ``gs_rect`` is converted into a ``MTLScissorRect`` object before saving it in the "current" render state
/// for use in the next draw call.
@_cdecl("device_set_scissor_rect")
public func device_set_scissor_rect(device: UnsafeRawPointer, rect: UnsafePointer<gs_rect>?) {
    let device: MetalDevice = unretained(device)

    if let rect {
        device.renderState.scissorRect = rect.pointee.mtlScissorRect
        device.renderState.scissorRectEnabled = true
    } else {
        device.renderState.scissorRect = nil
        device.renderState.scissorRectEnabled = false
    }
}

/// Sets up an orthographic projection matrix with the provided view frustum
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - left: Left edge of view frustum on the near plane
///   - right: Right edge of view frustum on the near plane
///   - top: Top edge of view frustum on the near plane
///   - bottom: Bottom edge of view frustum on the near plane
///   - near: Distance of near plane on the Z axis
///   - far: Distance of far plane on the Z axis
@_cdecl("device_ortho")
public func device_ortho(
    device: UnsafeRawPointer, left: Float, right: Float, top: Float, bottom: Float, near: Float, far: Float
) {
    let device: MetalDevice = unretained(device)

    let rml = right - left
    let bmt = bottom - top
    let fmn = far - near

    device.renderState.projectionMatrix = matrix_float4x4(
        rows: [
            SIMD4((2.0 / rml), 0.0, 0.0, 0.0),
            SIMD4(0.0, (2.0 / -bmt), 0.0, 0.0),
            SIMD4(0.0, 0.0, (1 / fmn), 0.0),
            SIMD4((left + right) / -rml, (bottom + top) / bmt, near / -fmn, 1.0),
        ]
    )
}

/// Sets up a perspective projection matrix with the provided view frustum
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - left: Left edge of view frustum on the near plane
///   - right: Right edge of view frustum on the near plane
///   - top: Top edge of view frustum on the near plane
///   - bottom: Bottom edge of view frustum on the near plane
///   - near: Distance of near plane on the Z axis
///   - far: Distance of far plane on the Z axis
@_cdecl("device_frustum")
public func device_frustum(
    device: UnsafeRawPointer, left: Float, right: Float, top: Float, bottom: Float, near: Float, far: Float
) {
    let device: MetalDevice = unretained(device)

    let rml = right - left
    let tmb = top - bottom
    let fmn = far - near

    device.renderState.projectionMatrix = matrix_float4x4(
        columns: (
            SIMD4(((2 * near) / rml), 0.0, 0.0, 0.0),
            SIMD4(0.0, ((2 * near) / tmb), 0.0, 0.0),
            SIMD4(((left + right) / rml), ((top + bottom) / tmb), (-far / fmn), -1.0),
            SIMD4(0.0, 0.0, (-(far * near) / fmn), 0.0)
        )
    )
}

/// Requests the current projection matrix to be pushed into a projection stack
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// OBS Studio's renderer works with the assumption of one big "current" state stack, which requires the entire state
/// to be changed to meet different rendering requirements. Part of this state is the current projection matrix, which
/// might need to be replaced temporarily. This function will be called when another projection matrix will be set up
/// to allow for its restoration later.
@_cdecl("device_projection_push")
public func device_projection_push(device: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)

    device.renderState.projections.append(device.renderState.projectionMatrix)
}

/// Requests the most recently pushed projection matrix to be removed from the stack and set up as the new current
/// matrix
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///
/// OBS Studio's renderer works with the assumption of one big "current" state stack. This requires some elements of
/// this state to be temporarily retained before reinstating them after. This function will reinstate the most recently
/// added matrix as the new "current" matrix.
@_cdecl("device_projection_pop")
public func device_projection_pop(device: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)

    device.renderState.projectionMatrix = device.renderState.projections.removeLast()
}

/// Checks whether the current display is capable of displaying high dynamic range content.
///
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - monitor: Opaque pointer of a platform-dependent monitor identifier
/// - Returns: `true` if the display is capable of displaying high dynamic range content, `false` otherwise
///
/// On macOS this capability is described by the ``NSScreen/maximumPotentialExtendedDynamicRangeColorComponentValue``
/// property, which can be checked using the  ``NSWindow/screen`` property after retrieving the ``NSView/window``
/// property.
@_cdecl("device_is_monitor_hdr")
public func device_is_monitor_hdr(device: UnsafeRawPointer, monitor: UnsafeRawPointer) -> Bool {
    let device: MetalDevice = unretained(device)

    guard let swapChain = device.renderState.swapChain else {
        return false
    }

    return swapChain.edrHeadroom > 1.0
}
