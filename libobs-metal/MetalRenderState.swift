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
import simd

/// The MetalRenderState struct emulates a state object like Direct3D's `ID3D11DeviceContext`, holding references to
/// elements of a render pipeline that would be considered the "current" variant of each.
///
/// Typical "current" state elements include (but are not limited to):
///
/// * Variant of the render target for linear color writes
/// * Variant of the render target for color writes with automatic sRGB gamma encoding
/// * View matrix and view projection matrix
/// * Vertex buffer and optional index buffer
/// * Depth stencil attachment
/// * Vertex shader
/// * Fragment shader
/// * View port size
/// * Cull mode
///
/// These references are swapped out by OBS for each "scene" and "scene items" within it before issuing draw calls,
/// thus actual pipelines need to be created "on demand" based on the pipeline descriptor and stored in a cache to
/// avoid the cost of pipeline validation on consecutive render passes.
struct MetalRenderState {
    var viewMatrix: matrix_float4x4
    var projectionMatrix: matrix_float4x4
    var viewProjectionMatrix: matrix_float4x4

    var renderTarget: MetalTexture?
    var sRGBrenderTarget: MetalTexture?
    var depthStencilAttachment: MetalTexture?
    var isRendertargetChanged = false

    var vertexBuffer: MetalVertexBuffer?
    var indexBuffer: MetalIndexBuffer?

    var vertexShader: MetalShader?
    var fragmentShader: MetalShader?

    var viewPort = MTLViewport()
    var cullMode = MTLCullMode.none

    var scissorRectEnabled: Bool
    var scissorRect: MTLScissorRect?

    var gsColorSpace: gs_color_space
    var useSRGBGamma = false

    var swapChain: OBSSwapChain?
    var isInDisplaysRenderStage = false

    var pipelineDescriptor = MTLRenderPipelineDescriptor()
    var clearPipelineDescriptor = MTLRenderPipelineDescriptor()
    var renderPassDescriptor = MTLRenderPassDescriptor()
    var depthStencilDescriptor = MTLDepthStencilDescriptor()
    var commandBuffer: MTLCommandBuffer?

    var textures = [MTLTexture?](repeating: nil, count: Int(GS_MAX_TEXTURES))
    var samplers = [MTLSamplerState?](repeating: nil, count: Int(GS_MAX_TEXTURES))

    var projections = [matrix_float4x4]()
    var inFlightRenderTargets = Set<MetalTexture>()
}
