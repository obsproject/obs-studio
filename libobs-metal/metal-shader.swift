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

private typealias ParserError = MetalError.OBSShaderParserError
private typealias ShaderError = MetalError.OBSShaderError
private typealias MetalShaderError = MetalError.MetalShaderError

/// Creates a ``MetalShader`` instance from the given shader string for use as a vertex shader.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - shader: C character pointer with the contents of the `libobs` effect file
///   - file: C character pointer with the contents of the `libobs` effect file location
///   - error_string: Pointer for another C character pointer with the contents of an error description
/// - Returns: Opaque pointer to a new ``MetalShader`` instance on success or `nil` on error
///
/// The string pointed to by the `data` argument is a re-compiled shader string created from the associated "effect"
/// file (which will contain multiple effects). Each effect is made up of several passes (though usually only a single
/// pass is defined), each of which contains a vertex and fragment shader. This function is then called with just the
/// vertex shader string.
///
/// This vertex shader string needs to be parsed again and transpiled into a Metal shader string, which is handled by
/// the ``OBSShader`` class. The transpiled string is then used to create the actual ``MetalShader`` instance.
@_cdecl("device_vertexshader_create")
public func device_vertexshader_create(
    device: UnsafeRawPointer, shader: UnsafePointer<CChar>, file: UnsafePointer<CChar>,
    error_string: UnsafeMutablePointer<UnsafeMutablePointer<CChar>>
) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    let content = String(cString: shader)
    let fileLocation = String(cString: file)

    do {
        let obsShader = try OBSShader(type: .vertex, content: content, fileLocation: fileLocation)
        let transpiled = try obsShader.transpiled()

        guard let metaData = obsShader.metaData else {
            OBSLog(.error, "device_vertexshader_create: No required metadata found for transpiled shader")
            return nil
        }

        let metalShader = try MetalShader(device: device, source: transpiled, type: .vertex, data: metaData)

        return metalShader.getRetained()
    } catch let error as ParserError {
        switch error {
        case .parseFail(let description):
            OBSLog(.error, "device_vertexshader_create: Error parsing shader.\n\(description)")
        default:
            OBSLog(.error, "device_vertexshader_create: Error parsing shader.\n\(error.description)")
        }
    } catch let error as ShaderError {
        switch error {
        case .transpileError(let description):
            OBSLog(.error, "device_vertexshader_create: Error transpiling shader.\n\(description)")
        case .parseError(let description):
            OBSLog(.error, "device_vertexshader_create: OBS parser error.\n\(description)")
        case .parseFail(let description):
            OBSLog(.error, "device_vertexshader_create: OBS parser failure.\n\(description)")
        default:
            OBSLog(.error, "device_vertexshader_create: OBS shader error.\n\(error.description)")
        }
    } catch {
        switch error {
        case let error as MetalShaderError:
            OBSLog(.error, "device_vertexshader_create: Error compiling shader.\n\(error.description)")
        case let error as MetalError.MTLDeviceError:
            OBSLog(.error, "device_vertexshader_create: Device error compiling shader.\n\(error.description)")
        default:
            OBSLog(.error, "device_vertexshader_create: Unknown error occurred")
        }
    }

    return nil
}

/// Creates a ``MetalShader`` instance from the given shader string for use as a fragment shader.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - shader: C character pointer with the contents of the `libobs` effect file
///   - file: C character pointer with the contents of the `libobs` effect file location
///   - error_string: Pointer for another C character pointer with the contents of an error description
/// - Returns: Opaque pointer to a new ``MetalShader`` instance on success or `nil` on error
///
/// The string pointed to by the `data` argument is a re-compiled shader string created from the associated "effect"
/// file (which will contain multiple effects). Each effect is made up of several passes (though usually only a single
/// pass is defined), each of which contains a vertex and fragment shader. This function is then called with just the
/// vertex shader string.
///
/// This fragment shader string needs to be parsed again and transpiled into a Metal shader string, which is handled by
/// the ``OBSShader`` class. The transpiled string is then used to create the actual ``MetalShader`` instance.
@_cdecl("device_pixelshader_create")
public func device_pixelshader_create(
    device: UnsafeRawPointer, shader: UnsafePointer<CChar>, file: UnsafePointer<CChar>,
    error_string: UnsafeMutablePointer<UnsafeMutablePointer<CChar>>
) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    let content = String(cString: shader)
    let fileLocation = String(cString: file)

    do {
        let obsShader = try OBSShader(type: .fragment, content: content, fileLocation: fileLocation)
        let transpiled = try obsShader.transpiled()

        guard let metaData = obsShader.metaData else {
            OBSLog(.error, "device_pixelshader_create: No required metadata found for transpiled shader")
            return nil
        }

        let metalShader = try MetalShader(device: device, source: transpiled, type: .fragment, data: metaData)

        return metalShader.getRetained()
    } catch let error as ParserError {
        switch error {
        case .parseFail(let description):
            OBSLog(.error, "device_vertexshader_create: Error parsing shader.\n\(description)")
        default:
            OBSLog(.error, "device_vertexshader_create: Error parsing shader.\n\(error.description)")
        }
    } catch let error as ShaderError {
        switch error {
        case .transpileError(let description):
            OBSLog(.error, "device_vertexshader_create: Error transpiling shader.\n\(description)")
        case .parseError(let description):
            OBSLog(.error, "device_vertexshader_create: OBS parser error.\n\(description)")
        case .parseFail(let description):
            OBSLog(.error, "device_vertexshader_create: OBS parser failure.\n\(description)")
        default:
            OBSLog(.error, "device_vertexshader_create: OBS shader error.\n\(error.description)")
        }
    } catch {
        switch error {
        case let error as MetalShaderError:
            OBSLog(.error, "device_vertexshader_create: Error compiling shader.\n\(error.description)")
        case let error as MetalError.MTLDeviceError:
            OBSLog(.error, "device_vertexshader_create: Device error compiling shader.\n\(error.description)")
        default:
            OBSLog(.error, "device_vertexshader_create: Unknown error occurred")
        }
    }

    return nil
}

/// Loads the ``MetalShader`` instance for use as the vertex shader for the current render pipeline descriptor.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - vertShader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
///
/// This function will simply set up the ``MTLFunction`` wrapped by the ``MetalShader`` instance as the current
/// pipeline descriptor's `vertexFunction`. The Metal renderer will lazily create new render pipeline states for each
/// permutation of pipeline descriptors, which is a comparatively costly operation but will only occur once for any
/// such permutation.
///
/// > Note: If a `NULL` pointer is passed for the `vertShader` argument, the vertex function on the current render
/// pipeline descriptor will be _unset_.
///
@_cdecl("device_load_vertexshader")
public func device_load_vertexshader(device: UnsafeRawPointer, vertShader: UnsafeRawPointer?) {
    let device: MetalDevice = unretained(device)

    if let vertShader {
        let shader: MetalShader = unretained(vertShader)

        guard shader.type == .vertex else {
            assertionFailure("device_load_vertexshader: Invalid shader type \(shader.type)")
            return
        }

        device.renderState.vertexShader = shader
        device.renderState.pipelineDescriptor.vertexFunction = shader.function
        device.renderState.pipelineDescriptor.vertexDescriptor = shader.vertexDescriptor
    } else {
        device.renderState.vertexShader = nil
        device.renderState.pipelineDescriptor.vertexFunction = nil
        device.renderState.pipelineDescriptor.vertexDescriptor = nil
    }
}

/// Loads the ``MetalShader`` instance for use as the fragment shader for the current render pipeline descriptor.
/// - Parameters:
///   - device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
///   - vertShader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
///
/// This function will simply set up the ``MTLFunction`` wrapped by the ``MetalShader`` instance as the current
/// pipeline descriptor's `fragmentFunction`. The Metal renderer will lazily create new render pipeline states for
/// each permutation of pipeline descriptors, which is a comparatively costly operation but will only occur once for
/// any such permutation.
///
/// As any fragment function is potentially associated with a number of textures and associated sampler states, the
/// associated arrays are reset whenever a new fragment function is set up.
///
/// > Note: If a `NULL` pointer is passed for the `pixelShader` argument, the fragment function on the current render
/// pipeline descriptor will be _unset_.
///
@_cdecl("device_load_pixelshader")
public func device_load_pixelshader(device: UnsafeRawPointer, pixelShader: UnsafeRawPointer?) {
    let device: MetalDevice = unretained(device)

    for index in 0..<Int(GS_MAX_TEXTURES) {
        device.renderState.textures[index] = nil
        device.renderState.samplers[index] = nil
    }

    if let pixelShader {
        let shader: MetalShader = unretained(pixelShader)

        guard shader.type == .fragment else {
            assertionFailure("device_load_pixelshader: Invalid shader type \(shader.type)")
            return
        }

        device.renderState.fragmentShader = shader
        device.renderState.pipelineDescriptor.fragmentFunction = shader.function

        if let samplers = shader.samplers {
            device.renderState.samplers.replaceSubrange(0..<samplers.count, with: samplers)
        }
    } else {
        device.renderState.pipelineDescriptor.fragmentFunction = nil
    }
}

/// Gets the ``MetalShader`` set up as the current vertex shader for the pipeline
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Opaque pointer to ``MetalShader`` instance if a vertex shader is currently set up or `nil` otherwise
@_cdecl("device_get_vertex_shader")
public func device_get_vertex_shader(device: UnsafeRawPointer) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    if let shader = device.renderState.vertexShader {
        return shader.getUnretained()
    } else {
        return nil
    }
}

/// Gets the ``MetalShader`` set up as the current fragment shader for the pipeline
/// - Parameter device: Opaque pointer to ``MetalDevice`` instance shared with `libobs`
/// - Returns: Opaque pointer to ``MetalShader`` instance if a fragment shader is currently set up or `nil` otherwise
@_cdecl("device_get_pixel_shader")
public func device_get_pixel_shader(device: UnsafeRawPointer) -> OpaquePointer? {
    let device: MetalDevice = unretained(device)

    if let shader = device.renderState.fragmentShader {
        return shader.getUnretained()
    } else {
        return nil
    }
}

/// Requests the deinitialization of the ``MetalShader`` instance shared with `libobs`
/// - Parameter shader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
///
/// Ownership of the ``MetalShader`` instance will be transferred into the function and if this was the last strong
/// reference to it, the object will be automatically deinitialized and deallocated by Swift.
@_cdecl("gs_shader_destroy")
public func gs_shader_destroy(shader: UnsafeRawPointer) {
    let _ = retained(shader) as MetalShader
}

/// Gets the number of uniform parameters used on the ``MetalShader``
/// - Parameter shader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
/// - Returns: Number of uniforms
@_cdecl("gs_shader_get_num_params")
public func gs_shader_get_num_params(shader: UnsafeRawPointer) -> UInt32 {
    let shader: MetalShader = unretained(shader)

    return UInt32(shader.uniforms.count)
}

/// Gets a uniform parameter from the ``MetalShader`` by its array index
/// - Parameters:
///   - shader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
///   - param: Array index of uniform parameter to get
/// - Returns: Opaque pointer to a ``ShaderUniform`` instance if index within uniform array bounds or `nil` otherwise
///
/// This function requires that the array indices of the uniforms array do not change for a ``MetalShader`` and also
/// that the exact order of uniforms is identical between `libobs`'s interpretation of the effects file and the
/// transpiled shader's analysis of the uniforms.
///
/// > Important: The opaque pointer for the ``ShaderUniform`` instance is passe unretained and as such can become
/// invalid when its owning ``MetalShader`` instance either is deinitialized itself or is replaced in the uniforms
/// array.
@_cdecl("gs_shader_get_param_by_idx")
public func gs_shader_get_param_by_idx(shader: UnsafeRawPointer, param: UInt32) -> OpaquePointer? {
    let shader: MetalShader = unretained(shader)

    guard param < shader.uniforms.count else {
        return nil
    }

    let uniform = shader.uniforms[Int(param)]
    let unretained = Unmanaged.passUnretained(uniform).toOpaque()

    return OpaquePointer(unretained)
}

/// Gets a uniform parameter from the ``MetalShader`` by its name
/// - Parameters:
///   - shader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
///   - param: C character array pointer with the name of the requested uniform parameter
/// - Returns: Opaque pointer to a ``ShaderUniform`` instance if any uniform with the provided name was found or `nil`
/// otherwise
///
/// > Important: The opaque pointer for the ``ShaderUniform`` instance is passe unretained and as such can become
/// invalid when its owning ``MetalShader`` instance either is deinitialized itself or is replaced in the uniforms
/// array.
///
@_cdecl("gs_shader_get_param_by_name")
public func gs_shader_get_param_by_name(shader: UnsafeRawPointer, param: UnsafeMutablePointer<CChar>) -> OpaquePointer?
{
    let shader: MetalShader = unretained(shader)

    let paramName = String(cString: param)

    for uniform in shader.uniforms {
        if uniform.name == paramName {
            let unretained = Unmanaged.passUnretained(uniform).toOpaque()
            return OpaquePointer(unretained)
        }
    }

    return nil
}

/// Gets the uniform parameter associated with the view projection matrix used by the ``MetalShader``
/// - Parameter shader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
/// - Returns: Opaque pointer to a ``ShaderUniform`` instance if a uniform for the view projection matrix was found
/// or `nil` otherwise
///
/// The uniform for the view projection matrix has the associated name `viewProj` in the Metal renderer, thus a
/// name-based lookup is used to find the associated ``ShaderUniform`` instance.
///
/// > Important: The opaque pointer for the ``ShaderUniform`` instance is passe unretained and as such can become
/// invalid when its owning ``MetalShader`` instance either is deinitialized itself or is replaced in the uniforms
/// array.
///
@_cdecl("gs_shader_get_viewproj_matrix")
public func gs_shader_get_viewproj_matrix(shader: UnsafeRawPointer) -> OpaquePointer? {
    let shader: MetalShader = unretained(shader)
    let paramName = "viewProj"

    for uniform in shader.uniforms {
        if uniform.name == paramName {
            let unretained = Unmanaged.passUnretained(uniform).toOpaque()
            return OpaquePointer(unretained)
        }
    }

    return nil
}

/// Gets the uniform parameter associated with the world projection matrix used by the ``MetalShader``
/// - Parameter shader: Opaque pointer to ``MetalShader`` instance shared with `libobs`
/// - Returns: Opaque pointer to a ``ShaderUniform`` instance if a uniform for the world projection matrix was found
/// or `nil` otherwise
///
/// The uniform for the view projection matrix has the associated name `worldProj` in the Metal renderer, thus a
/// name-based lookup is used to find the associated ``ShaderUniform`` instance.
///
/// > Important: The opaque pointer for the ``ShaderUniform`` instance is passe unretained and as such can become
/// invalid when its owning ``MetalShader`` instance either is deinitialized itself or is replaced in the uniforms
/// array.
@_cdecl("gs_shader_get_world_matrix")
public func gs_shader_get_world_matrix(shader: UnsafeRawPointer) -> OpaquePointer? {
    let shader: MetalShader = unretained(shader)
    let paramName = "worldProj"

    for uniform in shader.uniforms {
        if uniform.name == paramName {
            let unretained = Unmanaged.passUnretained(uniform).toOpaque()
            return OpaquePointer(unretained)
        }
    }

    return nil
}

/// Gets the name and uniform type from the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - info: Pointer to a `gs_shader_param_info` struct pre-allocated by `libobs`
///
/// > Warning: The C character array pointer holding the name of the uniform is managed by Swift and might become
/// invalid at any point in time.
@_cdecl("gs_shader_get_param_info")
public func gs_shader_get_param_info(shaderParam: UnsafeRawPointer, info: UnsafeMutablePointer<gs_shader_param_info>) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    shaderUniform.name.withCString {
        info.pointee.name = $0
    }
    info.pointee.type = shaderUniform.gsType
}

/// Sets a boolean value on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: Boolean value to set for the uniform
@_cdecl("gs_shader_set_bool")
public func gs_shader_set_bool(shaderParam: UnsafeRawPointer, val: Bool) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    withUnsafePointer(to: val) {
        shaderUniform.setParameter(data: $0, size: MemoryLayout<Int32>.size)
    }
}

/// Sets a 32-bit floating point value on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: 32-bit floating point value to set for the uniform
@_cdecl("gs_shader_set_float")
public func gs_shader_set_float(shaderParam: UnsafeRawPointer, val: Float32) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    withUnsafePointer(to: val) {
        shaderUniform.setParameter(data: $0, size: MemoryLayout<Float32>.size)
    }
}

/// Sets a 32-bit signed integer value on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: 32-bit signed integer value to set for the uniform
@_cdecl("gs_shader_set_int")
public func gs_shader_set_int(shaderParam: UnsafeRawPointer, val: Int32) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    withUnsafePointer(to: val) {
        shaderUniform.setParameter(data: $0, size: MemoryLayout<Int32>.size)
    }
}

/// Sets a 3x3 matrix of 32-bit floating point values on the ``ShaderUniform``instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: A 3x3 matrix of 32-bit floating point values
///
/// The 3x3 matrix is converted into a 4x4 matrix (padded with zeros) before actually being set as the uniform data
@_cdecl("gs_shader_set_matrix3")
public func gs_shader_set_matrix3(shaderParam: UnsafeRawPointer, val: UnsafePointer<matrix3>) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    var newMatrix = matrix4()
    matrix4_from_matrix3(&newMatrix, val)

    shaderUniform.setParameter(data: &newMatrix, size: MemoryLayout<matrix4>.size)
}

/// Sets a 4x4 matrix of 32-bit floating point values on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: A 4x4 matrix of 32-bit floating point values
@_cdecl("gs_shader_set_matrix4")
public func gs_shader_set_matrix4(shaderParam: UnsafeRawPointer, val: UnsafePointer<matrix4>) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    shaderUniform.setParameter(data: val, size: MemoryLayout<matrix4>.size)
}

/// Sets a vector of 2 32-bit floating point values on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: A vector of 2 32-bit floating point values
@_cdecl("gs_shader_set_vec2")
public func gs_shader_set_vec2(shaderParam: UnsafeRawPointer, val: UnsafePointer<vec2>) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    shaderUniform.setParameter(data: val, size: MemoryLayout<vec2>.size)
}

/// Sets a vector of 3 32-bit floating point values on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: A vector of 3 32-bit floating point values
@_cdecl("gs_shader_set_vec3")
public func gs_shader_set_vec3(shaderParam: UnsafeRawPointer, val: UnsafePointer<vec3>) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    shaderUniform.setParameter(data: val, size: MemoryLayout<vec3>.size)
}

/// Sets a vector of 4 32-bit floating point values on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: A vector of 4 32-bit floating point values
@_cdecl("gs_shader_set_vec4")
public func gs_shader_set_vec4(shaderParam: UnsafeRawPointer, val: UnsafePointer<vec4>) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    shaderUniform.setParameter(data: val, size: MemoryLayout<vec4>.size)
}

/// Sets up the data of a `gs_shader_texture` struct as a uniform on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: A pointer to a `gs_shader_struct` containing an opaque pointer to the actual ``MetalTexture`` instance
///     and an sRGB gamma state flag
///
/// The struct's data is copied verbatim into the uniform, which allows reconstruction of the pointer at a later point
/// as long as the actual ``MetalTexture`` instance still exists.
@_cdecl("gs_shader_set_texture")
public func gs_shader_set_texture(shaderParam: UnsafeRawPointer, val: UnsafePointer<gs_shader_texture>?) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    if let val {
        shaderUniform.setParameter(data: val, size: MemoryLayout<gs_shader_texture>.size)
    }
}

/// Sets an arbitrary value on the ``ShaderUniform`` instance
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - val: Opaque pointer to some unknown data for use as the uniform
///   - size: The size of the data available at the memory pointed to by the `val` argument
///
/// The ``ShaderUniform`` itself is set up to hold a specific uniform type, each of which is associated with a size of
/// bytes required for it. If the size of the data pointed to by `val` does not fit into this size, the uniform will
/// not be updated.
///
/// If the ``ShaderUniform`` expects a texture parameter, the pointer will be bound as memory of a `gs_shader_texture`
/// instance before setting it up.
@_cdecl("gs_shader_set_val")
public func gs_shader_set_val(shaderParam: UnsafeRawPointer, val: UnsafeRawPointer, size: UInt32) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    let size = Int(size)
    let valueSize = shaderUniform.gsType.size

    guard valueSize == size else {
        assertionFailure("gs_shader_set_val: Required size of uniform does not match size of input")
        return
    }

    if shaderUniform.gsType == GS_SHADER_PARAM_TEXTURE {
        let shaderTexture = val.bindMemory(to: gs_shader_texture.self, capacity: 1)

        shaderUniform.setParameter(data: shaderTexture, size: valueSize)
    } else {
        let bytes = val.bindMemory(to: UInt8.self, capacity: valueSize)
        shaderUniform.setParameter(data: bytes, size: valueSize)
    }
}

/// Resets the ``ShaderUniform``'s current data with its default data
/// - Parameter shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///
/// Each ``ShaderUniform`` is optionally set up with a set of default data (stored as an array of bytes) which is
/// simply copied into the current values.
@_cdecl("gs_shader_set_default")
public func gs_shader_set_default(shaderParam: UnsafeRawPointer) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    if let defaultValues = shaderUniform.defaultValues {
        shaderUniform.currentValues = Array(defaultValues)
    }
}

/// Sets up the ``MTLSamplerState`` as the sampler state for the ``ShaderUniform``
/// - Parameters:
///   - shaderParam: Opaque pointer to ``ShaderUniform`` instance shared with `libobs`
///   - sampler: Opaque pointer to ``MTLSamplerState`` instance shared with `libobs`
///
/// If the uniform represents a texture for use in the associated shader, this function will also set up the provided
/// ``MTLSamplerState`` for the associated texture's texture slot.
@_cdecl("gs_shader_set_next_sampler")
public func gs_shader_set_next_sampler(shaderParam: UnsafeRawPointer, sampler: UnsafeRawPointer) {
    let shaderUniform: MetalShader.ShaderUniform = unretained(shaderParam)

    let samplerState = Unmanaged<MTLSamplerState>.fromOpaque(sampler).takeUnretainedValue()

    shaderUniform.samplerState = samplerState
}
