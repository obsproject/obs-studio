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

/// Creates a ``MetalIndexBuffer`` object to share with `libobs` and hold the provided indices
///
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - type: Size of each index value (16 bit or 32 bit)
///   - indices: Opaque pointer to index buffer data set up by `libobs`
///   - num: Count of vertices present at the memory address provided by the `indices` argument
///   - flags: Bit field of `libobs` buffer flags
/// - Returns: Opaque pointer to a retained ``MetalIndexBuffer`` instance if valid index type was provided, `nil`
/// otherwise
///
/// > Note: The ownership of the memory pointed to by `indices` is implicitly transferred to the ``MetalIndexBuffer``
/// instance, but is not managed by Swift.
@_cdecl("device_indexbuffer_create")
public func device_indexbuffer_create(
    device: UnsafeRawPointer, type: gs_index_type, indices: UnsafeMutableRawPointer, num: UInt32, flags: UInt32
) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    guard let indexType = type.mtlType else {
        return nil
    }

    let indexBuffer = MetalIndexBuffer(
        device: device,
        type: indexType,
        data: indices,
        count: Int(num),
        dynamic: (Int32(flags) & GS_DYNAMIC) != 0
    )

    return indexBuffer.getRetained()
}

/// Sets up a ``MetalIndexBuffer`` as the index buffer for the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - indexbuffer: Opaque pointer to ``MetalIndexBuffer`` instance shared with `libobs`
///
/// > Note: The reference count of the ``MetalIndexBuffer`` instance will not be increased by this call.
///
/// > Important: If a `nil` pointer is provided as the index buffer, the index buffer will be _unset_.
@_cdecl("device_load_indexbuffer")
public func device_load_indexbuffer(device: UnsafeRawPointer, indexbuffer: UnsafeRawPointer?) {
    let device: MetalDevice = unretained(device)

    if let indexbuffer {
        device.renderState.indexBuffer = unretained(indexbuffer)
    } else {
        device.renderState.indexBuffer = nil
    }
}

/// Requests the deinitialization of a shared ``MetalIndexBuffer`` instance
/// - Parameter indexBuffer: Opaque pointer to ``MetalIndexBuffer`` instance shared with `libobs`
///
/// The deinitialization is handled automatically by Swift after the ownership of the instance has been transferred
/// into the function and becomes the last strong reference to it. After the function leaves its scope, the object will
/// be deinitialized and deallocated automatically.
///
/// > Note: The index buffer data memory is implicitly owned by the ``MetalIndexBuffer`` instance and will be manually
/// cleaned up and deallocated by the instance's `deinit` method.
@_cdecl("gs_indexbuffer_destroy")
public func gs_indexbuffer_destroy(indexBuffer: UnsafeRawPointer) {
    let _ = retained(indexBuffer) as MetalIndexBuffer
}

/// Requests the index buffer's current data to be transferred into GPU memory
/// - Parameter indexBuffer: Opaque pointer to ``MetalIndexBuffer`` instance shared with `libobs`
///
/// This function will call `gs_indexbuffer_flush_direct` with `nil` data pointer.
@_cdecl("gs_indexbuffer_flush")
public func gs_indexbuffer_flush(indexBuffer: UnsafeRawPointer) {
    gs_indexbuffer_flush_direct(indexBuffer: indexBuffer, data: nil)
}

/// Requests the index buffer to be updated with the provided data and then transferred into GPU memory
/// - Parameters:
///   - indexBuffer: Opaque pointer to ``MetalIndexBuffer`` instance shared with `libobs`
///   - data: Opaque pointer to index buffer data set up by `libobs`
///
/// This function is called to ensure that the index buffer data that is contained in the memory pointed at by the
/// `data` argument is uploaded into GPU memory. If a `nil` pointer is provided instead, the data provided to the
/// instance during creation will be used instead.
@_cdecl("gs_indexbuffer_flush_direct")
public func gs_indexbuffer_flush_direct(indexBuffer: UnsafeRawPointer, data: UnsafeMutableRawPointer?) {
    let indexBuffer: MetalIndexBuffer = unretained(indexBuffer)

    indexBuffer.setupBuffers(data)
}

/// Returns an opaque pointer to the index buffer data associated with the ``MetalIndexBuffer`` instance
/// - Parameter indexBuffer: Opaque pointer to ``MetalIndexBuffer`` instance shared with `libobs`
/// - Returns: Opaque pointer to index buffer data in memory
///
/// The returned opaque pointer represents the unchanged memory address that was provided for the creation of the index
/// buffer object.
///
/// > Warning: There is only limited memory safety associated with this pointer. It is implicitly owned and its
/// lifetime is managed by the ``MetalIndexBuffer`` instance, but it was originally created by `libobs`.
@_cdecl("gs_indexbuffer_get_data")
public func gs_indexbuffer_get_data(indexBuffer: UnsafeRawPointer) -> UnsafeMutableRawPointer? {
    let indexBuffer: MetalIndexBuffer = unretained(indexBuffer)

    return indexBuffer.indexData
}

/// Returns the number of indices associated with the ``MetalIndexBuffer`` instance
/// - Parameter indexBuffer: Opaque pointer to ``MetalIndexBuffer`` instance shared with `libobs`
/// - Returns: Number of index buffers
///
/// > Note: This returns the same number that was provided for the creation of the index buffer object.
@_cdecl("gs_indexbuffer_get_num_indices")
public func gs_indexbuffer_get_num_indices(indexBuffer: UnsafeRawPointer) -> UInt32 {
    let indexBuffer: MetalIndexBuffer = unretained(indexBuffer)

    return UInt32(indexBuffer.count)
}

/// Gets the index buffer type as a `libobs` enum value
/// - Parameter indexBuffer: Opaque pointer to ``MetalIndexBuffer`` instance shared with `libobs`
/// - Returns: Index buffer type as identified by the `gs_index_type` enum
///
/// > Warning: As the `gs_index_type` enumeration does not provide an "invalid" value (and thus `0` becomes a valied
/// value), this function has no way to communicate an incompatible index buffer type that might be introduced at a
/// later point.
@_cdecl("gs_indexbuffer_get_type")
public func gs_indexbuffer_get_type(indexBuffer: UnsafeRawPointer) -> gs_index_type {
    let indexBuffer: MetalIndexBuffer = unretained(indexBuffer)

    switch indexBuffer.type {
    case .uint16: return GS_UNSIGNED_SHORT
    case .uint32: return GS_UNSIGNED_LONG
    @unknown default:
        assertionFailure("gs_indexbuffer_get_type: Unsupported index buffer type \(indexBuffer.type)")
        return GS_UNSIGNED_SHORT
    }
}
