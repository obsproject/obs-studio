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

/// Creates ``MetalTexture`` for use as a depth stencil attachment
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - width: Desired width of the texture
///   - height: Desired height of the texture
///   - color_format: Desired color format of the depth stencil attachment as described by `gs_zstencil_format`
/// - Returns: Opaque pointer to a created ``MetalTexture`` instance or a `NULL` pointer on error
@_cdecl("device_zstencil_create")
public func device_zstencil_create(device: UnsafeRawPointer, width: UInt32, height: UInt32, format: gs_zstencil_format)
    -> OpaquePointer?
{
    let device: MetalDevice = unretained(device)

    let descriptor = MTLTextureDescriptor.init(
        width: width,
        height: height,
        colorFormat: format
    )

    guard let descriptor, let texture = MetalTexture(device: device, descriptor: descriptor) else {
        return nil
    }

    return texture.getRetained()
}

/// Gets the ``MetalTexture`` instance used as the depth stencil attachment for the current pipeline
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Opaque pointer to ``MetalTexture`` instance if any is set, `nil` otherwise
@_cdecl("device_get_zstencil_target")
public func device_get_zstencil_target(device: UnsafeRawPointer) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    guard let stencilAttachment = device.renderState.depthStencilAttachment else {
        return nil
    }

    return stencilAttachment.getUnretained()
}

/// Requests deinitialization of the ``MetalTexture`` instance shared with `libobs`
/// - Parameter zstencil: Opaque pointer to ``MetalTexture`` instance shared with `libobs`
///
/// The ownership of the shared pointer is transferred into this function and the instance is placed under Swift's
/// memory management again.
@_cdecl("gs_zstencil_destroy")
public func gs_zstencil_destroy(zstencil: UnsafeRawPointer) {
    let _ = retained(zstencil) as MetalTexture
}
