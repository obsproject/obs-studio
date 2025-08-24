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

import AppKit
import Foundation
import Metal
import simd

/// Describes which clear actions to take when an explicit clear is requested
struct ClearState {
    var colorAction: MTLLoadAction = .dontCare
    var depthAction: MTLLoadAction = .dontCare
    var stencilAction: MTLLoadAction = .dontCare
    var clearColor: MTLClearColor = MTLClearColor()
    var clearDepth: Double = 0.0
    var clearStencil: UInt32 = 0
    var clearTarget: MetalTexture? = nil
}

/// Object wrapping an `MTLDevice` object and providing convenience functions for interaction with `libobs`
class MetalDevice {
    private let identityMatrix = matrix_float4x4.init(diagonal: SIMD4(1.0, 1.0, 1.0, 1.0))
    private let fallbackVertexBuffer: MTLBuffer
    private var nopVertexFunction: MTLFunction
    private var pipelines = [Int: MTLRenderPipelineState]()
    private var depthStencilStates = [Int: MTLDepthStencilState]()
    private var obsSignalCallbacks = [MetalSignalType: () -> Void]()
    private var displayLink: CVDisplayLink?

    let device: MTLDevice
    let commandQueue: MTLCommandQueue
    var renderState: MetalRenderState
    var swapChains = [OBSSwapChain]()
    let swapChainQueue = DispatchQueue(label: "swapchainUpdateQueue", qos: .userInteractive)

    init(device: MTLDevice) throws {
        self.device = device

        guard let commandQueue = device.makeCommandQueue() else {
            throw MetalError.MTLDeviceError.commandQueueCreationFailure
        }

        guard let buffer = device.makeBuffer(length: 1, options: .storageModePrivate) else {
            throw MetalError.MTLDeviceError.bufferCreationFailure("Fallback vertex buffer")
        }

        let nopVertexSource = "[[vertex]] float4 vsNop() { return (float4)0; }"

        let compileOptions = MTLCompileOptions()
        if #available(macOS 15, *) {
            compileOptions.mathMode = .fast
        } else {
            compileOptions.fastMathEnabled = true
        }

        guard let library = try? device.makeLibrary(source: nopVertexSource, options: compileOptions),
            let function = library.makeFunction(name: "vsNop")
        else {
            throw MetalError.MTLDeviceError.shaderCompilationFailure("Vertex NOP shader")
        }

        CVDisplayLinkCreateWithActiveCGDisplays(&displayLink)
        if displayLink == nil {
            throw MetalError.MTLDeviceError.displayLinkCreationFailure
        }

        self.commandQueue = commandQueue
        self.nopVertexFunction = function
        self.fallbackVertexBuffer = buffer

        self.renderState = MetalRenderState(
            viewMatrix: identityMatrix,
            projectionMatrix: identityMatrix,
            viewProjectionMatrix: identityMatrix,
            scissorRectEnabled: false,
            gsColorSpace: GS_CS_SRGB
        )

        let clearPipelineDescriptor = renderState.clearPipelineDescriptor
        clearPipelineDescriptor.colorAttachments[0].isBlendingEnabled = false
        clearPipelineDescriptor.vertexFunction = nopVertexFunction
        clearPipelineDescriptor.fragmentFunction = nil
        clearPipelineDescriptor.inputPrimitiveTopology = .point

        setupSignalHandlers()
        setupDisplayLink()
    }

    func dispatchSignal(type: MetalSignalType) {
        if let callback = obsSignalCallbacks[type] {
            callback()
        }
    }

    /// Creates signal handlers for specific OBS signals and adds them to a collection of signal handlers using the signal name as their key
    private func setupSignalHandlers() {
        let videoResetCallback = { [self] in
            guard let displayLink else { return }

            CVDisplayLinkStop(displayLink)
            CVDisplayLinkStart(displayLink)
        }

        obsSignalCallbacks.updateValue(videoResetCallback, forKey: MetalSignalType.videoReset)
    }

    /// Sets up the `CVDisplayLink` used by the ``MetalDevice`` to synchronize projector output with the operating
    /// system's screen refresh rate.
    private func setupDisplayLink() {
        func displayLinkCallback(
            displayLink: CVDisplayLink,
            _ now: UnsafePointer<CVTimeStamp>,
            _ outputTime: UnsafePointer<CVTimeStamp>,
            _ flagsIn: CVOptionFlags,
            _ flagsOut: UnsafeMutablePointer<CVOptionFlags>,
            _ displayLinkContext: UnsafeMutableRawPointer?
        ) -> CVReturn {
            guard let displayLinkContext else { return kCVReturnSuccess }

            let metalDevice = unsafeBitCast(displayLinkContext, to: MetalDevice.self)

            metalDevice.blitSwapChains()

            return kCVReturnSuccess
        }

        let opaqueSelf = UnsafeMutableRawPointer(Unmanaged.passUnretained(self).toOpaque())

        CVDisplayLinkSetOutputCallback(displayLink!, displayLinkCallback, opaqueSelf)
    }

    /// Iterates over all ``OBSSwapChain`` instances present on the ``MetalDevice`` instance and encodes a block
    /// transfer command on the GPU to copy the contents of the projector rendered by `libobs`'s render loop into the
    /// drawable provided by a `CAMetalLayer`.
    func blitSwapChains() {
        guard swapChains.count > 0 else { return }

        guard let commandBuffer = commandQueue.makeCommandBuffer(),
            let encoder = commandBuffer.makeBlitCommandEncoder()
        else {
            return
        }

        self.swapChainQueue.sync {
            swapChains = swapChains.filter { $0.discard == false }
        }

        for swapChain in swapChains {
            guard let renderTarget = swapChain.renderTarget, let drawable = swapChain.layer.nextDrawable() else {
                continue
            }

            guard renderTarget.texture.width == drawable.texture.width,
                renderTarget.texture.height == drawable.texture.height,
                renderTarget.texture.pixelFormat == drawable.texture.pixelFormat
            else {
                continue
            }

            autoreleasepool {
                encoder.waitForFence(swapChain.fence)
                encoder.copy(from: renderTarget.texture, to: drawable.texture)

                commandBuffer.addScheduledHandler { _ in
                    drawable.present()
                }
            }
        }

        encoder.endEncoding()
        commandBuffer.commit()
    }

    /// Simulates an explicit "clear" command commonly used in OpenGL or Direct3D11 implementations.
    /// - Parameter state: A ``ClearState`` object holding the requested clear actions
    ///
    /// Metal (like Direct3D12 and Vulkan) does not have an explicit clear command anymore. Devices with M- and
    /// A-series SOCs have deferred tile-based GPUs which do not load render targets as single large textures, but
    /// instead interact with textures via tiles. A load and store command is executed every time this occurs and a
    /// clear is achieved via a load command.
    ///
    /// If no actual rendering occurs however, no load or store commands are executed, and a render target will be
    /// "untouched". This would lead to issues in situations like switching to an empty scene, as the lack of any
    /// sources would trigger no draw calls.
    ///
    /// Thus an explicit draw call needs to be scheduled to achieve the same outcome as the explicit "clear" call in
    /// legacy APIs. This is achieved using the most lightweight pipeline possible:
    /// * A single vertex shader that returns 0 for all points
    /// * No fragment shader
    /// * Just load and store commands
    ///
    /// While this is indeed more inefficient than the "native" approach, it is the best way to ensure expected
    /// output with `libobs` rendering system.
    ///
    func clear(state: ClearState) throws {
        try ensureCommandBuffer()

        let commandBuffer = renderState.commandBuffer!

        guard let renderTarget = renderState.renderTarget else {
            return
        }

        let pipelineDescriptor = renderState.clearPipelineDescriptor

        if renderState.useSRGBGamma && renderTarget.sRGBtexture != nil {
            pipelineDescriptor.colorAttachments[0].pixelFormat = renderTarget.sRGBtexture!.pixelFormat
        } else {
            pipelineDescriptor.colorAttachments[0].pixelFormat = renderTarget.texture.pixelFormat
        }

        pipelineDescriptor.colorAttachments[0].isBlendingEnabled = false

        if let depthStencilAttachment = renderState.depthStencilAttachment {
            pipelineDescriptor.depthAttachmentPixelFormat = depthStencilAttachment.texture.pixelFormat
            pipelineDescriptor.stencilAttachmentPixelFormat = depthStencilAttachment.texture.pixelFormat
        } else {
            pipelineDescriptor.depthAttachmentPixelFormat = .invalid
            pipelineDescriptor.stencilAttachmentPixelFormat = .invalid
        }

        let stateHash = pipelineDescriptor.hashValue

        let renderPipelineState: MTLRenderPipelineState

        if let pipelineState = pipelines[stateHash] {
            renderPipelineState = pipelineState
        } else {
            do {
                let pipelineState = try device.makeRenderPipelineState(descriptor: pipelineDescriptor)
                pipelines.updateValue(pipelineState, forKey: stateHash)

                renderPipelineState = pipelineState
            } catch {
                throw MetalError.MTLDeviceError.pipelineStateCreationFailure
            }
        }

        let depthStencilDescriptor = MTLDepthStencilDescriptor()
        depthStencilDescriptor.isDepthWriteEnabled = false
        let depthStateHash = depthStencilDescriptor.hashValue

        let depthStencilState: MTLDepthStencilState

        if let state = depthStencilStates[depthStateHash] {
            depthStencilState = state
        } else {
            guard let state = device.makeDepthStencilState(descriptor: depthStencilDescriptor) else {
                throw MetalError.MTLDeviceError.depthStencilStateCreationFailure
            }

            depthStencilStates.updateValue(state, forKey: depthStateHash)

            depthStencilState = state
        }

        let renderPassDescriptor = MTLRenderPassDescriptor()

        if state.colorAction == .clear {
            renderPassDescriptor.colorAttachments[0].loadAction = .clear
            renderPassDescriptor.colorAttachments[0].storeAction = .store
            renderPassDescriptor.colorAttachments[0].clearColor = state.clearColor
        } else {
            renderPassDescriptor.colorAttachments[0].loadAction = state.colorAction
        }

        if state.depthAction == .clear {
            renderPassDescriptor.depthAttachment.loadAction = .clear
            renderPassDescriptor.depthAttachment.storeAction = .store
            renderPassDescriptor.depthAttachment.clearDepth = state.clearDepth
        } else {
            renderPassDescriptor.depthAttachment.loadAction = state.depthAction
        }

        if state.stencilAction == .clear {
            renderPassDescriptor.stencilAttachment.loadAction = .clear
            renderPassDescriptor.stencilAttachment.storeAction = .store
            renderPassDescriptor.stencilAttachment.clearStencil = state.clearStencil
        } else {
            renderPassDescriptor.stencilAttachment.loadAction = state.stencilAction
        }

        if renderState.useSRGBGamma && renderTarget.sRGBtexture != nil {
            renderPassDescriptor.colorAttachments[0].texture = renderTarget.sRGBtexture!
        } else {
            renderPassDescriptor.colorAttachments[0].texture = renderTarget.texture
        }

        renderTarget.hasPendingWrites = true
        renderState.inFlightRenderTargets.insert(renderTarget)

        renderPassDescriptor.colorAttachments[0].level = 0
        renderPassDescriptor.colorAttachments[0].slice = 0
        renderPassDescriptor.colorAttachments[0].depthPlane = 0

        if let zstencilAttachment = renderState.depthStencilAttachment {
            renderPassDescriptor.depthAttachment.texture = zstencilAttachment.texture
            renderPassDescriptor.stencilAttachment.texture = zstencilAttachment.texture
        } else {
            renderPassDescriptor.depthAttachment.texture = nil
            renderPassDescriptor.stencilAttachment.texture = nil
        }

        guard let encoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor) else {
            throw MetalError.MTLCommandBufferError.encoderCreationFailure
        }

        encoder.setRenderPipelineState(renderPipelineState)

        if renderState.depthStencilAttachment != nil {
            encoder.setDepthStencilState(depthStencilState)
        }

        encoder.setCullMode(.none)
        encoder.drawPrimitives(type: .point, vertexStart: 0, vertexCount: 1, instanceCount: 1, baseInstance: 0)
        encoder.endEncoding()
    }

    /// Schedules a draw call on the GPU with the information currently set up in the ``MetalRenderState`.`
    /// - Parameters:
    ///   - primitiveType: Type of primitives to render
    ///   - vertexStart: Start index for the vertices to be drawn
    ///   - vertexCount: Amount of vertices to be drawn
    ///
    /// Modern APIs like Metal have moved away from the "magic  state" mental model used by legacy APIs like OpenGL or
    /// Direct3D11 which required the APIs to validate the "global state" at every draw call. Instead Metal requires
    /// the creation of a pipeline object which is immutable after creation and thus has to run validation once and can
    /// then run draw calls directly.
    ///
    /// Due to the nature of OBS Studio, the pipeline state can change constantly, as blending, filtering, and
    /// conversion of data can constantly be changed by users of the program, which means that the combination of blend
    /// modes, shaders, and attachments can change constantly.
    ///
    /// To avoid a costly re-creation of pipelines for every draw call, pipelines are cached after creation and if a
    /// draw call uses an established pipeline, it will be reused from cache instead. While this cannot avoid the cost
    /// of creating new pipelines during runtime, it mitigates the cost for consecutive draw calls.
    func draw(primitiveType: MTLPrimitiveType, vertexStart: Int, vertexCount: Int) throws {
        try ensureCommandBuffer()

        let commandBuffer = renderState.commandBuffer!

        guard let renderTarget = renderState.renderTarget else {
            return
        }

        guard renderState.vertexBuffer != nil || vertexCount > 0 else {
            assertionFailure("MetalDevice: Attempted to render without a vertex buffer set")
            return
        }

        guard let vertexShader = renderState.vertexShader else {
            assertionFailure("MetalDevice: Attempted to render without vertex shader set")
            return
        }

        guard let fragmentShader = renderState.fragmentShader else {
            assertionFailure("MetalDevice: Attempted to render without fragment shader set")
            return
        }

        let renderPipelineDescriptor = renderState.pipelineDescriptor
        let renderPassDescriptor = renderState.renderPassDescriptor

        if renderState.isRendertargetChanged {
            if renderState.useSRGBGamma && renderTarget.sRGBtexture != nil {
                renderPipelineDescriptor.colorAttachments[0].pixelFormat = renderTarget.sRGBtexture!.pixelFormat
                renderPassDescriptor.colorAttachments[0].texture = renderTarget.sRGBtexture!
            } else {
                renderPipelineDescriptor.colorAttachments[0].pixelFormat = renderTarget.texture.pixelFormat
                renderPassDescriptor.colorAttachments[0].texture = renderTarget.texture
            }

            renderTarget.hasPendingWrites = true
            renderState.inFlightRenderTargets.insert(renderTarget)

            if let zstencilAttachment = renderState.depthStencilAttachment {
                renderPipelineDescriptor.depthAttachmentPixelFormat = zstencilAttachment.texture.pixelFormat
                renderPipelineDescriptor.stencilAttachmentPixelFormat = zstencilAttachment.texture.pixelFormat
                renderPassDescriptor.depthAttachment.texture = zstencilAttachment.texture
                renderPassDescriptor.stencilAttachment.texture = zstencilAttachment.texture
            } else {
                renderPipelineDescriptor.depthAttachmentPixelFormat = .invalid
                renderPipelineDescriptor.stencilAttachmentPixelFormat = .invalid
                renderPassDescriptor.depthAttachment.texture = nil
                renderPassDescriptor.stencilAttachment.texture = nil

            }
        }

        renderPassDescriptor.colorAttachments[0].loadAction = .load
        renderPassDescriptor.depthAttachment.loadAction = .load
        renderPassDescriptor.stencilAttachment.loadAction = .load

        let stateHash = renderState.pipelineDescriptor.hashValue

        let pipelineState: MTLRenderPipelineState

        if let state = pipelines[stateHash] {
            pipelineState = state
        } else {
            do {
                let state = try device.makeRenderPipelineState(descriptor: renderPipelineDescriptor)

                pipelines.updateValue(state, forKey: stateHash)
                pipelineState = state
            } catch {
                throw MetalError.MTLDeviceError.pipelineStateCreationFailure
            }
        }

        guard let commandEncoder = commandBuffer.makeRenderCommandEncoder(descriptor: renderPassDescriptor)
        else {
            throw MetalError.MTLCommandBufferError.encoderCreationFailure
        }

        commandEncoder.setRenderPipelineState(pipelineState)

        if let effect: OpaquePointer = gs_get_effect() {
            gs_effect_update_params(effect)
        }

        commandEncoder.setViewport(renderState.viewPort)
        commandEncoder.setFrontFacing(.counterClockwise)
        commandEncoder.setCullMode(renderState.cullMode)

        if let scissorRect = renderState.scissorRect, renderState.scissorRectEnabled {
            commandEncoder.setScissorRect(scissorRect)
        }

        let depthStateHash = renderState.depthStencilDescriptor.hashValue
        let depthStencilState: MTLDepthStencilState

        if let state = depthStencilStates[depthStateHash] {
            depthStencilState = state
        } else {
            guard let state = device.makeDepthStencilState(descriptor: renderState.depthStencilDescriptor) else {
                throw MetalError.MTLDeviceError.depthStencilStateCreationFailure
            }

            depthStencilStates.updateValue(state, forKey: depthStateHash)
            depthStencilState = state
        }

        commandEncoder.setDepthStencilState(depthStencilState)

        var gsViewMatrix: matrix4 = matrix4()
        gs_matrix_get(&gsViewMatrix)

        let viewMatrix = matrix_float4x4(
            rows: [
                SIMD4(gsViewMatrix.x.x, gsViewMatrix.x.y, gsViewMatrix.x.z, gsViewMatrix.x.w),
                SIMD4(gsViewMatrix.y.x, gsViewMatrix.y.y, gsViewMatrix.y.z, gsViewMatrix.y.w),
                SIMD4(gsViewMatrix.z.x, gsViewMatrix.z.y, gsViewMatrix.z.z, gsViewMatrix.z.w),
                SIMD4(gsViewMatrix.t.x, gsViewMatrix.t.y, gsViewMatrix.t.z, gsViewMatrix.t.w),
            ]
        )

        renderState.viewProjectionMatrix = (viewMatrix * renderState.projectionMatrix)

        if let viewProjectionUniform = vertexShader.viewProjection {
            viewProjectionUniform.setParameter(
                data: &renderState.viewProjectionMatrix, size: MemoryLayout<matrix_float4x4>.size)
        }

        vertexShader.uploadShaderParameters(encoder: commandEncoder)
        fragmentShader.uploadShaderParameters(encoder: commandEncoder)

        if let vertexBuffer = renderState.vertexBuffer {
            let buffers = vertexBuffer.getShaderBuffers(for: vertexShader)

            commandEncoder.setVertexBuffers(
                buffers,
                offsets: .init(repeating: 0, count: buffers.count),
                range: 0..<buffers.count)
        } else {
            commandEncoder.setVertexBuffer(fallbackVertexBuffer, offset: 0, index: 0)
        }

        for (index, texture) in renderState.textures.enumerated() {
            if let texture {
                commandEncoder.setFragmentTexture(texture, index: index)
            }
        }

        for (index, samplerState) in renderState.samplers.enumerated() {
            if let samplerState {
                commandEncoder.setFragmentSamplerState(samplerState, index: index)
            }
        }

        if let indexBuffer = renderState.indexBuffer,
            let bufferData = indexBuffer.indices
        {
            commandEncoder.drawIndexedPrimitives(
                type: primitiveType,
                indexCount: (vertexCount > 0) ? vertexCount : indexBuffer.count,
                indexType: indexBuffer.type,
                indexBuffer: bufferData,
                indexBufferOffset: 0
            )
        } else {
            if let vertexBuffer = renderState.vertexBuffer,
                let vertexData = vertexBuffer.vertexData
            {
                commandEncoder.drawPrimitives(
                    type: primitiveType,
                    vertexStart: vertexStart,
                    vertexCount: vertexData.pointee.num
                )
            } else {
                commandEncoder.drawPrimitives(
                    type: primitiveType,
                    vertexStart: vertexStart,
                    vertexCount: vertexCount
                )
            }
        }

        commandEncoder.endEncoding()
    }

    /// Creates a command buffer on the render state if none exists
    func ensureCommandBuffer() throws {
        if renderState.commandBuffer == nil {
            guard let buffer = commandQueue.makeCommandBuffer() else {
                throw MetalError.MTLCommandQueueError.commandBufferCreationFailure
            }

            renderState.commandBuffer = buffer
        }
    }

    /// Updates a memory fence used on the GPU to signal that the current render target (which is associated with a
    /// ``OBSSwapChain`` is available for  other GPU commands.
    ///
    /// This is necessary as the final output of projectors needs to be blitted into the drawables provided by the
    /// `CAMetalLayer` of each ``OBSSwapChain`` at the screen refresh interval, but projectors are usually rendered
    /// using tens of seperate little draw calls.
    ///
    /// Thus a virtual "display render stage" state is maintained by the Metal renderer, which is started when a
    /// ``OBSSwapChain`` instance is loaded by `libobs`  and ended when `device_end_scene` is called.
    func finishDisplayRenderStage() {
        let buffer = commandQueue.makeCommandBufferWithUnretainedReferences()
        let encoder = buffer?.makeBlitCommandEncoder()

        guard let buffer, let encoder, let swapChain = renderState.swapChain else {
            return
        }

        encoder.updateFence(swapChain.fence)
        encoder.endEncoding()
        buffer.commit()
    }

    /// Ensures that all encoded render commands in the current command buffer are committed to the command queue for
    /// execution on the GPU.
    ///
    /// This is particularly important when textures (or texture data) is to be blitted into other textures or buffers,
    /// as pending GPU commands in the existing buffer need to run before any commands that rely on the result of these
    /// draw commands to have taken place.
    ///
    /// Within the same queue this is ensured by Metal itself, but requires the commands to be encoded and committed
    /// in the desired order.
    func finishPendingCommands() {
        guard let commandBuffer = renderState.commandBuffer, commandBuffer.status != .committed else {
            return
        }

        commandBuffer.commit()

        renderState.inFlightRenderTargets.forEach {
            $0.hasPendingWrites = false
        }

        renderState.inFlightRenderTargets.removeAll(keepingCapacity: true)
        renderState.commandBuffer = nil
    }

    /// Copies the contents of a texture into another texture of identical dimensions
    /// - Parameters:
    ///   - source: Source texture to copy from
    ///   - destination: Destination texture to copy to
    ///
    /// This function requires both textures to have been created with the same dimensions, otherwise the copy
    /// operation will fail.
    ///
    /// If the source texture has pending writes (e.g., it was used as the render target for a clear or draw command),
    /// then the current command buffer  will be committed to ensure that the blit command encoded by this function
    /// happens after the pending commands.
    func copyTexture(source: MetalTexture, destination: MetalTexture) throws {
        if source.hasPendingWrites {
            finishPendingCommands()
        }

        try ensureCommandBuffer()

        let buffer = renderState.commandBuffer!
        let encoder = buffer.makeBlitCommandEncoder()

        guard let encoder else {
            throw MetalError.MTLCommandQueueError.commandBufferCreationFailure
        }

        encoder.copy(from: source.texture, to: destination.texture)
        encoder.endEncoding()
    }

    /// Copies the contents of a texture into a texture for CPU access
    /// - Parameters:
    ///   - source: Source texture to copy from
    ///   - destination: Destination texture to copy to
    ///
    /// This function requires both texture to have been created with the same dimensions, otherwise the copy operation
    /// will fail.
    ///
    /// If the source texture has pending writes (e.g., it was used as the render target for a clear or draw command),
    /// then the current command buffer will be comitted to ensure that the blit command encoded by this function
    /// happens after the pending commands.
    ///
    /// > Important: This function differs from ``copyTexture`` insofar as it will wait for the completion of all
    /// commands in the command queue to ensure that the GPU has actually completed the blit into the destination
    /// texture.
    func stageTexture(source: MetalTexture, destination: MetalTexture) throws {
        if source.hasPendingWrites {
            finishPendingCommands()
        }

        let buffer = commandQueue.makeCommandBufferWithUnretainedReferences()
        let encoder = buffer?.makeBlitCommandEncoder()

        guard let buffer, let encoder else {
            throw MetalError.MTLCommandQueueError.commandBufferCreationFailure
        }

        encoder.copy(from: source.texture, to: destination.texture)
        encoder.endEncoding()
        buffer.commit()
        buffer.waitUntilCompleted()
    }

    /// Copies the contents of a texture into a buffer for CPU access
    /// - Parameters:
    ///   - source: Source texture to copy from
    ///   - destination: Destination buffer to copy to
    ///
    /// This function requires that the destination buffer has been created with enough capacity to hold the source
    /// textures pixel data.
    ///
    /// If the source texture has pending writes (e.g., it was used as the render target for a clear or draw command),
    /// then the current command buffer will be comitted to ensure that the blit command encoded by this function
    /// happens after the pending commands.
    ///
    /// > Important: This function will wait for the completion of all commands in the command queue to ensure that the
    /// GPU has actually completed the blit into the destination buffer.
    ///
    func stageTextureToBuffer(source: MetalTexture, destination: MetalStageBuffer) throws {
        if source.hasPendingWrites {
            finishPendingCommands()
        }

        let buffer = commandQueue.makeCommandBufferWithUnretainedReferences()
        let encoder = buffer?.makeBlitCommandEncoder()

        guard let buffer, let encoder else {
            throw MetalError.MTLCommandQueueError.commandBufferCreationFailure
        }

        encoder.copy(
            from: source.texture,
            sourceSlice: 0,
            sourceLevel: 0,
            sourceOrigin: .init(x: 0, y: 0, z: 0),
            sourceSize: .init(width: source.texture.width, height: source.texture.height, depth: 1),
            to: destination.buffer,
            destinationOffset: 0,
            destinationBytesPerRow: destination.width * destination.format.bytesPerPixel!,
            destinationBytesPerImage: 0)

        encoder.endEncoding()
        buffer.commit()
        buffer.waitUntilCompleted()
    }

    /// Copies the contents of a buffer into a texture for GPU access
    /// - Parameters:
    ///   - source: Source buffer to copy from
    ///   - destination: Destination texture to copy to
    ///
    /// This function requires that the destination texture has been created with enough capacity to hold the source
    /// buffer pixel data.
    ///
    func stageBufferToTexture(source: MetalStageBuffer, destination: MetalTexture) throws {
        let buffer = commandQueue.makeCommandBufferWithUnretainedReferences()
        let encoder = buffer?.makeBlitCommandEncoder()

        guard let buffer, let encoder else {
            throw MetalError.MTLCommandQueueError.commandBufferCreationFailure
        }

        encoder.copy(
            from: source.buffer,
            sourceOffset: 0,
            sourceBytesPerRow: source.width * source.format.bytesPerPixel!,
            sourceBytesPerImage: 0,
            sourceSize: .init(width: source.width, height: source.height, depth: 1),
            to: destination.texture,
            destinationSlice: 0,
            destinationLevel: 0,
            destinationOrigin: .init(x: 0, y: 0, z: 0)
        )

        encoder.endEncoding()
        buffer.commit()
        buffer.waitUntilScheduled()
    }

    /// Copies a region from a source texture into a region of a destination texture
    /// - Parameters:
    ///   - source: Source texture to copy from
    ///   - sourceRegion: Region of the source texture to copy from
    ///   - destination: Destination texture to copy to
    ///   - destinationRegion: Destination region to copy into
    ///
    /// This function requires that the destination region fits within the dimensions of the destination texture,
    /// otherwise the copy operation will fail.
    ///
    /// If the source texture has pending writes (e.g., it was used as the render target for a clear or draw command),
    /// then the current command buffer will be comitted to ensure that the blit command encoded by this function
    /// happens after the pending commands.
    ///
    func copyTextureRegion(
        source: MetalTexture, sourceRegion: MTLRegion, destination: MetalTexture, destinationRegion: MTLRegion
    ) throws {
        if source.hasPendingWrites {
            finishPendingCommands()
        }

        let buffer = commandQueue.makeCommandBufferWithUnretainedReferences()
        let encoder = buffer?.makeBlitCommandEncoder()

        guard let buffer, let encoder else {
            throw MetalError.MTLCommandQueueError.commandBufferCreationFailure
        }

        encoder.copy(
            from: source.texture,
            sourceSlice: 0,
            sourceLevel: 0,
            sourceOrigin: sourceRegion.origin,
            sourceSize: sourceRegion.size,
            to: destination.texture,
            destinationSlice: 0,
            destinationLevel: 0,
            destinationOrigin: destinationRegion.origin
        )

        encoder.endEncoding()
        buffer.commit()
    }

    /// Stops the `CVDisplayLink` used by the ``MetalDevice`` instance
    func shutdown() {
        guard let displayLink else { return }

        CVDisplayLinkStop(displayLink)
        self.displayLink = nil
    }

    deinit {
        shutdown()
    }
}
