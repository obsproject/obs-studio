#include <cinttypes>
#include <util/base.h>

#include "metal-subsystem.hpp"

using namespace std;

void gs_device::InitDevice(uint32_t deviceIdx)
{
	NSArray *devices;
	
	devIdx  = deviceIdx;
	devices = MTLCopyAllDevices();
	if (devices == nil)
		throw "Failed to create MTLDevice";

	for (size_t i = 0; i < devices.count; i++) {
		if (i == devIdx) {
			device = devices[i];
			break;
		}
	}

	if (device == nil)
		throw "Well I guess there's no device, try using OpenGL "
		      "rather than this awful apple API that they made "
		      "with the sole purpose of making developer's lives "
		      "difficult.";

	blog(LOG_INFO, "Loading up Metal on adapter %s (%" PRIu32 ")",
			device.name.UTF8String, deviceIdx);

	if ([device supportsFeatureSet:MTLFeatureSet_OSX_GPUFamily1_v2]) {
		featureSetFamily  = 1;
		featureSetVersion = 2;
	} else
		throw "Failed to initialize Metal";

	blog(LOG_INFO, "Metal loaded successfully, feature set used: %u_v%u",
			featureSetFamily, featureSetVersion);
}

void gs_device::SetClear()
{
	ClearState state = clearStates.top().second;
	
	if (state.flags & GS_CLEAR_COLOR) {
		MTLRenderPassColorAttachmentDescriptor *colorAttachment =
				passDesc.colorAttachments[0];
		colorAttachment.loadAction = MTLLoadActionClear;
		colorAttachment.clearColor = MTLClearColorMake(
				state.color.x, state.color.y, state.color.z,
				state.color.w);
	} else
		passDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
	
	if (state.flags & GS_CLEAR_DEPTH) {
		MTLRenderPassDepthAttachmentDescriptor *depthAttachment =
				passDesc.depthAttachment;
		depthAttachment.loadAction = MTLLoadActionClear;
		depthAttachment.clearDepth = state.depth;
	} else
		passDesc.depthAttachment.loadAction = MTLLoadActionLoad;
	
	if (state.flags & GS_CLEAR_STENCIL) {
		MTLRenderPassStencilAttachmentDescriptor *stencilAttachment =
				passDesc.stencilAttachment;
		stencilAttachment.loadAction   = MTLLoadActionClear;
		stencilAttachment.clearStencil = state.stencil;
	} else
		passDesc.stencilAttachment.loadAction = MTLLoadActionLoad;
	
	clearStates.pop();
	if (clearStates.size())
		preserveClearTarget = clearStates.top().first;
	else
		preserveClearTarget = nullptr;
}

void gs_device::UploadVertexBuffer(id<MTLRenderCommandEncoder> commandEncoder)
{
	vector<id<MTLBuffer>> buffers;
	vector<NSUInteger>    offsets;
	
	if (curVertexBuffer && curVertexShader) {
		curVertexBuffer->MakeBufferList(curVertexShader, buffers);
		if (curVertexBuffer->isDynamic)
			curVertexBuffer->Release();
	} else {
		size_t buffersToClear = curVertexShader ?
				curVertexShader->NumBuffersExpected() : 0;
		buffers.resize(buffersToClear);
	}
	
	offsets.resize(buffers.size());
	
	[commandEncoder setVertexBuffers:buffers.data()
			offsets:offsets.data()
			withRange:NSMakeRange(0, buffers.size())];
	
	lastVertexBuffer = curVertexBuffer;
	lastVertexShader = curVertexShader;
}

void gs_device::UploadTextures(id<MTLRenderCommandEncoder> commandEncoder)
{
	for (size_t i = 0; i < GS_MAX_TEXTURES; i++) {
		gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(
				curTextures[i]);
		if (tex2d == nullptr)
			break;

		[commandEncoder setFragmentTexture:tex2d->texture atIndex:i];
	}
}

void gs_device::UploadSamplers(id<MTLRenderCommandEncoder> commandEncoder)
{
	for (size_t i = 0; i < GS_MAX_TEXTURES; i++) {
		gs_sampler_state *sampler = curSamplers[i];
		if (sampler == nullptr)
			break;

		[commandEncoder setFragmentSamplerState:sampler->samplerState
				atIndex:i];
	}
}

void gs_device::LoadRasterState(id<MTLRenderCommandEncoder> commandEncoder)
{
	[commandEncoder setViewport:rasterState.mtlViewport];
	/* use CCW to convert to a right-handed coordinate system */
	[commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
	[commandEncoder setCullMode:rasterState.mtlCullMode];
	if (rasterState.scissorEnabled)
		[commandEncoder setScissorRect:rasterState.mtlScissorRect];
}

void gs_device::LoadZStencilState(id<MTLRenderCommandEncoder> commandEncoder)
{
	if (zstencilState.depthEnabled) {
		if (depthStencilState == nil) {
			depthStencilState = [device
					newDepthStencilStateWithDescriptor:
					zstencilState.dsd];
		}
		[commandEncoder setDepthStencilState:depthStencilState];
	}
}

void gs_device::UpdateViewProjMatrix()
{
	gs_matrix_get(&curViewMatrix);

	/* negate Z col of the view matrix for right-handed coordinate system */
	curViewMatrix.x.z = -curViewMatrix.x.z;
	curViewMatrix.y.z = -curViewMatrix.y.z;
	curViewMatrix.z.z = -curViewMatrix.z.z;
	curViewMatrix.t.z = -curViewMatrix.t.z;

	matrix4_mul(&curViewProjMatrix, &curViewMatrix, &curProjMatrix);
	matrix4_transpose(&curViewProjMatrix, &curViewProjMatrix);

	if (curVertexShader->viewProj)
		gs_shader_set_matrix4(curVertexShader->viewProj,
				&curViewProjMatrix);
}

void gs_device::DrawPrimitives(id<MTLRenderCommandEncoder> commandEncoder,
		gs_draw_mode drawMode, uint32_t startVert, uint32_t numVerts)
{
	MTLPrimitiveType primitive = ConvertGSTopology(drawMode);
	if (curIndexBuffer) {
		if (numVerts == 0)
			numVerts = static_cast<uint32_t>(curIndexBuffer->num);
		[commandEncoder drawIndexedPrimitives:primitive
				indexCount:numVerts
				indexType:curIndexBuffer->indexType
				indexBuffer:curIndexBuffer->indexBuffer
				indexBufferOffset:0];
		if (curIndexBuffer->isDynamic)
			curIndexBuffer->indexBuffer = nil;
	} else {
		if (numVerts == 0)
			numVerts = static_cast<uint32_t>(
					curVertexBuffer->vbData->num);
		[commandEncoder drawPrimitives:primitive
				vertexStart:startVert vertexCount:numVerts];
	}
}

void gs_device::Draw(gs_draw_mode drawMode, uint32_t startVert,
		uint32_t numVerts)
{
	try {
		if (!curVertexShader)
			throw "No vertex shader specified";
		
		if (!curPixelShader)
			throw "No pixel shader specified";
		
		if (!curVertexBuffer)
			throw "No vertex buffer specified";
		
		if (!curRenderTarget)
			throw "No render target to render to";
		
	} catch (const char *error) {
		blog(LOG_ERROR, "device_draw (Metal): %s", error);
		return;
	}

	if (pipelineState == nil || piplineStateChanged) {
		NSError *error = nil;
		pipelineState = [device newRenderPipelineStateWithDescriptor:
				pipelineDesc error:&error];
		
		if (pipelineState == nil) {
			blog(LOG_ERROR, "device_draw (Metal): %s",
					error.localizedDescription.UTF8String);
			return;
		}
		
		piplineStateChanged = false;
	}

	if (preserveClearTarget != curRenderTarget) {
		passDesc.colorAttachments[0].loadAction = MTLLoadActionLoad;
		passDesc.depthAttachment.loadAction     = MTLLoadActionLoad;
		passDesc.stencilAttachment.loadAction   = MTLLoadActionLoad;
	} else
		SetClear();

	id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer
			renderCommandEncoderWithDescriptor:passDesc];
	[commandEncoder setRenderPipelineState:pipelineState];

	try {
		gs_effect_t *effect = gs_get_effect();
		if (effect)
			gs_effect_update_params(effect);

		LoadRasterState(commandEncoder);
		LoadZStencilState(commandEncoder);
		UpdateViewProjMatrix();
		curVertexShader->UploadParams(commandEncoder);
		curPixelShader->UploadParams(commandEncoder);
		UploadVertexBuffer(commandEncoder);
		UploadTextures(commandEncoder);
		UploadSamplers(commandEncoder);
		DrawPrimitives(commandEncoder, drawMode, startVert, numVerts);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_draw (Metal): %s", error);
	}

	[commandEncoder endEncoding];
}

static inline id<MTLBuffer> CreateBuffer(id<MTLDevice> device,
		void *data, size_t length)
{
	length = (length + 15) & ~15;
	
	MTLResourceOptions options = MTLResourceCPUCacheModeWriteCombined |
			MTLResourceStorageModeShared;
	id<MTLBuffer> buffer = [device newBufferWithBytes:data
			length:length options:options];
	if (buffer == nil)
		throw "Failed to create buffer";
	return buffer;
}

id<MTLBuffer> gs_device::GetBuffer(void *data, size_t length)
{
	lock_guard<mutex> lock(mutexObj);
	auto target = find_if(unusedBufferPool.begin(), unusedBufferPool.end(),
		[length](id<MTLBuffer> b) { return b.length >= length; });
	if (target == unusedBufferPool.end()) {
		id<MTLBuffer> newBuffer = CreateBuffer(device, data, length);
		curBufferPool.push_back(newBuffer);
		return newBuffer;
	}
	
	id<MTLBuffer> targetBuffer = *target;
	unusedBufferPool.erase(target);
	curBufferPool.push_back(targetBuffer);
	memcpy(targetBuffer.contents, data, length);
	return targetBuffer;
}

void gs_device::PushResources()
{
	lock_guard<mutex> lock(mutexObj);
	bufferPools.push(curBufferPool);
	curBufferPool.clear();
}

void gs_device::ReleaseResources()
{
	lock_guard<mutex> lock(mutexObj);
	auto& pool = bufferPools.front();
	unusedBufferPool.insert(unusedBufferPool.end(),
			pool.begin(), pool.end());
	bufferPools.pop();
}

gs_device::gs_device(uint32_t adapterIdx)
{
	matrix4_identity(&curProjMatrix);
	matrix4_identity(&curViewMatrix);
	matrix4_identity(&curViewProjMatrix);

	passDesc = [[MTLRenderPassDescriptor alloc] init];
	pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];

	InitDevice(adapterIdx);

	commandQueue = [device newCommandQueue];

	device_set_render_target(this, nullptr, nullptr);
}
