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

class MetalShader {
    /// This class wraps a single uniform shader variable, which will hold the data associated with the uniform updated
    /// by `libobs` at each render loop, which is then converted and set as vertex or fragment bytes for a render pass
    /// by the ``MetalDevice/draw`` function.
    class ShaderUniform {
        let name: String
        let gsType: gs_shader_param_type
        fileprivate let textureSlot: Int
        var samplerState: MTLSamplerState?
        fileprivate let byteOffset: Int

        var currentValues: [UInt8]?
        var defaultValues: [UInt8]?
        fileprivate var hasUpdates: Bool

        init(
            name: String, gsType: gs_shader_param_type, textureSlot: Int, samplerState: MTLSamplerState?,
            byteOffset: Int
        ) {
            self.name = name
            self.gsType = gsType

            self.textureSlot = textureSlot
            self.samplerState = samplerState
            self.byteOffset = byteOffset
            self.currentValues = nil
            self.defaultValues = nil
            self.hasUpdates = false
        }

        /// Sets the data for the shader uniform
        /// - Parameters:
        ///   - data: Pointer to data of type `T`
        ///   - size: Size of data available at the pointer provided by `data`
        ///
        /// This function will reinterpet the data provided by the pointer as raw bytes and store it as raw bytes on
        /// the Uniform.
        public func setParameter<T>(data: UnsafePointer<T>?, size: Int) {
            guard let data else {
                assertionFailure(
                    "MetalShader.ShaderUniform: Attempted to set a shader parameter with an empty data pointer")
                return
            }

            data.withMemoryRebound(to: UInt8.self, capacity: size) {
                self.currentValues = Array(UnsafeBufferPointer<UInt8>(start: $0, count: size))
            }

            hasUpdates = true
        }
    }

    /// This struct serves as a data container to communicate shader meta data between the ``OBSShader`` shader
    /// transpiler and the actual ``MetalShader`` instances created with them.
    struct ShaderData {
        let uniforms: [ShaderUniform]
        let bufferOrder: [MetalBuffer.BufferDataType]

        let vertexDescriptor: MTLVertexDescriptor?
        let samplerDescriptors: [MTLSamplerDescriptor]?

        let bufferSize: Int
        let textureCount: Int
    }

    private weak var device: MetalDevice?
    let source: String
    private var uniformData: [UInt8]
    private var uniformSize: Int
    private var uniformBuffer: MTLBuffer?

    private let library: MTLLibrary
    let function: MTLFunction
    var uniforms: [ShaderUniform]
    var vertexDescriptor: MTLVertexDescriptor?
    var textureCount = 0
    var samplers: [MTLSamplerState]?

    let type: MTLFunctionType
    let bufferOrder: [MetalBuffer.BufferDataType]

    var viewProjection: ShaderUniform?

    init(device: MetalDevice, source: String, type: MTLFunctionType, data: ShaderData) throws {
        self.device = device
        self.source = source
        self.type = type
        self.uniforms = data.uniforms
        self.bufferOrder = data.bufferOrder
        self.uniformSize = (data.bufferSize + 0x0F) & ~0x0F
        self.uniformData = [UInt8](repeating: 0, count: self.uniformSize)
        self.textureCount = data.textureCount

        switch type {
        case .vertex:
            guard let descriptor = data.vertexDescriptor else {
                throw MetalError.MetalShaderError.missingVertexDescriptor
            }

            self.vertexDescriptor = descriptor

            self.viewProjection = self.uniforms.first(where: { $0.name == "ViewProj" })
        case .fragment:
            guard let samplerDescriptors = data.samplerDescriptors else {
                throw MetalError.MetalShaderError.missingSamplerDescriptors
            }

            var samplers = [MTLSamplerState]()
            samplers.reserveCapacity(samplerDescriptors.count)

            for descriptor in samplerDescriptors {
                guard let samplerState = device.device.makeSamplerState(descriptor: descriptor) else {
                    throw MetalError.MTLDeviceError.samplerStateCreationFailure
                }

                samplers.append(samplerState)
            }

            self.samplers = samplers
        default:
            fatalError("MetalShader: Unsupported shader type \(type)")
        }

        do {
            library = try device.device.makeLibrary(source: source, options: nil)
        } catch {
            throw MetalError.MTLDeviceError.shaderCompilationFailure("Failed to create shader library")
        }

        guard let function = library.makeFunction(name: "_main") else {
            throw MetalError.MTLDeviceError.shaderCompilationFailure("Failed to create '_main' function")
        }

        self.function = function
    }

    /// Updates the Metal-specific data associated with a ``ShaderUniform`` with the raw bytes provided by `libobs`
    /// - Parameter uniform: Inout reference to the ``ShaderUniform`` instance
    ///
    /// Uniform data is provided by `libobs` precisely in the format required by the shader (and interpreted by
    /// `libobs`), which means that the raw bytes stored on the ``ShaderUniform`` are usually already in the correct
    /// order and can be used without reinterpretation.
    ///
    /// The exception to this rule is data for textures, which represents a copy of a `gs_shader_texture` struct that
    /// itself contains the pointer address of an `OpaquePointer` for a ``MetalTexture`` instance.
    private func updateUniform(uniform: inout ShaderUniform) {
        guard let device = self.device else { return }
        guard let currentValues = uniform.currentValues else { return }

        if uniform.gsType == GS_SHADER_PARAM_TEXTURE {
            var textureObject: OpaquePointer?
            var isSrgb = false

            currentValues.withUnsafeBufferPointer {
                $0.baseAddress?.withMemoryRebound(to: gs_shader_texture.self, capacity: 1) {
                    textureObject = $0.pointee.tex
                    isSrgb = $0.pointee.srgb
                }
            }

            if let textureObject {
                let texture: MetalTexture = unretained(UnsafeRawPointer(textureObject))

                if texture.sRGBtexture != nil, isSrgb {
                    device.renderState.textures[uniform.textureSlot] = texture.sRGBtexture!
                } else {
                    device.renderState.textures[uniform.textureSlot] = texture.texture
                }
            }

            if let samplerState = uniform.samplerState {
                device.renderState.samplers[uniform.textureSlot] = samplerState
                uniform.samplerState = nil
            }
        } else {
            if uniform.hasUpdates {
                let startIndex = uniform.byteOffset
                let endIndex = uniform.byteOffset + currentValues.count

                uniformData.replaceSubrange(startIndex..<endIndex, with: currentValues)
            }
        }

        uniform.hasUpdates = false
    }

    /// Creates a new buffer with the provided data or updates an existing buffer with the provided data
    /// - Parameters:
    ///   - buffer: Reference to a buffer variable to either receive the new buffer or provide an existing buffer
    ///   - data: Raw byte data array
    private func createOrUpdateBuffer(buffer: inout MTLBuffer?, data: inout [UInt8]) {
        guard let device = self.device else { return }

        let size = MemoryLayout<UInt8>.size * data.count
        let alignedSize = (size + 0x0F) & ~0x0F

        if buffer != nil {
            if buffer!.length == alignedSize {
                buffer!.contents().copyMemory(from: data, byteCount: size)
                return
            }
        }

        buffer = device.device.makeBuffer(bytes: data, length: alignedSize)
    }

    /// Sets uniform data for a current render encoder either directly as a buffer
    /// - Parameter encoder: `MTLRenderCommandEncoder` for a render pass that requires the uniform data
    ///
    /// Uniform data will be uploaded at index 30 (the very last available index) and is available as a single
    /// contiguous block of data. Uniforms are declared as structs in the Metal Shaders and explicitly passed into
    /// each function that requires access to them.
    func uploadShaderParameters(encoder: MTLRenderCommandEncoder) {
        for var uniform in uniforms {
            updateUniform(uniform: &uniform)
        }

        guard uniformSize > 0 else {
            return
        }

        switch function.functionType {
        case .vertex:
            switch uniformData.count {
            case 0..<4096: encoder.setVertexBytes(&uniformData, length: uniformData.count, index: 30)
            default:
                createOrUpdateBuffer(buffer: &uniformBuffer, data: &uniformData)
                #if DEBUG
                    uniformBuffer?.label = "Vertex shader uniform buffer"
                #endif
                encoder.setVertexBuffer(uniformBuffer, offset: 0, index: 30)
            }
        case .fragment:
            switch uniformData.count {
            case 0..<4096: encoder.setFragmentBytes(&uniformData, length: uniformData.count, index: 30)
            default:
                createOrUpdateBuffer(buffer: &uniformBuffer, data: &uniformData)
                #if DEBUG
                    uniformBuffer?.label = "Fragment shader uniform buffer"
                #endif
                encoder.setFragmentBuffer(uniformBuffer, offset: 0, index: 30)
            }
        default:
            fatalError("MetalShader: Unsupported shader type \(function.functionType)")
        }
    }

    /// Gets an opaque pointer for the ``MetalShader`` instance and increases its reference count by one
    /// - Returns: `OpaquePointer` to class instance
    ///
    /// > Note: Use this method when the instance is to be shared via an `OpaquePointer` and needs to be retained. Any
    /// opaque pointer shared this way  needs to be converted into a retained reference again to ensure automatic
    /// deinitialization by the Swift runtime.
    func getRetained() -> OpaquePointer {
        let retained = Unmanaged.passRetained(self).toOpaque()

        return OpaquePointer(retained)
    }

    /// Gets an opaque pointer for the ``MetalShader`` instance without increasing its reference count
    /// - Returns: `OpaquePointer` to class instance
    func getUnretained() -> OpaquePointer {
        let unretained = Unmanaged.passUnretained(self).toOpaque()

        return OpaquePointer(unretained)
    }
}
