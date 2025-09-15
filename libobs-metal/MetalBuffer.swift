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

enum MetalBufferType {
    case vertex
    case index
}

/// The MetalBuffer class serves as the super class for both vertex and index buffer objects.
///
/// It provides convenience functions to pass buffer instances as retained and unretained opaque pointers and provides
/// a generic buffer factory method.
class MetalBuffer {
    enum BufferDataType {
        case vertex
        case normal
        case tangent
        case color
        case texcoord
    }

    private let device: MTLDevice
    fileprivate let isDynamic: Bool

    init(device: MetalDevice, isDynamic: Bool) {
        self.device = device.device
        self.isDynamic = isDynamic
    }

    /// Creates a new buffer with the provided data or updates an existing buffer with the provided data
    /// - Parameters:
    ///   - buffer: Reference to a buffer variable to either receive the new buffer or provide an existing buffer
    ///   - data: Pointer to raw data of provided type `T`
    ///   - count: Byte size of data to be written into the buffer
    ///   - dynamic: `true` if underlying buffer is dynamically updated for each frame, `false` otherwise.
    ///
    /// > Note: Some sources (like the `text-freetype2` source) generate "dynamic" buffers but don't update them at
    /// every frame and instead treat them as "static" buffers. For this reason `MTLBuffer` objects have to be cached
    /// and re-used per `MetalBuffer` instance and cannot be dynamically provided from a pool of buffers of a `MTLHeap`.
    fileprivate func createOrUpdateBuffer<T>(
        buffer: inout MTLBuffer?, data: UnsafeMutablePointer<T>, count: Int, dynamic: Bool
    ) {
        let size = MemoryLayout<T>.size * count
        let alignedSize = (size + 15) & ~15

        if buffer != nil {
            if dynamic && buffer!.length == alignedSize {
                buffer!.contents().copyMemory(from: data, byteCount: size)
                return
            }
        }

        buffer = device.makeBuffer(
            bytes: data, length: alignedSize, options: [.cpuCacheModeWriteCombined, .storageModeShared])
    }

    /// Gets an opaque pointer for the ``MetalBuffer`` instance and increases its reference count by one
    /// - Returns: `OpaquePointer` to class instance
    ///
    /// > Note: Use this method when the instance is to be shared via an `OpaquePointer` and needs to be retained. Any
    /// opaque pointer shared this way needs to be converted into a retained reference again to ensure automatic
    /// deinitialization by the Swift runtime.
    func getRetained() -> OpaquePointer {
        let retained = Unmanaged.passRetained(self).toOpaque()

        return OpaquePointer(retained)
    }

    /// Gets an opaque pointer for the ``MetalBuffer`` instance without increasing its reference count
    /// - Returns: `OpaquePointer` to class instance
    func getUnretained() -> OpaquePointer {
        let unretained = Unmanaged.passUnretained(self).toOpaque()

        return OpaquePointer(unretained)
    }
}

final class MetalVertexBuffer: MetalBuffer {
    public var vertexData: UnsafeMutablePointer<gs_vb_data>?
    private var points: MTLBuffer?
    private var normals: MTLBuffer?
    private var tangents: MTLBuffer?
    private var vertexColors: MTLBuffer?
    private var uvCoordinates: [MTLBuffer?]

    init(device: MetalDevice, data: UnsafeMutablePointer<gs_vb_data>, dynamic: Bool) {
        self.vertexData = data
        self.uvCoordinates = Array(repeating: nil, count: data.pointee.num_tex)

        super.init(device: device, isDynamic: dynamic)

        if !dynamic {
            setupBuffers()
        }
    }

    /// Sets up buffer objects for the data provided in the provided `gs_vb_data` structure
    /// - Parameter data: Pointer to a `gs_vb_data` instance
    ///
    /// The provided `gs_vb_data` instance is expected to:
    /// * Always contain vertex data
    /// * Optionally contain normals data
    /// * Optionally contain tangents data
    /// * Optionally contain color data
    /// * Optionally contain either 2 or 4 texture coordinates per vertex
    ///
    /// > Note: The color data needs to be converted from the packed UInt32 format used by `libobs` into a normalized
    /// vector of Float32 values as Metal does not support implicit conversion of these types when vertex data is
    /// provided in a single buffer to a vertex shader.
    public func setupBuffers(data: UnsafeMutablePointer<gs_vb_data>? = nil) {
        guard let data = data ?? self.vertexData else {
            assertionFailure("MetalBuffer: Unable to create MTLBuffers without vertex data")
            return
        }

        let numVertices = data.pointee.num

        createOrUpdateBuffer(buffer: &points, data: data.pointee.points, count: numVertices, dynamic: isDynamic)

        #if DEBUG
            points?.label = "Vertex buffer points data"
        #endif

        if let normalsData = data.pointee.normals {
            createOrUpdateBuffer(buffer: &normals, data: normalsData, count: numVertices, dynamic: isDynamic)

            #if DEBUG
                normals?.label = "Vertex buffer normals data"
            #endif
        }

        if let tangentsData = data.pointee.tangents {
            createOrUpdateBuffer(buffer: &tangents, data: tangentsData, count: numVertices, dynamic: isDynamic)

            #if DEBUG
                tangents?.label = "Vertex buffer tangents data"
            #endif
        }

        if let colorsData = data.pointee.colors {
            var unpackedColors = [SIMD4<Float>]()
            unpackedColors.reserveCapacity(4)

            for i in 0..<numVertices {
                let vertexColor = colorsData.advanced(by: i)

                vertexColor.withMemoryRebound(to: UInt8.self, capacity: 4) {
                    let colorValues = UnsafeBufferPointer<UInt8>(start: $0, count: 4)

                    let color = SIMD4<Float>(
                        x: Float(colorValues[0]) / 255.0,
                        y: Float(colorValues[1]) / 255.0,
                        z: Float(colorValues[2]) / 255.0,
                        w: Float(colorValues[3]) / 255.0
                    )

                    unpackedColors.append(color)
                }
            }

            unpackedColors.withUnsafeMutableBufferPointer {
                createOrUpdateBuffer(
                    buffer: &vertexColors, data: $0.baseAddress!, count: numVertices, dynamic: isDynamic)
            }

            #if DEBUG
                vertexColors?.label = "Vertex buffer colors data"
            #endif
        }

        guard data.pointee.num_tex > 0 else {
            return
        }

        let textureVertices = UnsafeMutableBufferPointer<gs_tvertarray>(
            start: data.pointee.tvarray, count: data.pointee.num_tex)

        for (textureSlot, textureVertex) in textureVertices.enumerated() {
            textureVertex.array.withMemoryRebound(to: Float32.self, capacity: textureVertex.width * numVertices) {
                createOrUpdateBuffer(
                    buffer: &uvCoordinates[textureSlot], data: $0, count: textureVertex.width * numVertices,
                    dynamic: isDynamic)
            }

            #if DEBUG
                uvCoordinates[textureSlot]?.label = "Vertex buffer texture uv data (texture slot \(textureSlot))"
            #endif
        }
    }

    /// Gets a collection of all ` MTLBuffer` objects created for the vertex data contained in the ``MetalBuffer``.
    /// - Parameter shader: ``MetalShader`` instance for which the buffers will be used
    /// - Returns: Array for `MTLBuffer`s in the order required by the shader
    ///
    /// > Important: To ensure that the data in the buffers is aligned with the structures declared in the shaders,
    /// each ``MetalShader`` provides a "buffer order". The corresponding collection will contain the associated
    /// ``MTLBuffer`` objects in this order.
    public func getShaderBuffers(for shader: MetalShader) -> [MTLBuffer] {
        var bufferList = [MTLBuffer]()

        for bufferType in shader.bufferOrder {
            switch bufferType {
            case .vertex:
                if let points {
                    bufferList.append(points)
                }
            case .normal:
                if let normals { bufferList.append(normals) }
            case .tangent:
                if let tangents { bufferList.append(tangents) }
            case .color:
                if let vertexColors { bufferList.append(vertexColors) }
            case .texcoord:
                guard shader.textureCount == uvCoordinates.count else {
                    assertionFailure(
                        "MetalBuffer: Amount of available texture uv coordinates not sufficient for vertex shader")
                    break
                }

                for i in 0..<shader.textureCount {
                    if let uvCoordinate = uvCoordinates[i] {
                        bufferList.append(uvCoordinate)
                    }
                }
            }
        }

        return bufferList
    }

    deinit {
        gs_vbdata_destroy(vertexData)
    }
}

final class MetalIndexBuffer: MetalBuffer {
    public var indexData: UnsafeMutableRawPointer?
    public var count: Int
    public var type: MTLIndexType

    var indices: MTLBuffer?

    init(device: MetalDevice, type: MTLIndexType, data: UnsafeMutableRawPointer?, count: Int, dynamic: Bool) {
        self.indexData = data
        self.count = count
        self.type = type

        super.init(device: device, isDynamic: dynamic)

        if !dynamic {
            setupBuffers()
        }
    }

    /// Sets up buffer objects for the data provided in the provided memory location
    /// - Parameter data: Pointer to bytes representing index buffer data
    ///
    /// The provided memory location is expected to provide bytes represnting index buffer data as either unsigned
    /// 16-bit integers or unsigned 32-bit integers. The size depends on the type used to create the
    /// ``MetalIndexBuffer`` instance.
    public func setupBuffers(_ data: UnsafeMutableRawPointer? = nil) {
        guard let indexData = data ?? indexData else {
            assertionFailure("MetalIndexBuffer: Unable to generate MTLBuffer without buffer data")
            return
        }

        let byteSize =
            switch type {
            case .uint16: 2 * count
            case .uint32: 4 * count
            @unknown default:
                fatalError("MTLIndexType \(type) is not supported")
            }

        indexData.withMemoryRebound(to: UInt8.self, capacity: byteSize) {
            createOrUpdateBuffer(buffer: &indices, data: $0, count: byteSize, dynamic: isDynamic)
        }

        #if DEBUG
            if !isDynamic {
                indices?.label = "Index buffer static data"
            } else {
                indices?.label = "Index buffer dynamic data"
            }
        #endif
    }

    deinit {
        bfree(indexData)
    }
}
