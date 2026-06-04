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

enum MetalError {
    enum MTLCommandQueueError: Error, CustomStringConvertible {
        case commandBufferCreationFailure

        var description: String {
            switch self {
            case .commandBufferCreationFailure:
                "MTLCommandQueue failed to create command buffer"
            }
        }
    }

    enum MTLDeviceError: Error, CustomStringConvertible {
        case commandQueueCreationFailure
        case displayLinkCreationFailure
        case bufferCreationFailure(String)
        case shaderCompilationFailure(String)
        case pipelineStateCreationFailure
        case depthStencilStateCreationFailure
        case samplerStateCreationFailure

        var description: String {
            switch self {
            case .commandQueueCreationFailure:
                "MTLDevice failed to create command queue"
            case .displayLinkCreationFailure:
                "MTLDevice failed to create CVDisplayLink for projector output"
            case .bufferCreationFailure(_):
                "MTLDevice failed to create buffer"
            case .shaderCompilationFailure(_):
                "MTLDevice failed to create shader library and function"
            case .pipelineStateCreationFailure:
                "MTLDevice failed to create render pipeline state"
            case .depthStencilStateCreationFailure:
                "MTLDevice failed to create depth stencil state"
            case .samplerStateCreationFailure:
                "MTLDevice failed to create sampler state with provided descriptor"
            }
        }
    }

    enum MTLCommandBufferError: Error, CustomStringConvertible {
        case encoderCreationFailure

        var description: String {
            switch self {
            case .encoderCreationFailure:
                "MTLCommandBuffer failed to create command encoder"
            }
        }
    }

    enum MetalShaderError: Error, CustomStringConvertible {
        case missingVertexDescriptor
        case missingSamplerDescriptors

        var description: String {
            switch self {
            case .missingVertexDescriptor:
                "MetalShader of type vertex requires a vertex descriptor"
            case .missingSamplerDescriptors:
                "MetalShader of type fragment requires at least a single sampler descriptor"
            }
        }
    }

    enum OBSShaderParserError: Error, CustomStringConvertible {
        case parseFail(String)
        case unsupportedType
        case missingNextToken
        case unexpectedToken
        case missingMainFunction

        var description: String {
            switch self {
            case .parseFail:
                "Failed to parse provided shader string"
            case .unsupportedType:
                "Provided GS type is not convertible to a Metal type"
            case .missingNextToken:
                "Required next token not found in parser token collection"
            case .unexpectedToken:
                "Required next token had unexpected type in parser token collection"
            case .missingMainFunction:
                "Shader has no main function"
            }
        }
    }

    enum OBSShaderError: Error, CustomStringConvertible {
        case unsupportedType
        case parseFail(String)
        case parseError(String)
        case transpileError(String)

        var description: String {
            switch self {
            case .unsupportedType:
                "Unsupported Metal shader type"
            case .parseFail(_):
                "OBS shader parser failed to parse effect"
            case .parseError(_):
                "OBS shader parser encountered warnings and/or errors while parsing effect"
            case .transpileError(_):
                "Transpiling OBS effects file into MSL shader failed"
            }
        }
    }
}
