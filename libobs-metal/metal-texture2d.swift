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

/// Creates a two-dimensional ``MetalTexture`` instance with the specified usage options and the raw image data (if
/// provided)
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - width: Desired width of the texture
///   - height: Desired height of the texture
///   - color_format: Desired color format of the texture as described by `gs_color_format`
///   - levels: Amount of mip map levels to generate for the texture
///   - data: Optional pointer to raw pixel data per mip map level
///   - flags: Texture resource use information encoded as `libobs` bitfield
/// - Returns: Opaque pointer to a created ``MetalTexture`` instance or a `nil` pointer on error
///
/// This function will create a new ``MTLTexture`` wrapped within a ``MetalTexture`` class and also upload any pixel
/// data if non-`nil` pointers have been provided via the `data` argument.
///
/// > Important: If mipmap generation is requested, execution will be blocked by waiting for the blit command encoder
/// to generate the mipmaps.
@_cdecl("device_texture_create")
public func device_texture_create(
    device: UnsafeRawPointer, width: UInt32, height: UInt32, color_format: gs_color_format, levels: UInt32,
    data: UnsafePointer<UnsafePointer<UInt8>?>?, flags: UInt32
) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    let descriptor = MTLTextureDescriptor.init(
        type: .type2D,
        width: width,
        height: height,
        depth: 1,
        colorFormat: color_format,
        levels: levels,
        flags: flags
    )

    guard let descriptor, let texture = MetalTexture(device: device, descriptor: descriptor) else {
        return nil
    }

    if let data {
        texture.upload(data: data, mipmapLevels: descriptor.mipmapLevelCount)
    }

    return texture.getRetained()
}

/// Creates a ``MetalTexture`` instance for a cube texture with the specified usage options and the raw image data (if provided)
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - size: Desized edge length for the cube
///   - color_format: Desired color format of the texture as described by `gs_color_format`
///   - levels: Amount of mip map levels to generate for the texture
///   - data: Optional pointer to raw pixel data per mip map level
///   - flags: Texture resource use information encoded as `libobs` bitfield
/// - Returns: Opaque pointer to created ``MetalTexture`` instance or a `nil` pointer on error
///
/// This function will create a new ``MTLTexture`` wrapped within a ``MetalTexture`` class and also upload any pixel
/// data if non-`nil` pointers have
/// been provided via the `data` argument.
///
/// > Important: If mipmap generation is requested, execution will be blocked by waiting for the blit command encoder
/// to generate the mipmaps.
@_cdecl("device_cubetexture_create")
public func device_cubetexture_create(
    device: UnsafeRawPointer, size: UInt32, color_format: gs_color_format, levels: UInt32,
    data: UnsafePointer<UnsafePointer<UInt8>?>?, flags: UInt32
) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    let descriptor = MTLTextureDescriptor.init(
        type: .typeCube,
        width: size,
        height: size,
        depth: 1,
        colorFormat: color_format,
        levels: levels,
        flags: flags
    )

    guard let descriptor, let texture = MetalTexture(device: device, descriptor: descriptor) else {
        return nil
    }

    if let data {
        texture.upload(data: data, mipmapLevels: descriptor.mipmapLevelCount)
    }

    return texture.getRetained()
}

/// Requests deinitialization of the ``MetalTexture`` instance shared with `libobs`
/// - Parameter texture: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///
/// The ownership of the shared pointer is transferred into this function and the instance is placed under Swift's
/// memory management again.
@_cdecl("gs_texture_destroy")
public func gs_texture_destroy(texture: UnsafeRawPointer) {
    let _ = retained(texture) as MetalTexture
}

/// Gets the type of the texture wrapped by the ``MetalTexture`` instance
/// - Parameter texture: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Texture type identified by `gs_texture_type` enum value
///
/// > Warning: As `libobs` has no enum value for "invalid texture type", there is no way for this function to signal
/// that the wrapped texture has an incompatible ``MTLTextureType``. Instead of crashing the program (which would
/// avoid undefined behavior), this function will return the 2D texture type value instead, which is incorrect, but is
/// more in line with how OBS Studio handles undefined behavior.
@_cdecl("device_get_texture_type")
public func device_get_texture_type(texture: UnsafeRawPointer) -> gs_texture_type {
    let texture: MetalTexture = unretained(texture)

    return texture.texture.textureType.gsTextureType ?? GS_TEXTURE_2D
}

/// Requests the ``MetalTexture`` instance to be loaded as one of the current pipeline's fragment attachments in the
/// specified texture slot
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///   - unit: Texture slot for fragment attachment
///
/// OBS Studio expects pipelines to support fragment attachments for textures and samplers up to the amount defined in
/// the `GS_MAX_TEXTURES` preprocessor directive. The order of this calls can be arbitrary, so at any point in time a
/// request to load a texture into slot "5" can take place, even if slots 0 to 4 are empty.
@_cdecl("device_load_texture")
public func device_load_texture(device: UnsafeRawPointer, tex: UnsafeRawPointer, unit: UInt32) {
    let device: MetalDevice = unretained(device)
    let texture: MetalTexture = unretained(tex)

    device.renderState.textures[Int(unit)] = texture.texture
}

/// Requests an sRGB variant of a ``MetalTexture`` instance to be set as one of the current pipeline's fragment
/// attachments in the specified texture slot.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///   - unit: Texture slot for fragment attachment
/// OBS Studio expects pipelines to support fragment attachments for textures and samplers up to the amount defined in
/// the `GS_MAX_TEXTURES` preprocessor directive. The order of this calls can be arbitrary, so at any point in time a
/// request to load a texture into slot "5" can take place, even if slots 0 to 4 are empty.
///
/// > Important: This variant of the texture load functions expects a texture whose color values are already sRGB gamma
/// encoded and thus also expects that the color values used in the fragment shader will have been automatically
/// decoded into linear gamma. If the ``MetalTexture`` instance has no dedicated ``MetalTexture/sRGBtexture`` instance,
/// it will use the normal ``MetalTexture/texture`` instance instead.
@_cdecl("device_load_texture_srgb")
public func device_load_texture_srgb(device: UnsafeRawPointer, tex: UnsafeRawPointer, unit: UInt32) {
    let device: MetalDevice = unretained(device)
    let texture: MetalTexture = unretained(tex)

    if texture.sRGBtexture != nil {
        device.renderState.textures[Int(unit)] = texture.sRGBtexture!
    } else {
        device.renderState.textures[Int(unit)] = texture.texture
    }
}

/// Copies image data from a region in the source ``MetalTexture`` into a destination ``MetalTexture`` at the provided
/// origin
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - dst: Opaque pointer to ``MetalTexture`` instance shared with `libobs`, used as destination for the copy operation
///   - dst_x: X coordinate of the origin in the destination texture
///   - dst_y: Y coordinate of the origin in the destination texture
///   - src: Opaque pointer to ``MetalTexture`` instance shared with `libobs`, used as source for the copy operation
///   - src_x: X coordinate of the origin in the source texture
///   - src_y: Y coordinate of the origin in the source texture
///   - src_w: Width of the region in the source texture
///   - src_h: Height of the region in the source texture
///
/// This function will fail if the destination texture's dimensions aren't large enough to hold the region copied from
/// the source texture. This check will use the desired origin within the destination texture and the region's size
/// into account and checks whether the total dimensions of the destination are large enough (starting at the
/// destination origin) to hold the source's region.
///
/// > Important: Execution will **not** be blocked, the copy operation will be committed to the command queue and
/// executed at some point after this function returns.
@_cdecl("device_copy_texture_region")
public func device_copy_texture_region(
    device: UnsafeRawPointer, dst: UnsafeRawPointer, dst_x: UInt32, dst_y: UInt32, src: UnsafeRawPointer, src_x: UInt32,
    src_y: UInt32, src_w: UInt32, src_h: UInt32
) {
    let device: MetalDevice = unretained(device)
    let source: MetalTexture = unretained(src)
    let destination: MetalTexture = unretained(dst)

    var sourceRegion = MTLRegion(
        origin: .init(x: Int(src_x), y: Int(src_y), z: 0),
        size: .init(width: Int(src_w), height: Int(src_h), depth: 1)
    )

    let destinationRegion = MTLRegion(
        origin: .init(x: Int(dst_x), y: Int(dst_y), z: 0),
        size: .init(width: destination.texture.width, height: destination.texture.height, depth: 1)
    )

    if sourceRegion.size.width == 0 {
        sourceRegion.size.width = source.texture.width - sourceRegion.origin.x
    }

    if sourceRegion.size.height == 0 {
        sourceRegion.size.height = source.texture.height - sourceRegion.origin.y
    }

    guard
        destinationRegion.size.width - destinationRegion.origin.x > sourceRegion.size.width
            && destinationRegion.size.height - destinationRegion.origin.y > sourceRegion.size.height
    else {
        OBSLog(
            .error,
            "device_copy_texture_region: Destination texture \(destinationRegion.size) is not large enough to hold source region (\(sourceRegion.size) at origin \(destinationRegion.origin)"
        )
        return
    }

    do {
        try device.copyTextureRegion(
            source: source,
            sourceRegion: sourceRegion,
            destination: destination,
            destinationRegion: destinationRegion)
    } catch let error as MetalError.MTLDeviceError {
        OBSLog(.error, "device_clear: \(error.description)")
    } catch {
        OBSLog(.error, "device_clear: Unknown error occurred")
    }
}

/// Copies the image data from the source ``MetalTexture`` into the destination ``MetalTexture``
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - dst: Opaque pointer to ``MetalTexture`` instance shared with `libobs`, used as destination for the copy
///     operation
///   - src: Opaque pointer to ``MetalTexture`` instance shared with `libobs`, used as source for the copy operation
///
/// > Warning: This function requires that the source and destination texture dimensions are identical, otherwise the
/// copy operation will fail.
///
/// > Important: Execution will **not** be blocked, the copy operation will be committed to the command queue and
/// executed at some point after this function returns.
@_cdecl("device_copy_texture")
public func device_copy_texture(device: UnsafeRawPointer, dst: UnsafeRawPointer, src: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)
    let source: MetalTexture = unretained(src)
    let destination: MetalTexture = unretained(dst)

    do {
        try device.copyTexture(source: source, destination: destination)
    } catch let error as MetalError.MTLDeviceError {
        OBSLog(.error, "device_clear: \(error.description)")
    } catch {
        OBSLog(.error, "device_clear: Unknown error occurred")
    }
}

/// Copies the image data from the source ``MetalTexture`` into the destination ``MetalTexture`` and blocks execution
/// until the copy operation has finished.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - dst: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`, used as destination for the copy
///     operation
///   - src: Opaque pointer to ``MetalTexture`` instance shared with `libobs`, used as source for the copy operation
///
/// > Important: Execution will be blocked by waiting for the blit command encoder to finish the copy operation.
@_cdecl("device_stage_texture")
public func device_stage_texture(device: UnsafeRawPointer, dst: UnsafeRawPointer, src: UnsafeRawPointer) {
    let device: MetalDevice = unretained(device)
    let source: MetalTexture = unretained(src)
    let destination: MetalStageBuffer = unretained(dst)

    do {
        try device.stageTextureToBuffer(source: source, destination: destination)
    } catch let error as MetalError.MTLDeviceError {
        OBSLog(.error, "device_clear: \(error.description)")
    } catch {
        OBSLog(.error, "device_clear: Unknown error occurred")
    }
}

/// Gets the width of the texture wrapped by the ``MetalTexture`` instance
/// - Parameter tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Width of the texture
@_cdecl("gs_texture_get_width")
public func device_texture_get_width(tex: UnsafeRawPointer) -> UInt32 {
    let texture: MetalTexture = unretained(tex)

    return UInt32(texture.texture.width)
}

/// Gets the height of the texture wrapped by the ``MetalTexture`` instance
/// - Parameter tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Height of the texture
@_cdecl("gs_texture_get_height")
public func device_texture_get_height(tex: UnsafeRawPointer) -> UInt32 {
    let texture: MetalTexture = unretained(tex)

    return UInt32(texture.texture.height)
}

/// Gets the color format of the texture wrapped by the ``MetalTexture`` instance
/// - Parameter tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Color format as defined by the `gs_color_format` enumeration
@_cdecl("gs_texture_get_color_format")
public func gs_texture_get_color_format(tex: UnsafeRawPointer) -> gs_color_format {
    let texture: MetalTexture = unretained(tex)

    return texture.texture.pixelFormat.gsColorFormat
}

/// Allocates memory for an update of the texture's image data wrapped by the ``MetalTexture`` instance.
/// - Parameters:
///   - tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///   - ptr: Pointer to memory for the raw image data
///   - linesize: Pointer to integer for the row size of the texture
/// - Returns: `true` if the mapping memory was allocated successfully, `false` otherwise
///
/// Metal does not provide "map" and "unmap" operations as they exist in Direct3D11, as resource management and
/// synchronization needs to be handled explicitly by the application. Thus "mapping" just means that enough memory for
/// raw image data is allocated and an unmanaged pointer to that memory is shared with `libobs` for writing the image data.
///
/// To ensure that the data written into the memory provided by this function is actually used to update the texture,
/// the corresponding function `gs_texture_unmap` needs to be used.
///
/// > Important: This function can only be used to **push** new image data into the texture. To _pull_ image data from
/// the texture, use a stage surface instead.
@_cdecl("gs_texture_map")
public func gs_texture_map(
    tex: UnsafeRawPointer, ptr: UnsafeMutablePointer<UnsafeMutableRawPointer>, linesize: UnsafeMutablePointer<UInt32>
) -> Bool {
    let texture: MetalTexture = unretained(tex)

    guard texture.texture.textureType == .type2D, let device = texture.device else {
        return false
    }

    let stageBuffer: MetalStageBuffer

    if texture.stageBuffer == nil
        || (texture.stageBuffer!.width != texture.texture.width
            && texture.stageBuffer!.height != texture.texture.height)
    {
        guard
            let buffer = MetalStageBuffer(
                device: device,
                width: texture.texture.width,
                height: texture.texture.height,
                format: texture.texture.pixelFormat
            )
        else {
            OBSLog(.error, "gs_texture_map: Unable to create MetalStageBuffer for mapping texture")
            return false
        }

        texture.stageBuffer = buffer
        stageBuffer = buffer
    } else {
        stageBuffer = texture.stageBuffer!
    }

    ptr.pointee = stageBuffer.buffer.contents()
    linesize.pointee = UInt32(stageBuffer.width * stageBuffer.format.bytesPerPixel!)

    return true
}

/// Writes back raw image data into the texture wrapped by the ``MetalTexture`` instance
/// - Parameter tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///
/// This function needs to be used in tandem with `gs_texture_map`, which allocates memory for raw image data that
/// should be used in an update of the wrapped `MTLTexture`. This function will then actually replace the image data
/// in the texture with that raw image data and deallocate the memory that was allocated during `gs_texture_map`.
@_cdecl("gs_texture_unmap")
public func gs_texture_unmap(tex: UnsafeRawPointer) {
    let texture: MetalTexture = unretained(tex)

    guard texture.texture.textureType == .type2D, let stageBuffer = texture.stageBuffer, let device = texture.device
    else {
        return
    }

    do {
        try device.stageBufferToTexture(source: stageBuffer, destination: texture)
    } catch let error as MetalError.MTLDeviceError {
        OBSLog(.error, "gs_texture_unmap: \(error.description)")
    } catch {
        OBSLog(.error, "gs_texture_unmap: Unknown error occurred")
    }
}

/// Gets an opaque pointer to the ``MTLTexture`` instance wrapped by the provided ``MetalTexture`` instance
/// - Parameter tex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Opaque pointer to ``MTLTexture`` instance
///
/// > Important: The opaque pointer returned by this function is **unretained**, which means that the ``MTLTexture``
/// instance it refers to might be deinitialized at any point when no other Swift code holds a strong reference to it.
@_cdecl("gs_texture_get_obj")
public func gs_texture_get_obj(tex: UnsafeRawPointer) -> OpaquePointer {
    let texture: MetalTexture = unretained(tex)

    let unretained = Unmanaged.passUnretained(texture.texture).toOpaque()

    return OpaquePointer(unretained)
}

/// Requests deinitialization of the ``MetalTexture`` instance shared with `libobs`
/// - Parameter cubetex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///
/// The ownership of the shared pointer is transferred into this function and the instance is placed under
/// Swift's memory management again.
@_cdecl("gs_cubetexture_destroy")
public func gs_cubetexture_destroy(cubetex: UnsafeRawPointer) {
    let _ = retained(cubetex) as MetalTexture
}

/// Gets the edge size of the cube texture wrapped by the ``MetalTexture`` instance
/// - Parameter cubetex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Edge size of the cube
@_cdecl("gs_cubetexture_get_size")
public func gs_cubetexture_get_size(cubetex: UnsafeRawPointer) -> UInt32 {
    let texture: MetalTexture = unretained(cubetex)

    return UInt32(texture.texture.width)
}

/// Gets the color format of the cube texture wrapped by the ``MetalTexture`` instance
/// - Parameter cubetex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Color format value
@_cdecl("gs_cubetexture_get_color_format")
public func gs_cubetexture_get_color_format(cubetex: UnsafeRawPointer) -> gs_color_format {
    let texture: MetalTexture = unretained(cubetex)

    return texture.texture.pixelFormat.gsColorFormat
}

/// Gets the device capability state for shared textures
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Always `true`
///
/// While Metal provides a specific "shared texture" type, OBS Studio understands this to mean "textures shared between
/// processes", which is usually achieved using ``IOSurface`` references on macOS. Metal textures can be created from
/// these references, so this is always `true`.
@_cdecl("device_shared_texture_available")
public func device_shared_texture_available(device: UnsafeRawPointer) -> Bool {
    return true
}

/// Creates a ``MetalTexture`` wrapping an ``MTLTexture`` that was created using the provided ``IOSurface`` reference.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - iosurf: ``IOSurface`` reference to use as the image data source for the texture
/// - Returns: An opaque pointer to a ``MetalTexture`` instance on success, `nil` otherwise
///
/// If the provided ``IOSurface`` uses a video image format that has no compatible ``Metal`` pixel format, creation of
/// the texture will fail.
@_cdecl("device_texture_create_from_iosurface")
public func device_texture_create_from_iosurface(device: UnsafeRawPointer, iosurf: IOSurfaceRef) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    let texture = MetalTexture(device: device, surface: iosurf)

    guard let texture else {
        return nil
    }

    return texture.getRetained()
}

/// Replaces the current ``IOSurface``-based ``MTLTexture`` wrapped by the provided ``MetalTexture`` instance with a
/// new instance.
/// - Parameters:
///   - texture: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///   - iosurf: ``IOSurface`` reference to use as the image data source for the texture
/// - Returns: An opaque pointer to a ``MetalTexture`` instance on success, `nil` otherwise
///
/// The "rebind" mentioned in the function name is limited to the ``MTLTexture`` instance wrapped inside the
/// ``MetalTexture`` instance, as textures are immutable objects (but their underlying data is mutable). This allows
/// `libobs` to hold onto the same opaque ``MetalTexture`` pointer even though the backing surface might have changed.
@_cdecl("gs_texture_rebind_iosurface")
public func gs_texture_rebind_iosurface(texture: UnsafeRawPointer, iosurf: IOSurfaceRef) -> Bool {
    let texture: MetalTexture = unretained(texture)

    return texture.rebind(surface: iosurf)
}

/// Creates a new ``MetalTexture`` instance with an opaque shared texture "handle"
/// - Parameters:
///   - device: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///   - handle: Arbitrary handle value that needs to be reinterpreted into the correct platform specific shared
///     reference type
/// - Returns: An opaque pointer to a ``MetalTexture`` instance on success, `nil` otherwise
///
/// The "handle" is a generalised argument used on all platforms and needs to be converted into a platform-specific
/// type before the "shared" texture can be created. In case of macOS this means converting the unsigned integer into
/// a ``IOSurface`` address.
///
/// > Warning: As the handle is a 32-bit integer, this can break on 64-bit systems if the ``IOSurface`` pointer
/// address does not fit into a 32-bit number.
@_cdecl("device_texture_open_shared")
public func device_texture_open_shared(device: UnsafeRawPointer, handle: UInt32) -> OpaquePointer? {
    if let reference = IOSurfaceLookupFromMachPort(handle) {
        let texture = device_texture_create_from_iosurface(device: device, iosurf: reference)

        return texture
    } else {
        return nil
    }
}
