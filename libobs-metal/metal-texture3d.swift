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

/// Creates a three-dimensional ``MetalTexture`` instance with the specified usage options and the raw image data
/// (if provided)
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - size: Desired size of the texture
///   - color_format: Desired color format of the texture as described by `gs_color_format`
///   - levels: Amount of mip map levels to generate for the texture
///   - data: Optional pointer to raw pixel data per mip map level
///   - flags: Texture resource use information encoded as `libobs` bitfield
/// - Returns: Opaque pointer to a created ``MetalTexture`` instance or a `NULL` pointer on error
///
/// This function will create a new ``MTLTexture`` wrapped within a ``MetalTexture`` class and also upload any pixel
/// data if non-`NULL` pointers have been provided via the `data` argument.
///
/// > Important: If mipmap generation is requested, execution will be blocked by waiting for the blit command encoder
/// to generate the mipmaps.
@_cdecl("device_voltexture_create")
public func device_voltexture_create(
    device: UnsafeRawPointer, width: UInt32, height: UInt32, depth: UInt32, color_format: gs_color_format,
    levels: UInt32, data: UnsafePointer<UnsafePointer<UInt8>?>?, flags: UInt32
) -> OpaquePointer? {
    let device = Unmanaged<MetalDevice>.fromOpaque(device).takeUnretainedValue()

    let descriptor = MTLTextureDescriptor.init(
        type: .type3D,
        width: width,
        height: height,
        depth: depth,
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
/// The ownership of the shared pointer is transferred into this function and the instance is placed under
/// Swift's memory management again.
@_cdecl("gs_voltexture_destroy")
public func gs_voltexture_destroy(voltex: UnsafeRawPointer) {
    let _ = retained(voltex) as MetalTexture
}

/// Gets the width of the texture wrapped by the ``MetalTexture`` instance
/// - Parameter voltex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Width of the texture
@_cdecl("gs_voltexture_get_width")
public func gs_voltexture_get_width(voltex: UnsafeRawPointer) -> UInt32 {
    let texture: MetalTexture = unretained(voltex)

    return UInt32(texture.texture.width)
}

/// Gets the height of the texture wrapped by the ``MetalTexture`` instance
/// - Parameter voltex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Height of the texture
@_cdecl("gs_voltexture_get_height")
public func gs_voltexture_get_height(voltex: UnsafeRawPointer) -> UInt32 {
    let texture: MetalTexture = unretained(voltex)

    return UInt32(texture.texture.height)
}

/// Gets the depth of the texture wrapped by the ``Metaltexture`` instance
/// - Parameter voltex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Depth of the texture
@_cdecl("gs_voltexture_get_depth")
public func gs_voltexture_get_depth(voltex: UnsafeRawPointer) -> UInt32 {
    let texture: MetalTexture = unretained(voltex)

    return UInt32(texture.texture.depth)
}

/// Gets the color format of the texture wrapped by the ``MetalTexture`` instance
/// - Parameter voltex: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
/// - Returns: Color format as defined by the `gs_color_format` enumeration
@_cdecl("gs_voltexture_get_color_format")
public func gs_voltexture_get_color_format(voltex: UnsafeRawPointer) -> gs_color_format {
    let texture: MetalTexture = unretained(voltex)

    return texture.texture.pixelFormat.gsColorFormat
}
