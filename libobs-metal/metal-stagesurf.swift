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

/// Creates a ``MetalStageBuffer`` instance for use as a stage surface by `libobs`
/// - Parameters:
///   - device: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`
///   - width: Number of data rows
///   - height: Number of data columns
///   - format: Color format of the stage surface texture as defined by `libobs`'s `gs_color_format` struct
/// - Returns: A ``MetalStageBuffer`` instance that wraps a `MTLBuffer` or a `nil` pointer otherwise
///
/// Stage surfaces are used by `libobs` for transfer of image data from the GPU to the CPU. The most common use case is
/// to block transfer (blit) the video output texture into a staging texture and then downloading the texture data from
/// the staging texture into CPU memory.
@_cdecl("device_stagesurface_create")
public func device_stagesurface_create(device: UnsafeRawPointer, width: UInt32, height: UInt32, format: gs_color_format)
    -> OpaquePointer?
{
    let device: MetalDevice = unretained(device)

    guard
        let buffer = MetalStageBuffer(
            device: device,
            width: Int(width),
            height: Int(height),
            format: format.mtlFormat
        )
    else {
        OBSLog(.error, "device_stagesurface_create: Unable to create MetalStageBuffer with provided format \(format)")
        return nil
    }

    return buffer.getRetained()
}

/// Requests the deinitialization of the ``MetalStageBuffer`` instance that was shared with `libobs`
/// - Parameter stagesurf: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`
///
/// The ownership of the shared pointer is transferred into this function and the instance is placed under Swift's
/// memory management again.
@_cdecl("gs_stagesurface_destroy")
public func gs_stagesurface_destroy(stagesurf: UnsafeRawPointer) {
    let _ = retained(stagesurf) as MetalStageBuffer
}

/// Gets the "width" of the staging texture
/// - Parameter stagesurf: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`
/// - Returns: Amount of data rows in the buffer representing the width of an image
@_cdecl("gs_stagesurface_get_width")
public func gs_stagesurface_get_width(stagesurf: UnsafeRawPointer) -> UInt32 {
    let stageSurface: MetalStageBuffer = unretained(stagesurf)

    return UInt32(stageSurface.width)
}

/// Gets the "height" of the staging texture
/// - Parameter stagesurf: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`
/// - Returns: Amount of data columns in the buffer representing the height of an image
@_cdecl("gs_stagesurface_get_height")
public func gs_stagesurface_get_height(stagesurf: UnsafeRawPointer) -> UInt32 {
    let stageSurface: MetalStageBuffer = unretained(stagesurf)

    return UInt32(stageSurface.height)
}

/// Gets the color format of the staged image data
/// - Parameter stagesurf: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`
/// - Returns: Color format in `libobs`'s own color format struct
///
/// The Metal color format is automatically converted into its corresponding `gs_color_format` variant.
@_cdecl("gs_stagesurface_get_color_format")
public func gs_stagesurface_get_height(stagesurf: UnsafeRawPointer) -> gs_color_format {
    let stageSurface: MetalStageBuffer = unretained(stagesurf)

    return stageSurface.format.gsColorFormat
}

/// Provides a pointer to memory that contains the buffer's raw data.
/// - Parameters:
///   - stagesurf: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`
///   - ptr: Opaque pointer to memory which itself can hold a pointer to the actual image data
///   - linesize: Opaque pointer to memory which itself can hold the row size of the image data
/// - Returns: `true` if the data can be provided, `false` otherwise
///
/// Metal does not provide "map" and "unmap" operations as they exist in Direct3D11, as resource management and
/// synchronization needs to be handled explicitly by the application. To reduce unnecessary copy operations, the
/// original texture's data was copied into a `MTLBuffer` (instead of another texture) using a block transfer on the
/// GPU.
///
/// As the Metal renderer is only available on Apple Silicon machines, this means that the buffer itself is available
/// for direct access by the CPU and thus a pointer to the raw bytes of the buffer can be shared with `libobs`.
@_cdecl("gs_stagesurface_map")
public func gs_stagesurface_map(
    stagesurf: UnsafeRawPointer, ptr: UnsafeMutablePointer<UnsafeMutableRawPointer>,
    linesize: UnsafeMutablePointer<UInt32>
) -> Bool {
    let stageSurface: MetalStageBuffer = unretained(stagesurf)

    ptr.pointee = stageSurface.buffer.contents()
    linesize.pointee = UInt32(stageSurface.width * stageSurface.format.bytesPerPixel!)

    return true
}

/// Signals that the downloaded image data of the stage texture is not needed anymore.
///
/// - Parameter stagesurf: Opaque pointer to ``MetalStageBuffer`` instance shared with `libobs`
///
/// This function has no effect as the `MTLBuffer` used by the ``MetalStageBuffer`` does not need to be "unmapped".
@_cdecl("gs_stagesurface_unmap")
public func gs_stagesurface_unmap(stagesurf: UnsafeRawPointer) {
    return
}
