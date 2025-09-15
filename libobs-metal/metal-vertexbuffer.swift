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

/// Creates a new ``MetalVertexBuffer`` instance with the given vertex buffer data and usage flags
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - data: Pointer to `gs_vb_data` vertex buffer data created by `libobs`
///   - flags: Usage flags encoded as `libobs` bitmask
/// - Returns: Opaque pointer to a new ``MetalVertexBuffer`` instance if successful, `nil` otherwise
///
/// > Note: The ownership of the memory pointed to by `data` is implicitly transferred to the ``MetalVertexBuffer``
/// instance, but is not managed by Swift.
@_cdecl("device_vertexbuffer_create")
public func device_vertexbuffer_create(device: UnsafeRawPointer, data: UnsafeMutablePointer<gs_vb_data>, flags: UInt32)
    -> OpaquePointer
{
    let device: MetalDevice = unretained(device)

    let vertexBuffer = MetalVertexBuffer(
        device: device,
        data: data,
        dynamic: (Int32(flags) & GS_DYNAMIC) != 0
    )

    return vertexBuffer.getRetained()
}

/// Requests the deinitialization of a shared ``MetalVertexBuffer`` instance
/// - Parameter indexBuffer: Opaque pointer to ``MetalVertexBuffer`` instance shared with `libobs`
///
/// The deinitialization is handled automatically by Swift after the ownership of the instance has been transferred
/// into the function and becomes the last strong reference to it. After the function leaves its scope, the object will
/// be deinitialized and deallocated automatically.
///
/// > Note: The vertex buffer data memory is implicitly owned by the ``MetalVertexBuffer`` instance and will be
/// manually cleaned up and deallocated by the instance's ``deinit`` method.
@_cdecl("gs_vertexbuffer_destroy")
public func gs_vertexbuffer_destroy(vertBuffer: UnsafeRawPointer) {
    let _ = retained(vertBuffer) as MetalVertexBuffer
}

/// Sets up a ``MetalVertexBuffer`` as the vertex buffer for the current pipeline
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - vertbuffer: Opaque pointer to ``MetalVertexBuffer`` instance shared with `libobs`
///
/// > Note: The reference count of the ``MetalVertexBuffer`` instance will not be increased by this call.
///
/// > Important: If a `nil` pointer is provided as the vertex buffer, the index buffer will be _unset_.
@_cdecl("device_load_vertexbuffer")
public func device_load_vertexbuffer(device: UnsafeRawPointer, vertBuffer: UnsafeMutableRawPointer?) {
    let device: MetalDevice = unretained(device)

    if let vertBuffer {
        device.renderState.vertexBuffer = unretained(vertBuffer)
    } else {
        device.renderState.vertexBuffer = nil
    }
}

/// Requests the vertex buffer's current data to be transferred into GPU memory
/// - Parameter vertBuffer: Opaque pointer to ``MetalVertexBuffer`` instance shared with `libobs`
///
/// This function will call `gs_vertexbuffer_flush_direct` with a `nil` pointer as the data pointer.
@_cdecl("gs_vertexbuffer_flush")
public func gs_vertexbuffer_flush(vertbuffer: UnsafeRawPointer) {
    gs_vertexbuffer_flush_direct(vertbuffer: vertbuffer, data: nil)
}

/// Requests the vertex buffer to be updated with the provided data and then transferred into GPU memory
/// - Parameters:
///   - vertBuffer: Opaque pointer to ``MetalVertexBuffer`` instance shared with `libobs`
///   - data: Opaque pointer to vertex buffer data set up by `libobs`
///
/// This function is called to ensure that the vertex buffer data that is contained in the memory pointed at by the
/// `data` argument is uploaded into GPU memory.
///
/// If a `nil` pointer is provided instead, the data provided to the instance during creation will be used instead.
@_cdecl("gs_vertexbuffer_flush_direct")
public func gs_vertexbuffer_flush_direct(vertbuffer: UnsafeRawPointer, data: UnsafeMutablePointer<gs_vb_data>?) {
    let vertexBuffer: MetalVertexBuffer = unretained(vertbuffer)

    vertexBuffer.setupBuffers(data: data)
}

/// Returns an opaque pointer to the vertex buffer data associated with the ``MetalVertexBuffer`` instance
/// - Parameter vertBuffer: Opaque pointer to ``MetalVertexBuffer`` instance shared with `libobs`
/// - Returns: Opaque pointer to index buffer data in memory
///
/// The returned opaque pointer represents the unchanged memory address that was provided for the creation of the index
/// buffer object.
///
/// > Warning: There is only limited memory safety associated with this pointer. It is implicitly owned and its
/// lifetime is managed by the ``MetalVertexBuffer``
/// instance, but it was originally created by `libobs`.
@_cdecl("gs_vertexbuffer_get_data")
public func gs_vertexbuffer_get_data(vertBuffer: UnsafeRawPointer) -> UnsafeMutablePointer<gs_vb_data>? {
    let vertexBuffer: MetalVertexBuffer = unretained(vertBuffer)

    return vertexBuffer.vertexData
}
