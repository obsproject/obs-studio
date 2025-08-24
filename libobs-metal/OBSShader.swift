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

private enum SampleVariant {
    case load
    case sample
    case sampleBias
    case sampleGrad
    case sampleLevel
}

private struct VariableType: OptionSet {
    var rawValue: UInt

    static let typeUniform = VariableType(rawValue: 1 << 0)
    static let typeStruct = VariableType(rawValue: 1 << 1)
    static let typeStructMember = VariableType(rawValue: 1 << 2)
    static let typeInput = VariableType(rawValue: 1 << 3)
    static let typeOutput = VariableType(rawValue: 1 << 4)
    static let typeTexture = VariableType(rawValue: 1 << 5)
    static let typeConstant = VariableType(rawValue: 1 << 6)

}

private struct OBSShaderFunction {
    let name: String

    var returnType: String
    var typeMap: [String: String]

    var requiresUniformBuffers: Bool
    var textures: [String]
    var samplers: [String]

    var arguments: [OBSShaderVariable]

    let gsFunction: UnsafeMutablePointer<shader_func>
}

private struct OBSShaderVariable {
    let name: String

    var type: String
    var mapping: String?
    var storageType: VariableType

    var requiredBy: Set<String>
    var returnedBy: Set<String>

    var isStage: Bool
    var attributeId: Int?
    var isConstant: Bool
    var isReference: Bool

    let gsVariable: UnsafeMutablePointer<shader_var>
}

private struct OBSShaderStruct {
    let name: String

    var storageType: VariableType
    var members: [OBSShaderVariable]

    let gsVariable: UnsafeMutablePointer<shader_struct>
}

private struct MSLTemplates {
    static let header = """
        #include <metal_stdlib>

        using namespace metal;
        """

    static let variable = "[qualifier] [type] [name] [mapping]"

    static let shaderStruct = """
        typedef struct {
        [variable]
        } [typename];
        """

    static let function = "[decorator] [type] [name]([parameters]) {[content]}"
}

private typealias ParserError = MetalError.OBSShaderParserError
private typealias ShaderError = MetalError.OBSShaderError

class OBSShader {
    private let type: MTLFunctionType
    private let content: String
    private let fileLocation: String

    private var parser: shader_parser
    private var parsed: Bool

    private var uniformsOrder = [String]()
    private var uniforms = [String: OBSShaderVariable]()
    private var structs = [String: OBSShaderStruct]()
    private var functionsOrder = [String]()
    private var functions = [String: OBSShaderFunction]()
    private var referenceVariables = [String]()

    var metaData: MetalShader.ShaderData?

    init(type: MTLFunctionType, content: String, fileLocation: String) throws {
        guard type == .vertex || type == .fragment else {
            throw ShaderError.unsupportedType
        }

        self.type = type
        self.content = content
        self.fileLocation = fileLocation

        self.parsed = false

        self.parser = shader_parser()

        try withUnsafeMutablePointer(to: &parser) {
            shader_parser_init($0)

            let result = shader_parse($0, content.cString(using: .utf8), content.cString(using: .utf8))
            let warnings = shader_parser_geterrors($0)

            if let warnings {
                throw ShaderError.parseError(String(cString: warnings))
            }

            if !result {
                throw ShaderError.parseFail("Shader failed to parse: \(fileLocation)")
            } else {
                self.parsed = true
            }
        }
    }

    /// Transpiles a `libobs` effect string into a Metal Shader Language (MSL) string
    /// - Returns: MSL string representing the transpiled shader
    func transpiled() throws -> String {
        try analyzeUniforms()
        try analyzeParameters()
        try analyzeFunctions()

        let uniforms = try transpileUniforms()
        let structs = try transpileStructs()
        let functions = try transpileFunctions()

        self.metaData = try buildMetadata()

        return [MSLTemplates.header, uniforms, structs, functions].joined(separator: "\n\n")
    }

    /// Builds a metadata object for the current shader
    /// - Returns: ``ShaderData`` object with the shader metadata
    ///
    /// The effects used by `libobs` are written in HLSL with some customizations to allow multiple shaders within the
    /// same effects file (which is supported natively by MSL). As MSL does not support "global" variables, uniforms
    /// have to be provided explicitly via buffers and the data inside those buffers needs to be laid out in the correct
    /// way.
    ///
    /// Uniforms are converted into `struct` objects in the shader files and as MSL is based on C++14, these structs
    /// will have a size, stride, and alignment, set by the compiler. Thus the uniform data used by the shader needs to
    /// be laid out in the buffer according to this alignment.
    ///
    /// The layout of vertex buffer data also needs to be communicated using `MTLVertexDescriptor` instances for vertex
    /// shaders and `MTLSamplerState` instances for fragment shaders. Both will be created and set up in a
    /// ``ShaderData`` which is used to create the actual ``MetalShader`` object.
    private func buildMetadata() throws -> MetalShader.ShaderData {
        var uniformInfo = [MetalShader.ShaderUniform]()

        var textureSlot = 0
        var uniformBufferSize = 0

        /// The order of buffers and uniforms is "load-bearing" as the order (and thus alignment and offsets) of
        /// uniforms in the corresponding uniforms struct are
        /// influenced by it.
        for uniformName in uniformsOrder {
            guard let uniform = uniforms[uniformName] else {
                throw ParserError.parseFail("No uniform data found for '\(uniformName)'")
            }

            let gsType = get_shader_param_type(uniform.gsVariable.pointee.type)
            let isTexture = uniform.storageType.contains(.typeTexture)

            let byteSize: Int
            let alignment: Int
            let bufferOffset: Int

            if isTexture {
                byteSize = 0
                alignment = 0
                bufferOffset = uniformBufferSize
            } else {
                byteSize = gsType.mtlSize
                alignment = gsType.mtlAlignment
                bufferOffset = (uniformBufferSize + (alignment - 1)) & ~(alignment - 1)
            }

            let shaderUniform = MetalShader.ShaderUniform(
                name: uniform.name,
                gsType: gsType,
                textureSlot: (isTexture ? textureSlot : 0),
                samplerState: nil,
                byteOffset: bufferOffset
            )

            shaderUniform.defaultValues = Array(
                UnsafeMutableBufferPointer(
                    start: uniform.gsVariable.pointee.default_val.array,
                    count: uniform.gsVariable.pointee.default_val.num)
            )

            shaderUniform.currentValues = shaderUniform.defaultValues

            uniformBufferSize = bufferOffset + byteSize

            if isTexture {
                textureSlot += 1
            }

            uniformInfo.append(shaderUniform)
        }

        guard let mainFunction = functions["main"] else {
            throw ParserError.missingMainFunction
        }

        let parameterMapper = { (mapping: String) -> MetalBuffer.BufferDataType? in
            switch mapping {
            case "POSITION":
                .vertex
            case "NORMAL":
                .normal
            case "TANGENT":
                .tangent
            case "COLOR":
                .color
            case _ where mapping.hasPrefix("TEXCOORD"):
                .texcoord
            default:
                .none
            }
        }

        let descriptorMapper = { (parameter: OBSShaderVariable) -> (MTLVertexFormat, Int)? in
            guard let mapping = parameter.mapping else {
                return nil
            }

            let type = parameter.type

            switch mapping {
            case "COLOR":
                return (.float4, MemoryLayout<vec4>.size)
            case "POSITION", "NORMAL", "TANGENT":
                return (.float4, MemoryLayout<vec4>.size)
            case _ where mapping.hasPrefix("TEXCOORD"):
                guard let numCoordinates = type[type.index(type.startIndex, offsetBy: 5)].wholeNumberValue else {
                    assertionFailure("Unsupported type \(type) for texture parameter")
                    return nil
                }

                let format: MTLVertexFormat =
                    switch numCoordinates {
                    case 0: .float
                    case 2: .float2
                    case 3: .float3
                    case 4: .float4
                    default: .invalid
                    }

                guard format != .invalid else {
                    assertionFailure("OBSShader: Unsupported amount of texture coordinates '\(numCoordinates)'")
                    return nil
                }

                return (format, MemoryLayout<Float32>.size * numCoordinates)
            case "VERTEXID":
                return nil
            default:
                assertionFailure("OBSShader: Unsupported mapping \(mapping)")
                return nil
            }
        }

        switch type {
        case .vertex:
            var bufferOrder = [MetalBuffer.BufferDataType]()
            var descriptorData = [(MTLVertexFormat, Int)?]()
            let descriptor = MTLVertexDescriptor()

            for argument in mainFunction.arguments {
                if argument.storageType.contains(.typeStruct) {
                    let actualStructType = argument.type.replacingOccurrences(of: "_In", with: "")

                    guard let shaderStruct = structs[actualStructType] else {
                        throw ParserError.parseFail("Shader function without struct metadata encountered ")
                    }

                    for shaderParameter in shaderStruct.members {
                        if let mapping = shaderParameter.mapping, let mapping = parameterMapper(mapping) {
                            bufferOrder.append(mapping)
                        }

                        if let description = descriptorMapper(shaderParameter) {
                            descriptorData.append(description)
                        }
                    }
                } else {
                    if let mapping = argument.mapping, let mapping = parameterMapper(mapping) {
                        bufferOrder.append(mapping)
                    }

                    if let description = descriptorMapper(argument) {
                        descriptorData.append(description)
                    }
                }
            }

            let textureUnitCount = bufferOrder.filter({ $0 == .texcoord }).count

            for (attributeId, description) in descriptorData.filter({ $0 != nil }).enumerated() {
                descriptor.attributes[attributeId].bufferIndex = attributeId
                descriptor.attributes[attributeId].format = description!.0
                descriptor.layouts[attributeId].stride = description!.1
            }

            return MetalShader.ShaderData(
                uniforms: uniformInfo,
                bufferOrder: bufferOrder,
                vertexDescriptor: descriptor,
                samplerDescriptors: nil,
                bufferSize: uniformBufferSize,
                textureCount: textureUnitCount
            )
        case .fragment:
            var samplers = [MTLSamplerDescriptor]()

            for i in 0..<parser.samplers.num {
                let sampler: UnsafeMutablePointer<shader_sampler>? = parser.samplers.array.advanced(by: i)

                if let sampler {
                    var sampler_info = gs_sampler_info()
                    shader_sampler_convert(sampler, &sampler_info)

                    let borderColor: MTLSamplerBorderColor =
                        switch sampler_info.border_color {
                        case 0x00_00_00_FF:
                            .opaqueBlack
                        case 0xFF_FF_FF_FF:
                            .opaqueWhite
                        default:
                            .transparentBlack
                        }

                    let descriptor = MTLSamplerDescriptor()

                    descriptor.borderColor = borderColor
                    descriptor.maxAnisotropy = Int(sampler_info.max_anisotropy)

                    guard
                        let sAddressMode = sampler_info.address_u.mtlMode,
                        let tAddressMode = sampler_info.address_v.mtlMode,
                        let rAddressMode = sampler_info.address_w.mtlMode,
                        let minMagFilter = sampler_info.filter.minMagFilter,
                        let mipFilter = sampler_info.filter.mipFilter
                    else {
                        samplers.append(descriptor)
                        continue
                    }

                    descriptor.sAddressMode = sAddressMode
                    descriptor.tAddressMode = tAddressMode
                    descriptor.rAddressMode = rAddressMode

                    descriptor.minFilter = minMagFilter
                    descriptor.magFilter = minMagFilter
                    descriptor.mipFilter = mipFilter

                    samplers.append(descriptor)
                }
            }

            return MetalShader.ShaderData(
                uniforms: uniformInfo,
                bufferOrder: [],
                vertexDescriptor: nil,
                samplerDescriptors: samplers,
                bufferSize: uniformBufferSize,
                textureCount: 0
            )
        default:
            throw ShaderError.unsupportedType
        }
    }

    /// Analyzes shader uniform parameters parsed by the ``libobs`` shader parser.
    ///
    /// Each global variable declared as a "uniform" is stored as an ``OBSShaderVariable`` struct, which will be
    /// extended with additional metadata by later analystics steps.
    ///
    /// This is necessary as MSL does not support global variables and all data needs to be explicitly provided
    /// via buffer objects, which requires these "unforms" to be wrapped into a single struct and passed as an explicit
    /// buffer object.
    private func analyzeUniforms() throws {
        for i in 0..<parser.params.num {
            let uniform: UnsafeMutablePointer<shader_var>? = parser.params.array.advanced(by: i)

            guard let uniform, let name = uniform.pointee.name, let type = uniform.pointee.type else {
                throw ParserError.parseFail("Uniform is missing name or type information")
            }

            let mapping: String? =
                if let mapping = uniform.pointee.mapping {
                    String(cString: mapping)
                } else {
                    nil
                }

            var data = OBSShaderVariable(
                name: String(cString: name),
                type: String(cString: type),
                mapping: mapping,
                storageType: .typeUniform,
                requiredBy: [],
                returnedBy: [],
                isStage: false,
                attributeId: 0,
                isConstant: (uniform.pointee.var_type == SHADER_VAR_CONST),
                isReference: false,
                gsVariable: uniform
            )

            if self.type == .fragment {
                /// A texture uniform does not contribute to the uniform buffer
                if data.type.hasPrefix("texture") {
                    data.storageType.remove(.typeUniform)
                    data.storageType.insert(.typeTexture)
                }
            }

            uniformsOrder.append(data.name)
            uniforms.updateValue(data, forKey: data.name)

        }
    }

    /// Analyzes struct parameter declarations parsed by the ``libobs`` shader parser.
    ///
    /// Structured data declarations are used to pass data into and out of shaders.
    ///
    /// Whereas HLSL allows one to use "InOut" structures with attribute mappings (e.g., using the same type defintion
    /// for vertex data going in and out of a vertex shader), MSL does not allow the mixing of input mappings and output
    /// mappings in the same type definition.
    ///
    /// Thus when the same struct type is used as an input argument for a function but also used as its output type, it
    /// needs to be split up into two separate types for the MSL shader.
    ///
    /// This function will first detect all struct type definitions in the shader file and then check if it is used as
    /// an input argument or function output and update the associated ``OBSShaderVariable`` structs accordingly.
    private func analyzeParameters() throws {
        for i in 0..<parser.structs.num {
            let shaderStruct: UnsafeMutablePointer<shader_struct>? = parser.structs.array.advanced(by: i)

            guard let shaderStruct, let name = shaderStruct.pointee.name else {
                throw ParserError.parseFail("Constant data struct has no name")
            }

            var parameters = [OBSShaderVariable]()
            parameters.reserveCapacity(shaderStruct.pointee.vars.num)

            for j in 0..<shaderStruct.pointee.vars.num {
                let variablePointer: UnsafeMutablePointer<shader_var>? = shaderStruct.pointee.vars.array.advanced(by: j)

                guard let variablePointer, let variableName = variablePointer.pointee.name,
                    let variableType = variablePointer.pointee.type
                else {
                    throw ParserError.parseFail("Constant data variable has no name")
                }

                let mapping: String? =
                    if let variableMapping = variablePointer.pointee.mapping { String(cString: variableMapping) } else {
                        nil
                    }

                let variable = OBSShaderVariable(
                    name: String(cString: variableName),
                    type: String(cString: variableType),
                    mapping: mapping,
                    storageType: .typeStructMember,
                    requiredBy: [],
                    returnedBy: [],
                    isStage: false,
                    attributeId: nil,
                    isConstant: false,
                    isReference: false,
                    gsVariable: variablePointer
                )

                parameters.append(variable)
            }

            let data = OBSShaderStruct(
                name: String(cString: name),
                storageType: [],
                members: parameters,
                gsVariable: shaderStruct
            )

            structs.updateValue(data, forKey: data.name)
        }

        for i in 0..<parser.funcs.num {
            let function: UnsafeMutablePointer<shader_func>? = parser.funcs.array.advanced(by: i)

            guard let function, let functionName = function.pointee.name, let returnType = function.pointee.return_type
            else {
                throw ParserError.parseFail("Shader function has no name or type information")
            }

            var functionData = OBSShaderFunction(
                name: String(cString: functionName),
                returnType: String(cString: returnType),
                typeMap: [:],
                requiresUniformBuffers: false,
                textures: [],
                samplers: [],
                arguments: [],
                gsFunction: function,
            )

            for j in 0..<function.pointee.params.num {
                let parameter: UnsafeMutablePointer<shader_var>? = function.pointee.params.array.advanced(by: j)

                guard let parameter, let parameterName = parameter.pointee.name,
                    let parameterType = parameter.pointee.type
                else {
                    throw ParserError.parseFail("Function parameter has no name or type information")
                }

                let mapping: String? =
                    if let parameterMapping = parameter.pointee.mapping {
                        String(cString: parameterMapping)
                    } else {
                        nil
                    }

                /// Most effects do not seem to use `out` or `inout` function arguments, but the lanczos scale filter
                /// does. The most straight-forward way
                /// to support this pattern is to use C++-style references with the `thread` storage specifier.
                let isReferenceVariable =
                    (parameter.pointee.var_type == SHADER_VAR_OUT || parameter.pointee.var_type == SHADER_VAR_INOUT)

                var parameterData = OBSShaderVariable(
                    name: String(cString: parameterName),
                    type: String(cString: parameterType),
                    mapping: mapping,
                    storageType: .typeInput,
                    requiredBy: [functionData.name],
                    returnedBy: [],
                    isStage: false,
                    attributeId: nil,
                    isConstant: (parameter.pointee.var_type == SHADER_VAR_CONST),
                    isReference: isReferenceVariable,
                    gsVariable: parameter
                )

                if isReferenceVariable {
                    referenceVariables.append(parameterData.name)
                }

                if parameterData.type == functionData.returnType {
                    parameterData.returnedBy.insert(functionData.name)
                }

                if !functionData.typeMap.keys.contains(parameterData.name) {
                    functionData.typeMap.updateValue(parameterData.type, forKey: parameterData.name)
                }

                /// Metal does not support using the same attribute mappings for structs as input to shader functions
                /// and output. They need to use different
                /// mappings and thus every "InOut" struct by `libobs` needs to be split up into a separate input and
                /// output struct type.
                for var shaderStruct in structs.values {
                    if shaderStruct.name == parameterData.type {
                        shaderStruct.storageType.insert(.typeInput)
                        parameterData.storageType.insert(.typeStruct)

                        if shaderStruct.name == functionData.returnType {
                            shaderStruct.storageType.insert(.typeOutput)
                            parameterData.storageType.insert(.typeOutput)
                            parameterData.type.append("_In")
                            functionData.returnType.append("_Out")
                        }

                        structs.updateValue(shaderStruct, forKey: shaderStruct.name)
                    }
                }

                functionData.arguments.append(parameterData)
            }

            if var shaderStruct = structs[functionData.returnType] {
                shaderStruct.storageType.insert(.typeOutput)
                structs.updateValue(shaderStruct, forKey: shaderStruct.name)
            }

            functions.updateValue(functionData, forKey: functionData.name)
        }
    }

    /// Analyzes function data parsed by the ``libobs`` shader parser
    ///
    /// As MSL does not support uniforms or using the same struct type for input and output, function bodies themselves
    /// need to be parsed again and checked for their usage of these types or variables.
    ///
    /// Due to the way that the ``libobs`` parser works, each body of a block (either within curly braces or
    /// parentheses) is analyzed recursively and updating the same ``OBSShaderFunction`` struct.
    ///
    /// After a full analysis pass, this struct should contain  information about all uniforms, textures, and samplers
    /// used (or passed on) by the function.
    private func analyzeFunctions() throws {
        for i in 0..<parser.funcs.num {
            let function: UnsafeMutablePointer<shader_func>? = parser.funcs.array.advanced(by: i)

            guard var function, var token = function.pointee.start, let functionName = function.pointee.name else {
                throw ParserError.parseFail("Shader function has no name")
            }

            let functionData = functions[String(cString: functionName)]

            guard var functionData else {
                throw ParserError.parseFail("Shader function without function meta data encountered")
            }

            try analyzeFunction(function: &function, functionData: &functionData, token: &token, end: "}")

            functionData.textures = functionData.textures.unique()
            functionData.samplers = functionData.samplers.unique()

            functions.updateValue(functionData, forKey: functionData.name)
            functionsOrder.append(functionData.name)
        }
    }

    /// Analyzes a function body or source scope to check for use of global variables, textures, or samplers.
    ///
    /// Because MSL does not support global variables, unforms, textures, or samplers need to be passed explicitly to a
    /// function. This requires scanning the entire function body (recursively in the case of separate function scopes
    /// denoted by curvy brackets or parantheses) for any occurrence of a known uniform, texture, or sampler variable
    /// name.
    ///
    /// - Parameters:
    ///   - function: Pointer to a ``shader_func`` element representing a parsed shader function
    ///   - functionData: Reference to a ``OBSShaderFunction`` struct, which will be updated by this function
    ///   - token: Pointer to a ``cf_token`` element used to interact with the shader parser provided by ``libobs``
    ///   - end: The sentinel character at which analysis (and parsing) should stop
    private func analyzeFunction(
        function: inout UnsafeMutablePointer<shader_func>, functionData: inout OBSShaderFunction,
        token: inout UnsafeMutablePointer<cf_token>, end: String
    ) throws {
        let uniformNames =
            (uniforms.filter {
                !$0.value.storageType.contains(.typeTexture)
            }).keys

        while token.pointee.type != CFTOKEN_NONE {
            token = token.successor()

            if token.pointee.str.isEqualTo(end) {
                break
            }

            let stringToken = token.pointee.str.getString()

            if token.pointee.type == CFTOKEN_NAME {
                if uniformNames.contains(stringToken) && functionData.requiresUniformBuffers == false {
                    functionData.requiresUniformBuffers = true
                }

                if let function = functions[stringToken] {
                    if function.requiresUniformBuffers && functionData.requiresUniformBuffers == false {
                        functionData.requiresUniformBuffers = true
                    }

                    functionData.textures.append(contentsOf: function.textures)
                    functionData.samplers.append(contentsOf: function.samplers)
                }

                if type == .fragment {
                    for uniform in uniforms.values {
                        if stringToken == uniform.name && uniform.storageType.contains(.typeTexture) {
                            functionData.textures.append(stringToken)
                        }
                    }

                    for i in 0..<parser.samplers.num {
                        let sampler: UnsafeMutablePointer<shader_sampler>? = parser.samplers.array.advanced(by: i)

                        guard let sampler, let samplerName = sampler.pointee.name else {
                            break
                        }

                        if stringToken == String(cString: samplerName) {
                            functionData.samplers.append(stringToken)
                        }
                    }
                }
            } else if token.pointee.type == CFTOKEN_OTHER {
                if token.pointee.str.isEqualTo("{") {
                    try analyzeFunction(function: &function, functionData: &functionData, token: &token, end: "}")
                } else if token.pointee.str.isEqualTo("(") {
                    try analyzeFunction(function: &function, functionData: &functionData, token: &token, end: ")")
                }
            }
        }
    }

    /// Transpiles the uniform global variables used by the shader into a `UniformData` struct that contains the
    /// uniforms.
    /// - Returns: String representing the uniform data struct
    private func transpileUniforms() throws -> String {
        var output = [String]()

        for uniformName in uniformsOrder {
            if var uniform = uniforms[uniformName] {
                uniform.isStage = false
                uniform.attributeId = 0

                if !uniform.storageType.contains(.typeTexture) {
                    let variableString = try transpileVariable(variable: uniform)
                    output.append("\(variableString);")
                }
            }
        }

        if output.count > 0 {
            let replacements = [
                ("[variable]", output.joined(separator: "\n")),
                ("[typename]", "UniformData"),
            ]

            let uniformString = replacements.reduce(into: MSLTemplates.shaderStruct) { string, replacement in
                string = string.replacingOccurrences(of: replacement.0, with: replacement.1)
            }

            return uniformString
        } else {
            return ""
        }
    }

    /// Transpiles the vertex data structs used by the shader
    /// - Returns: String representing the vertex data structs
    private func transpileStructs() throws -> String {
        var output = [String]()

        for var shaderStruct in structs.values {
            if shaderStruct.storageType.isSuperset(of: [.typeInput, .typeOutput]) {
                /// Metal does not support using the same attribute mappings for structs as input to shader functions
                /// and output. They need to use different mappings and thus every "InOut" struct by `libobs` needs to
                /// be split up into a separate input and output struct type.
                for suffix in ["_In", "_Out"] {
                    var variables = [String]()

                    for (structVariableId, var structVariable) in shaderStruct.members.enumerated() {
                        let variableString: String

                        switch suffix {
                        case "_In":
                            structVariable.storageType.formUnion([.typeInput])
                            structVariable.attributeId = structVariableId
                            variableString = try transpileVariable(variable: structVariable)
                            structVariable.storageType.remove([.typeInput])
                        case "_Out":
                            structVariable.storageType.formUnion([.typeOutput])
                            variableString = try transpileVariable(variable: structVariable)
                            structVariable.storageType.remove([.typeOutput])
                        default:
                            throw ParserError.parseFail("Shader struct with unknown prefix encountered")
                        }

                        variables.append("\(variableString);")
                        shaderStruct.members[structVariableId] = structVariable
                    }

                    let replacements = [
                        ("[variable]", variables.joined(separator: "\n")),
                        ("[typename]", "\(shaderStruct.name)\(suffix)"),
                    ]

                    let result = replacements.reduce(into: MSLTemplates.shaderStruct) {
                        string, replacement in
                        string = string.replacingOccurrences(of: replacement.0, with: replacement.1)
                    }

                    output.append(result)
                }
            } else {
                var variables = [String]()

                for (structVariableId, var structVariable) in shaderStruct.members.enumerated() {
                    if shaderStruct.storageType.contains(.typeInput) {
                        structVariable.storageType.insert(.typeInput)
                        structVariable.attributeId = structVariableId
                    } else if shaderStruct.storageType.contains(.typeOutput) {
                        structVariable.storageType.insert(.typeOutput)
                    }

                    let variableString = try transpileVariable(variable: structVariable)

                    structVariable.storageType.subtract([.typeInput, .typeOutput])

                    variables.append("\(variableString);")
                    shaderStruct.members[structVariableId] = structVariable
                }

                let replacements = [
                    ("[variable]", variables.joined(separator: "\n")),
                    ("[typename]", shaderStruct.name),
                ]

                let result = replacements.reduce(into: MSLTemplates.shaderStruct) {
                    string, replacement in
                    string = string.replacingOccurrences(of: replacement.0, with: replacement.1)
                }

                output.append(result)
            }
        }

        if output.count > 0 {
            return output.joined(separator: "\n\n")
        } else {
            return ""
        }
    }

    /// Transpiles a shader function into its MSL variant
    /// - Returns: String representing the transpiled MSL shader function
    private func transpileFunctions() throws -> String {
        var output = [String]()

        for functionName in functionsOrder {
            guard let function = functions[functionName], var token = function.gsFunction.pointee.start else {
                throw ParserError.parseFail("Shader function has no name")
            }

            var stageConsumed = false
            let isMain = functionName == "main"

            var variables = [String]()
            for var variable in function.arguments {
                if isMain && !stageConsumed {
                    variable.isStage = true
                    stageConsumed = true
                }

                try variables.append(transpileVariable(variable: variable))
            }

            /// As Metal has no support for global constants, the constant data needs to be wrapped into a `struct`
            /// and the associated data is uploaded into a vertex buffer at a specific index (30 in this case).
            ///
            /// Buffers are not automatically available to shader functions but are passed into the function explicitly
            ///as arguments.
            ///
            /// As `libobs` effects are based around a "main" entry function (something strongly discouraged by Metal),
            /// each "main" function needs to receive the actual buffer as an argument and each function called _by_
            /// the main function and which internally accesses the uniform needs to have that uniform passed
            /// explicitly as an argument as well.
            if (uniforms.values.filter { !$0.storageType.contains(.typeTexture) }).count > 0 {
                if isMain {
                    variables.append("constant UniformData &uniforms [[buffer(30)]]")
                } else if function.requiresUniformBuffers {
                    variables.append("constant UniformData &uniforms")
                }
            }

            if type == .fragment {
                var textureId = 0

                for uniformName in uniformsOrder {
                    guard let uniform = uniforms[uniformName] else {
                        break
                    }

                    if uniform.storageType.contains(.typeTexture) {
                        if isMain {
                            let variableString = try transpileVariable(variable: uniform)

                            variables.append("\(variableString) [[texture(\(textureId))]]")
                            textureId += 1
                        } else if function.textures.contains(uniform.name) {
                            let variableString = try transpileVariable(variable: uniform)
                            variables.append(variableString)
                        }
                    }
                }

                var samplerId = 0
                for i in 0..<parser.samplers.num {
                    let sampler: UnsafeMutablePointer<shader_sampler>? = parser.samplers.array.advanced(by: i)

                    if let sampler, let samplerName = sampler.pointee.name {
                        let name = String(cString: samplerName)

                        if isMain {
                            let variableString = "sampler \(name) [[sampler(\(samplerId))]]"
                            variables.append(variableString)
                            samplerId += 1
                        } else if function.samplers.contains(name) {
                            let variabelString = "sampler \(name)"
                            variables.append(variabelString)
                        }
                    }
                }
            }

            let mappedType = try convertToMTLType(gsType: function.returnType)

            let functionContent: String
            var replacements = [(String, String)]()

            /// Metal shaders do not have "main" functions - a single shader file usually contains all shader functions
            /// used by an application, each identified by their name and type decorator. This is not supported by OBS,
            /// so each shader needs to have a "main" function that calls the actual shader function, which thus
            /// requires a new shader library to be created for each effect file.
            if isMain {
                replacements = [
                    ("[name]", "_main"),
                    ("[parameters]", variables.joined(separator: ", ")),
                ]

                switch type {
                case .vertex:
                    replacements.append(("[decorator]", "[[vertex]]"))
                case .fragment:
                    replacements.append(("[decorator]", "[[fragment]]"))
                default:
                    fatalError("OBSShader: Unsupported shader type \(type)")
                }

                let temporaryContent = try transpileFunctionContent(token: &token, end: "}")

                if type == .fragment && isMain && mappedType == "float3" {
                    replacements.append(("[type]", "float4"))

                    // TODO: Replace with Swift-native Regex once macOS 13+ is minimum target
                    let regex = try NSRegularExpression(pattern: "return (.+);")
                    functionContent = regex.stringByReplacingMatches(
                        in: temporaryContent,
                        range: NSRange(location: 0, length: temporaryContent.count),
                        withTemplate: "return float4($1, 1);"
                    )
                } else {
                    functionContent = temporaryContent
                    replacements.append(("[type]", mappedType))
                }

                replacements.append(("[content]", functionContent))
            } else {
                functionContent = try transpileFunctionContent(token: &token, end: "}")

                replacements = [
                    ("[decorator]", ""),
                    ("[type]", mappedType),
                    ("[name]", function.name),
                    ("[parameters]", variables.joined(separator: ", ")),
                    ("[content]", functionContent),
                ]
            }

            let result = replacements.reduce(into: MSLTemplates.function) {
                string, replacement in
                string = string.replacingOccurrences(of: replacement.0, with: replacement.1)
            }

            output.append(result)
        }

        if output.count > 0 {
            return output.joined(separator: "\n\n")
        } else {
            return ""
        }
    }

    /// Transpiles a variable into its MSL variant
    /// - Parameter variable: Variable to transpile
    /// - Returns: String representing a transpiled variable
    ///
    /// Variables can either be members of a `struct` or an argument to a function. The ``OBSShaderVariable`` instance
    /// has a `storageType` property which encodes the use of the variable and helps in creation of the appropriate MSL
    /// string representation.
    private func transpileVariable(variable: OBSShaderVariable) throws -> String {
        var mappings = [String]()

        var metalMapping: String
        var indent = 0

        let metalType = try convertToMTLType(gsType: variable.type)

        if variable.storageType.contains(.typeUniform) {
            indent = 4
        } else if variable.storageType.isSuperset(of: [.typeInput, .typeStructMember]) {
            switch type {
            case .vertex:
                indent = 4

                /// Attributes are used to associate a member of a uniform `struct` with its data in the vertex buffer
                /// stage.
                if let attributeId = variable.attributeId {
                    mappings.append("attribute(\(attributeId))")
                }
            case .fragment:
                indent = 4

                if let mappingPointer = variable.gsVariable.pointee.mapping,
                    let mappedString = convertToMTLMapping(gsMapping: String(cString: mappingPointer))
                {
                    mappings.append(mappedString)
                }
            default:
                fatalError("OBSShader: Unsupported shader function type \(type)")
            }
        } else if variable.storageType.isSuperset(of: [.typeOutput, .typeStructMember]) {
            indent = 4

            if let mappingPointer = variable.gsVariable.pointee.mapping,
                let mappedString = convertToMTLMapping(gsMapping: String(cString: mappingPointer))
            {
                mappings.append(mappedString)
            }
        } else {
            indent = 0

            if variable.isStage {
                if let mappingPointer = variable.gsVariable.pointee.mapping,
                    let mappedString = convertToMTLMapping(gsMapping: String(cString: mappingPointer))
                {
                    mappings.append(mappedString)
                } else {
                    mappings.append("stage_in")
                }
            }
        }

        if mappings.count > 0 {
            metalMapping = " [[\(mappings.joined(separator: ", "))]]"
        } else {
            metalMapping = ""
        }

        let qualifier =
            if variable.storageType.contains(.typeConstant) {
                " constant "
            } else if variable.isReference {
                " thread "
            } else {
                ""
            }

        let name =
            if variable.isReference {
                "&\(variable.name)"
            } else { variable.name }

        let result = "\(String(repeating: " ", count: indent))\(qualifier)\(metalType) \(name)\(metalMapping)"

        return result
    }

    /// Transpiles the body of a function into its MSL representation
    /// - Parameters:
    ///   - token: Stateful `libobs` parser token pointer
    ///   - end: String representing which ends function body parsing if matched
    /// - Returns: String representing the body of a MSL shader function
    ///
    /// OBS effect function content needs to be transpiled into MSL function content token by token, as each token
    /// needs to be matched not only against direct translations (e.g., a HLSL function name into its appropriate MSL
    /// variant) but also to detect if a token represents a uniform variable which will not be available as a global
    /// variable in MSL, but instead will only exist as part of the `uniform` struct that was  explicitly passed into
    /// the function.
    ///
    /// Similarly, if a function call is encountered, the function's metadata needs to be checked for use of such a
    /// uniform and the call signature extended to explicitly pass the data into the called function.
    ///
    /// Because Metal does not implicitly or automagically coerce types (but the effects files sometimes rely on this),
    /// some arguments and parameters need to be explicitly wrapped in casts to wider types (e.g., a `float3` is
    /// returned from a fragment shader, but fragment shaders _have to_ provide a `float4`).
    ///
    /// There are many such conversions necessary, as MSL is more strict than HLSL or GLSL when it comes to type safety.
    private func transpileFunctionContent(token: inout UnsafeMutablePointer<cf_token>, end: String) throws -> String {
        var content = [String]()

        while token.pointee.type != CFTOKEN_NONE {
            token = token.successor()

            if token.pointee.str.isEqualTo(end) {
                break
            }

            let stringToken = token.pointee.str.getString()

            if token.pointee.type == CFTOKEN_NAME {
                let type = try convertToMTLType(gsType: stringToken)

                if stringToken == "obs_glsl_compile" {
                    content.append("false")
                    continue
                }

                if type != stringToken {
                    content.append(type)
                    continue
                }

                if let intrinsic = try convertToMTLIntrinsic(intrinsic: stringToken) {
                    content.append(intrinsic)
                    continue
                }

                if stringToken == "mul" {
                    try content.append(convertToMTLMultiplication(token: &token))
                    continue
                } else if stringToken == "mad" {
                    try content.append(convertToMTLMultiplyAdd(token: &token))
                    continue
                } else {
                    var skip = false
                    for uniform in uniforms.values {
                        if uniform.name == stringToken && uniform.storageType.contains(.typeTexture) {
                            try content.append(createSampler(token: &token))
                            skip = true
                            break
                        }
                    }

                    if skip {
                        continue
                    }
                }

                if uniforms.keys.contains(stringToken) {
                    let priorToken = token.predecessor()
                    let priorString = priorToken.pointee.str.getString()

                    if priorString != "." {
                        content.append("uniforms.\(stringToken)")
                        continue
                    }
                }

                var skip = false
                for shaderStruct in structs.values {
                    if shaderStruct.name == stringToken {
                        if shaderStruct.storageType.isSuperset(of: [.typeInput, .typeOutput]) {
                            content.append("\(stringToken)_Out")
                            skip = true
                            break
                        }
                    }
                }

                if skip {
                    continue
                }

                if let comparison = try convertToMTLComparison(token: &token) {
                    content.append(comparison)
                    continue
                }

                content.append(stringToken)
            } else if token.pointee.type == CFTOKEN_OTHER {
                if token.pointee.str.isEqualTo("{") {
                    let blockContent = try transpileFunctionContent(token: &token, end: "}")
                    content.append("{\(blockContent)}")
                    continue
                } else if token.pointee.str.isEqualTo("(") {
                    let priorToken = token.predecessor()
                    let functionName = priorToken.pointee.str.getString()

                    var functionParameters = [String]()

                    let parameters = try transpileFunctionContent(token: &token, end: ")")

                    if functionName == "int3" {
                        let intParameters = parameters.split(
                            separator: ",", maxSplits: 3, omittingEmptySubsequences: true)

                        switch intParameters.count {
                        case 3:
                            functionParameters.append(
                                "int(\(intParameters[0])), int(\(intParameters[1])), int(\(intParameters[2]))")
                        case 2:
                            functionParameters.append("int2(\(intParameters[0])), int(\(intParameters[1]))")
                        case 1:
                            functionParameters.append("\(intParameters)")
                        default:
                            throw ParserError.parseFail("int3 constructor with invalid amount of arguments encountered")
                        }
                    } else {
                        functionParameters.append(parameters)
                    }

                    if let additionalArguments = generateAdditionalArguments(for: functionName) {
                        functionParameters.append(additionalArguments)
                    }

                    content.append("(\(functionParameters.joined(separator: ", ")))")
                    continue
                }

                content.append(stringToken)
            } else {
                content.append(stringToken)
            }
        }

        return content.joined()
    }

    /// Converts a HLSL-like type into a MSL type if possible
    /// - Parameter gsType: HLSL-like type string
    /// - Returns: MSL type string
    private func convertToMTLType(gsType: String) throws -> String {
        switch gsType {
        case "texture2d":
            return "texture2d<float>"
        case "texture3d":
            return "texture3d<float>"
        case "texture_cube":
            return "texturecube<float>"
        case "texture_rect":
            throw ParserError.unsupportedType
        case "half2":
            return "float2"
        case "half3":
            return "float3"
        case "half4":
            return "float4"
        case "half":
            return "float"
        case "min16float2":
            return "half2"
        case "min16float3":
            return "half3"
        case "min16float4":
            return "half4"
        case "min16float":
            return "half"
        case "min10float":
            throw ParserError.unsupportedType
        case "double":
            throw ParserError.unsupportedType
        case "min16int2":
            return "short2"
        case "min16int3":
            return "short3"
        case "min16int4":
            return "short4"
        case "min16int":
            return "short"
        case "min16uint2":
            return "ushort2"
        case "min16uint3":
            return "ushort3"
        case "min16uint4":
            return "ushort4"
        case "min16uint":
            return "ushort"
        case "min13int":
            throw ParserError.unsupportedType
        default:
            return gsType
        }
    }

    /// Converts an HLSL-like uniform mapping into a MSL attribute decoration if possible
    /// - Parameter gsMapping: HLSL-like mapping
    /// - Returns: MSL attribute string
    private func convertToMTLMapping(gsMapping: String) -> String? {
        switch gsMapping {
        case "POSITION":
            return "position"
        case "VERTEXID":
            return "vertex_id"
        default:
            return nil
        }
    }

    /// Converts a HLSL-like comparison to a vector-safe MSL comparison operation
    /// - Parameter token: Start token of the comparison in the function body
    /// - Returns: MSL comparison operation
    ///
    /// A comparison operation that involves a vector will always result in a boolean vector in MSL (and not a scalar
    /// vector). Thus any functions that compares two vectors will also result in a vector
    /// (e.g., float2 == float2 -> bool2). This will break when a ternary expression is used, as the first element of
    /// it needs to be as scalar boolean in MSL.
    ///
    /// Wrapping the comparison in `all` ensures that a single scalar `true` is returned if all elements of the
    /// resulting boolean vectors are `true` as well.
    private func convertToMTLComparison(token: inout UnsafeMutablePointer<cf_token>) throws -> String? {
        var isComparator = false

        let nextToken = token.successor()

        if nextToken.pointee.type == CFTOKEN_OTHER {
            let comparators = ["==", "!=", "<", "<=", ">=", ">"]

            for comparator in comparators {
                if nextToken.pointee.str.isEqualTo(comparator) {
                    isComparator = true
                    break
                }
            }
        }

        if isComparator {
            var cfp = parser.cfp
            cfp.cur_token = token

            let lhs = cfp.cur_token.pointee.str.getString()

            guard cfp.advanceToken() else { throw ParserError.missingNextToken }

            let comparator = cfp.cur_token.pointee.str.getString()

            guard cfp.advanceToken() else { throw ParserError.missingNextToken }

            let rhs = cfp.cur_token.pointee.str.getString()

            return "all(\(lhs) \(comparator) \(rhs))"
        } else {
            return nil
        }
    }

    /// Converts HLSL-like intrinsic into its MSL representation
    /// - Parameter intrinsic: HLSL-like intrinsic string
    /// - Returns: MSL intrinsic string
    private func convertToMTLIntrinsic(intrinsic: String) throws -> String? {
        switch intrinsic {
        case "clip":
            throw ParserError.unsupportedType
        case "ddx":
            return "dfdx"
        case "ddy":
            return "dfdy"
        case "frac":
            return "fract"
        case "lerp":
            return "mix"
        default:
            return nil
        }
    }

    /// Converts a HLSL-like multiplication function call into a direct multiplication
    /// - Parameter token: Start token of the multiplication in the function body
    /// - Returns: MSL multiplication string
    private func convertToMTLMultiplication(token: inout UnsafeMutablePointer<cf_token>) throws -> String {
        var cfp = parser.cfp
        cfp.cur_token = token

        guard cfp.advanceToken() else { throw ParserError.missingNextToken }
        guard cfp.tokenIsEqualTo("(") else { throw ParserError.unexpectedToken }
        guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

        let lhs = try transpileFunctionContent(token: &cfp.cur_token, end: ",")

        guard cfp.advanceToken() else { throw ParserError.missingNextToken }

        cfp.cur_token = cfp.cur_token.predecessor()

        let rhs = try transpileFunctionContent(token: &cfp.cur_token, end: ")")

        token = cfp.cur_token

        return "(\(lhs)) * (\(rhs))"
    }

    /// Converts a HLSL-like multiply+add function call into a direct multiplication followed by addition
    /// - Parameter token: Start token of the multiply+add in the function body
    /// - Returns: MSL multiplication and addition string
    private func convertToMTLMultiplyAdd(token: inout UnsafeMutablePointer<cf_token>) throws -> String {
        var cfp = parser.cfp
        cfp.cur_token = token

        guard cfp.advanceToken() else { throw ParserError.missingNextToken }
        guard cfp.tokenIsEqualTo("(") else { throw ParserError.unexpectedToken }
        guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

        let first = try transpileFunctionContent(token: &cfp.cur_token, end: ",")

        guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

        let second = try transpileFunctionContent(token: &cfp.cur_token, end: ",")

        guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

        let third = try transpileFunctionContent(token: &cfp.cur_token, end: ")")

        token = cfp.cur_token

        return "((\(first)) * (\(second))) + (\(third))"
    }

    /// Creates an MSL sampler call from a HLSL-like sampler call
    /// - Parameter token: Start token of the sampler call in the function
    /// - Returns: String of an MSL sampler call
    private func createSampler(token: inout UnsafeMutablePointer<cf_token>) throws -> String {
        var cfp = parser.cfp
        cfp.cur_token = token

        let stringToken = token.pointee.str.getString()

        guard cfp.advanceToken() else { throw ParserError.missingNextToken }
        guard cfp.tokenIsEqualTo(".") else { throw ParserError.unexpectedToken }
        guard cfp.advanceToken() else { throw ParserError.missingNextToken }
        guard cfp.cur_token.pointee.type == CFTOKEN_NAME else { throw ParserError.unexpectedToken }

        let textureCall: String

        if cfp.tokenIsEqualTo("Sample") {
            textureCall = try createTextureCall(token: &cfp.cur_token, callType: .sample)
        } else if cfp.tokenIsEqualTo("SampleBias") {
            textureCall = try createTextureCall(token: &cfp.cur_token, callType: .sampleBias)
        } else if cfp.tokenIsEqualTo("SampleGrad") {
            textureCall = try createTextureCall(token: &cfp.cur_token, callType: .sampleGrad)
        } else if cfp.tokenIsEqualTo("SampleLevel") {
            textureCall = try createTextureCall(token: &cfp.cur_token, callType: .sampleLevel)
        } else if cfp.tokenIsEqualTo("Load") {
            textureCall = try createTextureCall(token: &cfp.cur_token, callType: .load)
        } else {
            throw ParserError.missingNextToken
        }

        token = cfp.cur_token
        return "\(stringToken).\(textureCall)"
    }

    /// Creates a MSL sampler call based on the sampling type
    /// - Parameters:
    ///   - token: Start token of the sampler call arguments in the function body
    ///   - callType: Type of sampling used
    /// - Returns: String of an MSL sampler call
    private func createTextureCall(token: inout UnsafeMutablePointer<cf_token>, callType: SampleVariant) throws
        -> String
    {
        var cfp = parser.cfp
        cfp.cur_token = token

        guard cfp.advanceToken() else { throw ParserError.missingNextToken }
        guard cfp.tokenIsEqualTo("(") else { throw ParserError.unexpectedToken }
        guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

        switch callType {
        case .sample:
            let first = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let second = try transpileFunctionContent(token: &cfp.cur_token, end: ")")

            token = cfp.cur_token
            return "sample(\(first), \(second))"
        case .sampleBias:
            let first = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let second = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let third = try transpileFunctionContent(token: &cfp.cur_token, end: ")")

            token = cfp.cur_token
            return "sample(\(first), \(second), bias(\(third)))"
        case .sampleGrad:
            let first = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let second = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let third = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let fourth = try transpileFunctionContent(token: &cfp.cur_token, end: ")")

            token = cfp.cur_token
            return "sample(\(first), \(second), gradient2d(\(third), \(fourth)))"
        case .sampleLevel:
            let first = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let second = try transpileFunctionContent(token: &cfp.cur_token, end: ",")
            guard cfp.hasNextToken() else { throw ParserError.missingNextToken }

            let third = try transpileFunctionContent(token: &cfp.cur_token, end: ")")

            token = cfp.cur_token
            return "sample(\(first), \(second), level(\(third)))"
        case .load:
            let first = try transpileFunctionContent(token: &cfp.cur_token, end: ")")

            let loadCall: String

            /// Many load calls in OBS effects files rely on implicit type conversion, which is not allowed in MSL in
            /// addition to `read` calls only accepting a `uint2` followed by a `uint`. Any instance of a `int3` thus
            /// needs to be converted into the appropriate variant compatible with the `read` call.
            if first.hasPrefix("int3(") {
                let loadParameters = first[
                    first.index(first.startIndex, offsetBy: 5)..<first.index(first.endIndex, offsetBy: -1)
                ].split(separator: ",", maxSplits: 3, omittingEmptySubsequences: true)

                switch loadParameters.count {
                case 3:
                    loadCall = "read(uint2(\(loadParameters[0]), \(loadParameters[1])), uint(\(loadParameters[2])))"
                case 2:
                    loadCall = "read(uint2(\(loadParameters[0])), uint(\(loadParameters[1])))"
                case 1:
                    loadCall = "read(uint2(\(loadParameters[0]).xy), 0)"
                default:
                    throw ParserError.parseFail("int3 constructor with invalid number of arguments encountered")
                }
            } else {
                loadCall = "read(uint2(\(first).xy), 0)"
            }

            token = cfp.cur_token
            return loadCall
        }
    }

    /// Generates the explicit arguments that need to be passed into MSL shader functions in place of direct access to
    /// uniform globals which are not supported by Metal.
    /// - Parameter functionName: Name of the function to generate the additional arguments for
    /// - Returns: String of additional arguments to be put into the function signature
    private func generateAdditionalArguments(for functionName: String) -> String? {
        var output = [String]()

        for function in functions.values {
            if function.name != functionName {
                continue
            }

            if function.requiresUniformBuffers {
                output.append("uniforms")
            }

            for texture in function.textures {
                for uniform in uniforms.values {
                    if uniform.name == texture && uniform.storageType.contains(.typeTexture) {
                        output.append(texture)
                    }
                }
            }

            for sampler in function.samplers {
                for i in 0..<parser.samplers.num {
                    let samplerPointer: UnsafeMutablePointer<shader_sampler>? = parser.samplers.array.advanced(by: i)

                    if let samplerPointer {
                        if sampler == String(cString: samplerPointer.pointee.name) {
                            output.append(sampler)
                        }
                    }
                }
            }
        }

        if output.count > 0 {
            return output.joined(separator: ", ")
        }

        return nil
    }

    deinit {
        withUnsafeMutablePointer(to: &parser) {
            shader_parser_free($0)
        }
    }
}
