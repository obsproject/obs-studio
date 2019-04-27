#include <cinttypes>
#include <util/base.h>
#include <graphics/matrix3.h>

#include "metal-subsystem.hpp"

using namespace std;

gs_obj::gs_obj(gs_device_t *device_, gs_type type) :
	device   (device_),
	obj_type (type)
{
	prev_next = &device->first_obj;
	next = device->first_obj;
	device->first_obj = this;
	if (next)
		next->prev_next = &next;
}

gs_obj::~gs_obj()
{
	if (prev_next)
		*prev_next = next;
	if (next)
		next->prev_next = prev_next;
}

const char *device_get_name(void)
{
	return "Metal";
}

int device_get_type(void)
{
	return GS_DEVICE_METAL;
}

const char *device_preprocessor_name(void)
{
	return "_Metal";
}

static inline void EnumMetalAdapters(
		bool (*callback)(void*, const char*, uint32_t),
		void *param)
{
	uint32_t i = 0;
	NSArray *devices = MTLCopyAllDevices();
	if (devices == nil)
		return;

	for (id<MTLDevice> device in devices) {
		if (!callback(param, [device name].UTF8String, i++))
			break;
	}
}

bool device_enum_adapters(
		bool (*callback)(void *param, const char *name, uint32_t id),
		void *param)
{
	EnumMetalAdapters(callback, param);
	return true;
}

static inline void CheckMetalSupport()
{
	if (NSProtocolFromString(@"MTLDevice") == nil)
		throw "This device doesn't support Metal.";
}

static inline void LogMetalAdapters()
{
	blog(LOG_INFO, "Available Video Adapters: ");

	NSArray *devices = MTLCopyAllDevices();
	if (devices == nil)
		return;

	for (size_t i = 0; i < devices.count; i++) {
		id<MTLDevice> device = devices[i];
		blog(LOG_INFO, "\tAdapter %zu: %s", i,
				[device name].UTF8String);
	}
}

int device_create(gs_device_t **p_device, uint32_t adapter)
{
	int errorcode = GS_SUCCESS;

	gs_device *device = nullptr;
	try {
		CheckMetalSupport();

		blog(LOG_INFO, "---------------------------------");
		blog(LOG_INFO, "Initializing Metal...");
		LogMetalAdapters();

		device = new gs_device(adapter);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_create (Metal): %s", error);
		errorcode = GS_ERROR_NOT_SUPPORTED;
	}

	*p_device = device;
	return errorcode;
}

void device_destroy(gs_device_t *device)
{
	delete device;
}

void device_enter_context(gs_device_t *device)
{
	/* does nothing */
	UNUSED_PARAMETER(device);
}

void device_leave_context(gs_device_t *device)
{
	/* does nothing */
	UNUSED_PARAMETER(device);
}

gs_swapchain_t *device_swapchain_create(gs_device_t *device,
		const struct gs_init_data *data)
{
	gs_swap_chain *swap = nullptr;
	try {
		swap = new gs_swap_chain(device, data);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_swapchain_create (Metal): %s", error);
	}

	return swap;
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	if (device->curSwapChain == nullptr) {
		blog(LOG_WARNING, "device_resize (Metal): No active swap");
		return;
	}

	try {
		id<MTLTexture> renderTarget = nil;
		id<MTLTexture> zstencilTarget = nil;

		device->passDesc.colorAttachments[0].texture = nil;
		device->passDesc.depthAttachment.texture     = nil;
		device->passDesc.stencilAttachment.texture   = nil;
		device->curSwapChain->Resize(cx, cy);

		if (device->curRenderTarget)
			renderTarget = device->curRenderTarget->texture;
		if (device->curZStencilBuffer)
			zstencilTarget  = device->curZStencilBuffer->texture;

		device->passDesc.colorAttachments[0].texture = renderTarget;
		device->passDesc.depthAttachment.texture     = zstencilTarget;
		device->passDesc.stencilAttachment.texture   = zstencilTarget;

	} catch (const char *error) {
		blog(LOG_ERROR, "device_resize (Metal): %s", error);
	}
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	if (device->curSwapChain) {
		CGSize curSize = device->curSwapChain->metalLayer.drawableSize;
		*cx = curSize.width;
		*cy = curSize.height;
	} else {
		blog(LOG_ERROR, "device_get_size (Metal): No active swap");
		*cx = 0;
		*cy = 0;
	}
}

uint32_t device_get_width(const gs_device_t *device)
{
	if (device->curSwapChain) {
		CGSize curSize = device->curSwapChain->metalLayer.drawableSize;
		return curSize.width;
	} else {
		blog(LOG_ERROR, "device_get_size (Metal): No active swap");
		return 0;
	}
}

uint32_t device_get_height(const gs_device_t *device)
{
	if (device->curSwapChain) {
		CGSize curSize = device->curSwapChain->metalLayer.drawableSize;
		return curSize.height;
	} else {
		blog(LOG_ERROR, "device_get_size (Metal): No active swap");
		return 0;
	}
}

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width,
		uint32_t height, enum gs_color_format color_format,
		uint32_t levels, const uint8_t **data, uint32_t flags)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, width, height, color_format,
				levels, data, flags, GS_TEXTURE_2D);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_texture_create (Metal): %s", error);
	}

	return texture;
}

gs_texture_t *device_cubetexture_create(gs_device_t *device, uint32_t size,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, size, size, color_format,
				levels, data, flags, GS_TEXTURE_CUBE);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_cubetexture_create (Metal): %s", error);
	}

	return texture;
}

gs_texture_t *device_voltexture_create(gs_device_t *device, uint32_t width,
		uint32_t height, uint32_t depth,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(depth);
	UNUSED_PARAMETER(color_format);
	UNUSED_PARAMETER(levels);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(flags);
	return NULL;
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width,
		uint32_t height, enum gs_zstencil_format format)
{
	gs_zstencil_buffer *zstencil = nullptr;
	try {
		zstencil = new gs_zstencil_buffer(device, width, height,
				format);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_zstencil_create (Metal): %s", error);
	}

	return zstencil;
}

gs_stagesurf_t *device_stagesurface_create(gs_device_t *device, uint32_t width,
		uint32_t height, enum gs_color_format color_format)
{
	gs_stage_surface *surf = nullptr;
	try {
		surf = new gs_stage_surface(device, width, height,
				color_format);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_stagesurface_create (Metal): %s",
				error);
	}

	return surf;
}

gs_samplerstate_t *device_samplerstate_create(gs_device_t *device,
		const struct gs_sampler_info *info)
{
	gs_sampler_state *ss = nullptr;
	try {
		ss = new gs_sampler_state(device, info);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_samplerstate_create (Metal): %s",
				error);
	}

	return ss;
}

gs_shader_t *device_vertexshader_create(gs_device_t *device,
		const char *shader_string, const char *file,
		char **error_string)
{
	gs_vertex_shader *shader = nullptr;
	try {
		shader = new gs_vertex_shader(device, file, shader_string);

	} catch (ShaderError error) {
		blog(LOG_ERROR, "device_vertexshader_create (Metal): "
		                "Compile warnings/errors for %s:\n%s",
		                file,
		                error.error.c_str());

	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexshader_create (Metal): %s",
				error);
	}

	return shader;

	UNUSED_PARAMETER(error_string);
}

gs_shader_t *device_pixelshader_create(gs_device_t *device,
		const char *shader_string, const char *file,
		char **error_string)
{
	gs_pixel_shader *shader = nullptr;
	try {
		shader = new gs_pixel_shader(device, file, shader_string);

	} catch (ShaderError error) {
		blog(LOG_ERROR, "device_pixelshader_create (Metal): "
		                "Compile warnings/errors for %s:\n%s",
		                file,
		                error.error.c_str());

	} catch (const char *error) {
		blog(LOG_ERROR, "device_pixelshader_create (Metal): %s", error);
	}

	return shader;

	UNUSED_PARAMETER(error_string);
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device,
		struct gs_vb_data *data, uint32_t flags)
{
	gs_vertex_buffer *buffer = nullptr;
	try {
		buffer = new gs_vertex_buffer(device, data, flags);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexbuffer_create (Metal): %s",
				error);
	}

	return buffer;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device,
		enum gs_index_type type, void *indices, size_t num,
		uint32_t flags)
{
	gs_index_buffer *buffer = nullptr;
	try {
		buffer = new gs_index_buffer(device, type, indices, num, flags);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_indexbuffer_create (Metal): %s", error);
	}

	return buffer;
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	return texture->type;
}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	if (device->curVertexBuffer == vertbuffer)
		return;

	device->curVertexBuffer = vertbuffer;
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	if (device->curIndexBuffer == indexbuffer)
		return;

	device->curIndexBuffer = indexbuffer;
}

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	if (device->curTextures[unit] == tex)
		return;

	device->curTextures[unit] = tex;
}

void device_load_samplerstate(gs_device_t *device,
		gs_samplerstate_t *samplerstate, int unit)
{
	if (device->curSamplers[unit] == samplerstate)
		return;

	device->curSamplers[unit] = samplerstate;
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	id<MTLFunction>     function  = nil;
	MTLVertexDescriptor *vertDesc = nil;

	if (device->curVertexShader == vertshader)
		return;

	gs_vertex_shader *vs = static_cast<gs_vertex_shader*>(vertshader);
	if (vertshader) {
		if (vertshader->type != GS_SHADER_VERTEX) {
			blog(LOG_ERROR, "device_load_vertexshader (Metal): "
			                "Specified shader is not a vertex "
			                "shader");
			return;
		}

		function = vs->function;
		vertDesc = vs->vertexDesc;
	}

	device->curVertexShader = vs;

	device->pipelineDesc.vertexFunction = function;
	device->pipelineDesc.vertexDescriptor = vertDesc;

	device->piplineStateChanged = true;
}

void device_clear_textures(gs_device_t *device)
{
	memset(device->curTextures, 0, sizeof(device->curTextures));
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	id<MTLFunction> function = nil;
	gs_sampler_state *states[GS_MAX_TEXTURES];

	if (device->curPixelShader == pixelshader)
		return;

	gs_pixel_shader *ps = static_cast<gs_pixel_shader*>(pixelshader);
	if (pixelshader) {
		if (pixelshader->type != GS_SHADER_PIXEL) {
			blog(LOG_ERROR, "device_load_pixelshader (Metal): "
			                "Specified shader is not a pixel "
			                "shader");
			return;
		}

		function  = ps->function;
		ps->GetSamplerStates(states);
	}

	device_clear_textures(device);

	device->curPixelShader = ps;
	for (size_t i = 0; i < GS_MAX_TEXTURES; i++)
		device->curSamplers[i] = states[i];

	device->pipelineDesc.fragmentFunction = function;

	device->piplineStateChanged = true;
}

void device_load_default_samplerstate(gs_device_t *device, bool b_3d, int unit)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(b_3d);
	UNUSED_PARAMETER(unit);
}

gs_shader_t *device_get_vertex_shader(const gs_device_t *device)
{
	return device->curVertexShader;
}

gs_shader_t *device_get_pixel_shader(const gs_device_t *device)
{
	return device->curPixelShader;
}

gs_texture_t *device_get_render_target(const gs_device_t *device)
{
	if (device->curSwapChain &&
	    device->curRenderTarget == device->curSwapChain->GetTarget())
		return nullptr;

	return device->curRenderTarget;
}

gs_zstencil_t *device_get_zstencil_target(const gs_device_t *device)
{
	return device->curZStencilBuffer;
}

void device_set_render_target(gs_device_t *device, gs_texture_t *tex,
		gs_zstencil_t *zstencil)
{
	if (device->curSwapChain) {
		if (!tex)
			tex = device->curSwapChain->GetTarget();
	}

	if (device->curRenderTarget   == tex &&
	    device->curZStencilBuffer == zstencil)
		return;

	if (tex && tex->type != GS_TEXTURE_2D) {
		blog(LOG_ERROR, "device_set_render_target (Metal): "
		                "texture is not a 2D texture");
		return;
	}

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	if (tex2d && tex2d->texture == nil) {
		blog(LOG_ERROR, "device_set_render_target (Metal): "
		                "texture is null");
		return;
	}

	device->curRenderTarget   = tex2d;
	device->curRenderSide     = 0;
	device->curZStencilBuffer = zstencil;

	if (tex2d) {
		device->passDesc.colorAttachments[0].texture = tex2d->texture;
		device->pipelineDesc.colorAttachments[0].pixelFormat =
				tex2d->mtlPixelFormat;
	} else
		device->passDesc.colorAttachments[0].texture = nil;

	if (zstencil) {
		device->passDesc.depthAttachment.texture   = zstencil->texture;
		device->passDesc.stencilAttachment.texture = zstencil->texture;
		device->pipelineDesc.depthAttachmentPixelFormat =
				zstencil->textureDesc.pixelFormat;
		device->pipelineDesc.stencilAttachmentPixelFormat =
				zstencil->textureDesc.pixelFormat;
	} else {
		device->passDesc.depthAttachment.texture   = nil;
		device->passDesc.stencilAttachment.texture = nil;
	}

	device->piplineStateChanged = true;
}

void device_set_cube_render_target(gs_device_t *device, gs_texture_t *tex,
		int side, gs_zstencil_t *zstencil)
{
	if (device->curSwapChain) {
		if (!tex)
			tex = device->curSwapChain->GetTarget();
	}

	if (device->curRenderTarget   == tex  &&
	    device->curRenderSide     == side &&
	    device->curZStencilBuffer == zstencil)
		return;

	if (tex->type != GS_TEXTURE_CUBE) {
		blog(LOG_ERROR, "device_set_cube_render_target (Metal): "
		                "texture is not a cube texture");
		return;
	}

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	if (tex2d && tex2d->texture == nil) {
		blog(LOG_ERROR, "device_set_cube_render_target (Metal): "
				"texture is null");
		return;
	}

	device->curRenderTarget   = tex2d;
	device->curRenderSide     = side;
	device->curZStencilBuffer = zstencil;

	if (tex2d) {
		device->passDesc.colorAttachments[0].texture = tex2d->texture;
		device->pipelineDesc.colorAttachments[0].pixelFormat =
				tex2d->mtlPixelFormat;
	} else
		device->passDesc.colorAttachments[0].texture = nil;

	if (zstencil) {
		device->passDesc.depthAttachment.texture   = zstencil->texture;
		device->passDesc.stencilAttachment.texture = zstencil->texture;
		device->pipelineDesc.depthAttachmentPixelFormat =
				zstencil->textureDesc.pixelFormat;
		device->pipelineDesc.stencilAttachmentPixelFormat =
				zstencil->textureDesc.pixelFormat;
	} else {
		device->passDesc.depthAttachment.texture   = nil;
		device->passDesc.stencilAttachment.texture = nil;
	}

	device->piplineStateChanged = true;
}

inline void gs_device::CopyTex(id<MTLTexture> dst,
		uint32_t dst_x, uint32_t dst_y,
		gs_texture_t *src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h)
{
	assert(commandBuffer != nil);

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(src);
	if (src_w == 0)
		src_w = tex2d->width;
	if (src_h == 0)
		src_h = tex2d->height;

	@autoreleasepool {
		id<MTLBlitCommandEncoder> commandEncoder =
				[commandBuffer blitCommandEncoder];
		MTLOrigin sourceOrigin      = MTLOriginMake(src_x, src_y, 0);
		MTLSize   sourceSize        = MTLSizeMake(src_w, src_h, 1);
		MTLOrigin destinationOrigin = MTLOriginMake(dst_x, dst_y, 0);
		[commandEncoder copyFromTexture:tex2d->texture
				sourceSlice:0
				sourceLevel:0
				sourceOrigin:sourceOrigin
				sourceSize:sourceSize
				toTexture:dst
				destinationSlice:0
				destinationLevel:0
				destinationOrigin:destinationOrigin];
		[commandEncoder endEncoding];
	}
}

void device_copy_texture_region(gs_device_t *device,
		gs_texture_t *dst, uint32_t dst_x, uint32_t dst_y,
		gs_texture_t *src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h)
{
	try {
		gs_texture_2d *src2d = static_cast<gs_texture_2d*>(src);
		gs_texture_2d *dst2d = static_cast<gs_texture_2d*>(dst);

		if (!src)
			throw "Source texture is null";
		if (!dst)
			throw "Destination texture is null";
		if (src->type != GS_TEXTURE_2D || dst->type != GS_TEXTURE_2D)
			throw "Source and destination textures must be a 2D "
			      "textures";
		if (dst->format != src->format)
			throw "Source and destination formats do not match";

		uint32_t copyWidth  = src_w ? src_w : (src2d->width - src_x);
		uint32_t copyHeight = src_h ? src_h : (src2d->height - src_y);

		uint32_t dstWidth  = dst2d->width  - dst_x;
		uint32_t dstHeight = dst2d->height - dst_y;

		if (dstWidth < copyWidth || dstHeight < copyHeight)
			throw "Destination texture region is not big "
			      "enough to hold the source region";

		if (dst_x == 0 && dst_y == 0 &&
		    src_x == 0 && src_y == 0 &&
		    src_w == 0 && src_h == 0) {
			copyWidth  = 0;
			copyHeight = 0;
		}

		device->CopyTex(dst2d->texture, dst_x, dst_y,
				src, src_x, src_y, copyWidth, copyHeight);

	} catch(const char *error) {
		blog(LOG_ERROR, "device_copy_texture (Metal): %s", error);
	}
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst,
		gs_texture_t *src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst,
		gs_texture_t *src)
{
	try {
		gs_texture_2d *src2d = static_cast<gs_texture_2d*>(src);

		if (!src)
			throw "Source texture is null";
		if (src->type != GS_TEXTURE_2D)
			throw "Source texture must be a 2D texture";
		if (!dst)
			throw "Destination surface is null";
		if (dst->format != src->format)
			throw "Source and destination formats do not match";
		if (dst->width  != src2d->width ||
		    dst->height != src2d->height)
			throw "Source and destination must have the same "
			      "dimensions";

		device->CopyTex(dst->texture, 0, 0, src, 0, 0, 0, 0);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_stage_texture (Metal): %s", error);
	}
}

void device_begin_scene(gs_device_t *device)
{
	device_clear_textures(device);

	device->commandBuffer = [device->commandQueue commandBuffer];
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode,
		uint32_t start_vert, uint32_t num_verts)
{
	/*
	 * Do not remove autorelease pool.
	 * Add MTLRenderCommandEncoder to autorelease pool.
	 */
	@autoreleasepool {
		device->Draw(draw_mode, start_vert, num_verts);
	}
}

void device_end_scene(gs_device_t *device)
{
	/* does nothing in Metal */
	UNUSED_PARAMETER(device);
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	if (device->curSwapChain == swapchain)
		return;

	if (swapchain) {
		device->curSwapChain      = swapchain;
		device->curRenderTarget   = swapchain->GetTarget();
		device->curRenderSide     = 0;
		device->curZStencilBuffer = nullptr;

		device->passDesc.colorAttachments[0].texture =
				device->curSwapChain->nextDrawable.texture;
		device->passDesc.depthAttachment.texture     = nil;
		device->passDesc.stencilAttachment.texture   = nil;

		device->pipelineDesc.colorAttachments[0].pixelFormat =
				device->curSwapChain->metalLayer.pixelFormat;
	} else {
		device->curSwapChain      = nullptr;
		device->curRenderTarget   = nullptr;
		device->curRenderSide     = 0;
		device->curZStencilBuffer = nullptr;

		device->passDesc.colorAttachments[0].texture = nil;
		device->passDesc.depthAttachment.texture     = nil;
		device->passDesc.stencilAttachment.texture   = nil;
	}

	device->piplineStateChanged = true;
}

void device_clear(gs_device_t *device, uint32_t clear_flags,
		const struct vec4 *color, float depth, uint8_t stencil)
{
	device->preserveClearTarget = device->curRenderTarget;

	ClearState state;
	state.flags   = clear_flags;
	state.color   = *color;
	state.depth   = depth;
	state.stencil = stencil;
	device->clearStates.emplace(device->curRenderTarget, state);
}

void device_present(gs_device_t *device)
{
	if (device->curSwapChain) {
		[device->commandBuffer presentDrawable:
				device->curSwapChain->nextDrawable];
	} else {
		blog(LOG_WARNING, "device_present (Metal): No active swap");
	}

	device->PushResources();

	[device->commandBuffer addCompletedHandler:^(id<MTLCommandBuffer> buf) {
		device->ReleaseResources();

		UNUSED_PARAMETER(buf);
	}];
	[device->commandBuffer commit];
	device->commandBuffer = nil;

	if (device->curSwapChain)
		device->curSwapChain->NextTarget();

}

void device_flush(gs_device_t *device)
{
	if (device->commandBuffer != nil) {
		device->PushResources();

		[device->commandBuffer commit];
		[device->commandBuffer waitUntilCompleted];
		device->commandBuffer = nil;

		device->ReleaseResources();

		if (device->curStageSurface) {
			device->curStageSurface->DownloadTexture();
			device->curStageSurface = nullptr;
		}
	}
}

void device_set_cull_mode(gs_device_t *device, enum gs_cull_mode mode)
{
	if (device->rasterState.cullMode == mode)
		return;

	device->rasterState.cullMode = mode;

	device->rasterState.mtlCullMode = ConvertGSCullMode(mode);
}

enum gs_cull_mode device_get_cull_mode(const gs_device_t *device)
{
	return device->rasterState.cullMode;
}

void device_enable_blending(gs_device_t *device, bool enable)
{
	if (device->blendState.blendEnabled == enable)
		return;

	device->blendState.blendEnabled = enable;

	device->pipelineDesc.colorAttachments[0].blendingEnabled =
			enable ? YES : NO;

	device->piplineStateChanged = true;
}

void device_enable_depth_test(gs_device_t *device, bool enable)
{
	if (device->zstencilState.depthEnabled == enable)
		return;

	device->zstencilState.depthEnabled = enable;

	device->depthStencilState = nil;
}

void device_enable_stencil_test(gs_device_t *device, bool enable)
{
	ZStencilState &state = device->zstencilState;

	if (state.stencilEnabled == enable)
		return;

	state.stencilEnabled = enable;

	state.dsd.frontFaceStencil.readMask = enable ? 1 : 0;
	state.dsd.backFaceStencil.readMask  = enable ? 1 : 0;

	device->depthStencilState = nil;
}

void device_enable_stencil_write(gs_device_t *device, bool enable)
{
	ZStencilState &state = device->zstencilState;

	if (state.stencilWriteEnabled == enable)
		return;

	state.stencilWriteEnabled = enable;

	state.dsd.frontFaceStencil.writeMask = enable ? 1 : 0;
	state.dsd.backFaceStencil.writeMask  = enable ? 1 : 0;

	device->depthStencilState = nil;
}

void device_enable_color(gs_device_t *device, bool red, bool green,
		bool blue, bool alpha)
{
	BlendState &state = device->blendState;

	if (state.redEnabled   == red   &&
	    state.greenEnabled == green &&
	    state.blueEnabled  == blue  &&
	    state.alphaEnabled == alpha)
		return;

	state.redEnabled   = red;
	state.greenEnabled = green;
	state.blueEnabled  = blue;
	state.alphaEnabled = alpha;

	MTLRenderPipelineColorAttachmentDescriptor *cad =
			device->pipelineDesc.colorAttachments[0];
	cad.writeMask = MTLColorWriteMaskNone;
	if (red)   cad.writeMask |= MTLColorWriteMaskRed;
	if (green) cad.writeMask |= MTLColorWriteMaskGreen;
	if (blue)  cad.writeMask |= MTLColorWriteMaskBlue;
	if (alpha) cad.writeMask |= MTLColorWriteMaskAlpha;

	device->piplineStateChanged = true;
}

void device_blend_function(gs_device_t *device, enum gs_blend_type src,
		enum gs_blend_type dest)
{
	BlendState &state = device->blendState;

	if (state.srcFactorC == src && state.destFactorC == dest &&
	    state.srcFactorA == src && state.destFactorA == dest)
		return;

	state.srcFactorC  = src;
	state.destFactorC = dest;
	state.srcFactorA  = src;
	state.destFactorA = dest;

	MTLRenderPipelineColorAttachmentDescriptor *cad =
			device->pipelineDesc.colorAttachments[0];
	cad.sourceRGBBlendFactor        = ConvertGSBlendType(src);
	cad.destinationRGBBlendFactor   = ConvertGSBlendType(dest);
	cad.sourceAlphaBlendFactor      = ConvertGSBlendType(src);
	cad.destinationAlphaBlendFactor = ConvertGSBlendType(dest);

	device->piplineStateChanged = true;
}

void device_blend_function_separate(gs_device_t *device,
		enum gs_blend_type src_c, enum gs_blend_type dest_c,
		enum gs_blend_type src_a, enum gs_blend_type dest_a)
{
	BlendState &state = device->blendState;

	if (state.srcFactorC == src_c && state.destFactorC == dest_c &&
	    state.srcFactorA == src_a && state.destFactorA == dest_a)
		return;

	state.srcFactorC  = src_c;
	state.destFactorC = dest_c;
	state.srcFactorA  = src_a;
	state.destFactorA = dest_a;

	MTLRenderPipelineColorAttachmentDescriptor *cad =
			device->pipelineDesc.colorAttachments[0];
	cad.sourceRGBBlendFactor        = ConvertGSBlendType(src_c);
	cad.destinationRGBBlendFactor   = ConvertGSBlendType(dest_c);
	cad.sourceAlphaBlendFactor      = ConvertGSBlendType(src_a);
	cad.destinationAlphaBlendFactor = ConvertGSBlendType(dest_a);

	device->piplineStateChanged = true;
}

void device_depth_function(gs_device_t *device, enum gs_depth_test test)
{
	if (device->zstencilState.depthFunc == test)
		return;

	device->zstencilState.depthFunc = test;

	device->zstencilState.dsd.depthCompareFunction =
			ConvertGSDepthTest(test);

	device->depthStencilState = nil;
}

static inline void update_stencilside_test(gs_device_t *device,
		StencilSide &side, MTLStencilDescriptor *desc,
		gs_depth_test test)
{
	if (side.test == test)
		return;

	side.test = test;

	desc.stencilCompareFunction = ConvertGSDepthTest(test);

	device->depthStencilState = nil;
}

void device_stencil_function(gs_device_t *device, enum gs_stencil_side side,
		enum gs_depth_test test)
{
	int sideVal = static_cast<int>(side);
	if (sideVal & GS_STENCIL_FRONT)
		update_stencilside_test(device,
				device->zstencilState.stencilFront,
				device->zstencilState.dsd.frontFaceStencil,
				test);
	if (sideVal & GS_STENCIL_BACK)
		update_stencilside_test(device,
				device->zstencilState.stencilBack,
				device->zstencilState.dsd.backFaceStencil,
				test);
}

static inline void update_stencilside_op(gs_device_t *device,
		StencilSide &side, MTLStencilDescriptor *desc,
		enum gs_stencil_op_type fail, enum gs_stencil_op_type zfail,
		enum gs_stencil_op_type zpass)
{
	if (side.fail == fail && side.zfail == zfail && side.zpass == zpass)
		return;

	side.fail  = fail;
	side.zfail = zfail;
	side.zpass = zpass;

	desc.stencilFailureOperation   = ConvertGSStencilOp(fail);
	desc.depthFailureOperation     = ConvertGSStencilOp(zfail);
	desc.depthStencilPassOperation = ConvertGSStencilOp(zpass);

	device->depthStencilState = nil;
}

void device_stencil_op(gs_device_t *device, enum gs_stencil_side side,
		enum gs_stencil_op_type fail, enum gs_stencil_op_type zfail,
		enum gs_stencil_op_type zpass)
{
	int sideVal = static_cast<int>(side);

	if (sideVal & GS_STENCIL_FRONT)
		update_stencilside_op(device,
				device->zstencilState.stencilFront,
				device->zstencilState.dsd.frontFaceStencil,
				fail, zfail, zpass);
	if (sideVal & GS_STENCIL_BACK)
		update_stencilside_op(device,
				device->zstencilState.stencilBack,
				device->zstencilState.dsd.backFaceStencil,
				fail, zfail, zpass);
}

void device_set_viewport(gs_device_t *device, int x, int y, int width,
		int height)
{
	RasterState &state = device->rasterState;

	if (state.viewport.x == x &&
	    state.viewport.y == y &&
	    state.viewport.cx == width &&
	    state.viewport.cy == height)
		return;

	state.viewport.x  = x;
	state.viewport.y  = y;
	state.viewport.cx = width;
	state.viewport.cy = height;

	state.mtlViewport = ConvertGSRectToMTLViewport(state.viewport);
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect)
{
	memcpy(rect, &device->rasterState.viewport, sizeof(gs_rect));
}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	if (rect != nullptr) {
		device->rasterState.scissorEnabled = true;
		device->rasterState.scissorRect    = *rect;
		device->rasterState.mtlScissorRect =
				ConvertGSRectToMTLScissorRect(*rect);
	} else {
		device->rasterState.scissorEnabled = false;
	}
}

void device_ortho(gs_device_t *device, float left, float right, float top,
		float bottom, float zNear, float zFar)
{
	matrix4 &dst = device->curProjMatrix;

	float rml = right - left;
	float bmt = bottom - top;
	float fmn = zFar - zNear;

	vec4_zero(&dst.x);
	vec4_zero(&dst.y);
	vec4_zero(&dst.z);
	vec4_zero(&dst.t);

	dst.x.x =           2.0f /  rml;
	dst.t.x = (left + right) / -rml;

	dst.y.y =           2.0f / -bmt;
	dst.t.y = (bottom + top) /  bmt;

	dst.z.z =           1.0f /  fmn;
	dst.t.z =          zNear / -fmn;

	dst.t.w = 1.0f;
}

void device_frustum(gs_device_t *device, float left, float right, float top,
		float bottom, float zNear, float zFar)
{
	matrix4 &dst = device->curProjMatrix;

	float rml    = right - left;
	float bmt    = bottom - top;
	float fmn    = zFar - zNear;
	float nearx2 = 2.0f * zNear;

	vec4_zero(&dst.x);
	vec4_zero(&dst.y);
	vec4_zero(&dst.z);
	vec4_zero(&dst.t);

	dst.x.x =         nearx2 /  rml;
	dst.z.x = (left + right) / -rml;

	dst.y.y =         nearx2 / -bmt;
	dst.z.y = (bottom + top) /  bmt;

	dst.z.z =           zFar /  fmn;
	dst.t.z = (zNear * zFar) / -fmn;

	dst.z.w = 1.0f;
}

void device_projection_push(gs_device_t *device)
{
	device->projStack.push(device->curProjMatrix);
}

void device_projection_pop(gs_device_t *device)
{
	if (!device->projStack.size())
		return;

	device->curProjMatrix = device->projStack.top();
	device->projStack.pop();
}

void device_debug_marker_begin(gs_device_t *, const char *, const float[4])
{
}

void device_debug_marker_end(gs_device_t *)
{
}

void gs_swapchain_destroy(gs_swapchain_t *swapchain)
{
	assert(swapchain->obj_type == gs_type::gs_swap_chain);

	if (swapchain->device->curSwapChain == swapchain)
		device_load_swapchain(swapchain->device, nullptr);

	delete swapchain;
}

void gs_texture_destroy(gs_texture_t *tex)
{
	delete tex;
}

uint32_t gs_texture_get_width(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return 0;

	return static_cast<const gs_texture_2d*>(tex)->width;
}

uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return 0;

	return static_cast<const gs_texture_2d*>(tex)->height;
}

enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return GS_UNKNOWN;

	return static_cast<const gs_texture_2d*>(tex)->format;
}

bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr, uint32_t *linesize)
{
	if (tex->type != GS_TEXTURE_2D)
		return false;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	uint32_t texSizeBytes = tex2d->height * tex2d->bytePerRow;

	tex2d->data.resize(1);
	tex2d->data[0].resize(texSizeBytes);

	*ptr      = (uint8_t *)tex2d->data[0].data();
	*linesize = tex2d->bytePerRow;
	return true;
}

void gs_texture_unmap(gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	tex2d->UploadTexture();
}

void *gs_texture_get_obj(gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return nullptr;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	return (__bridge void*)tex2d->texture;
}


void gs_cubetexture_destroy(gs_texture_t *cubetex)
{
	delete cubetex;
}

uint32_t gs_cubetexture_get_size(const gs_texture_t *cubetex)
{
	if (cubetex->type != GS_TEXTURE_CUBE)
		return 0;

	const gs_texture_2d *tex = static_cast<const gs_texture_2d*>(cubetex);
	return tex->width;
}

enum gs_color_format gs_cubetexture_get_color_format(
		const gs_texture_t *cubetex)
{
	if (cubetex->type != GS_TEXTURE_CUBE)
		return GS_UNKNOWN;

	const gs_texture_2d *tex = static_cast<const gs_texture_2d*>(cubetex);
	return tex->format;
}


void gs_voltexture_destroy(gs_texture_t *voltex)
{
	delete voltex;
}

uint32_t gs_voltexture_get_width(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_height(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

uint32_t gs_voltexture_get_depth(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return 0;
}

enum gs_color_format gs_voltexture_get_color_format(const gs_texture_t *voltex)
{
	/* TODO */
	UNUSED_PARAMETER(voltex);
	return GS_UNKNOWN;
}


void gs_stagesurface_destroy(gs_stagesurf_t *stagesurf)
{
	assert(stagesurf->obj_type == gs_type::gs_stage_surface);

	delete stagesurf;
}

uint32_t gs_stagesurface_get_width(const gs_stagesurf_t *stagesurf)
{
	assert(stagesurf->obj_type == gs_type::gs_stage_surface);

	return stagesurf->width;
}

uint32_t gs_stagesurface_get_height(const gs_stagesurf_t *stagesurf)
{
	assert(stagesurf->obj_type == gs_type::gs_stage_surface);

	return stagesurf->height;
}

enum gs_color_format gs_stagesurface_get_color_format(
		const gs_stagesurf_t *stagesurf)
{
	assert(stagesurf->obj_type == gs_type::gs_stage_surface);

	return stagesurf->format;
}

bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data,
		uint32_t *linesize)
{
	assert(stagesurf->obj_type == gs_type::gs_stage_surface);
	assert(stagesurf->device->commandBuffer != nil);

	@autoreleasepool {
		id<MTLBlitCommandEncoder> commandEncoder =
				[stagesurf->device->commandBuffer
				blitCommandEncoder];
		[commandEncoder synchronizeTexture:stagesurf->texture
				slice:0 level:0];
		[commandEncoder endEncoding];
	}

	*data     = (uint8_t *)stagesurf->data.data();
	*linesize = stagesurf->bytePerRow;

	stagesurf->device->curStageSurface = stagesurf;
	return true;
}

void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf)
{
	/* does nothing in Metal */
	UNUSED_PARAMETER(stagesurf);
}


void gs_zstencil_destroy(gs_zstencil_t *zstencil)
{
	assert(zstencil->obj_type == gs_type::gs_zstencil_buffer);

	delete zstencil;
}


void gs_samplerstate_destroy(gs_samplerstate_t *samplerstate)
{
	assert(samplerstate->obj_type == gs_type::gs_sampler_state);

	if (samplerstate->device) {
		for (size_t i = 0; i < GS_MAX_TEXTURES; i++)
			if (samplerstate->device->curSamplers[i] ==
			    samplerstate)
				samplerstate->device->curSamplers[i] = nullptr;
	}

	delete samplerstate;
}


void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer)
{
	assert(vertbuffer->obj_type == gs_type::gs_vertex_buffer);

	if (vertbuffer->device->lastVertexBuffer == vertbuffer)
		vertbuffer->device->lastVertexBuffer = nullptr;

	delete vertbuffer;
}

static inline void gs_vertexbuffer_flush_internal(gs_vertbuffer_t *vertbuffer,
		const gs_vb_data *data)
{
	assert(vertbuffer->obj_type == gs_type::gs_vertex_buffer);

	if (!vertbuffer->isDynamic) {
		blog(LOG_ERROR, "gs_vertexbuffer_flush: vertex buffer is not "
		                "dynamic");
		return;
	}

	vertbuffer->PrepareBuffers(data);
}

void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer)
{
	gs_vertexbuffer_flush_internal(vertbuffer, vertbuffer->vbData.get());
}

void gs_vertexbuffer_flush_direct(gs_vertbuffer_t *vertbuffer,
		const gs_vb_data *data)
{
	gs_vertexbuffer_flush_internal(vertbuffer, data);
}

struct gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer)
{
	assert(vertbuffer->obj_type == gs_type::gs_vertex_buffer);

	return vertbuffer->vbData.get();
}


void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer)
{
	assert(indexbuffer->obj_type == gs_type::gs_index_buffer);

	delete indexbuffer;
}

static inline void gs_indexbuffer_flush_internal(gs_indexbuffer_t *indexbuffer,
		void *indices)
{
	assert(indexbuffer->obj_type == gs_type::gs_index_buffer);

	if (!indexbuffer->isDynamic) {
		blog(LOG_ERROR, "gs_indexbuffer_flush: index buffer is not "
		                "dynamic");
		return;
	}

	indexbuffer->PrepareBuffer(indices);
}

void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer)
{
	gs_indexbuffer_flush_internal(indexbuffer,
			(void *)indexbuffer->indices.get());
}

void gs_indexbuffer_flush_direct(gs_indexbuffer_t *indexbuffer,
		const void *data)
{
	gs_indexbuffer_flush_internal(indexbuffer, (void *)data);
}

void *gs_indexbuffer_get_data(const gs_indexbuffer_t *indexbuffer)
{
	assert(indexbuffer->obj_type == gs_type::gs_index_buffer);

	return indexbuffer->indices.get();
}

size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *indexbuffer)
{
	assert(indexbuffer->obj_type == gs_type::gs_index_buffer);

	return indexbuffer->num;
}

enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *indexbuffer)
{
	assert(indexbuffer->obj_type == gs_type::gs_index_buffer);

	return indexbuffer->type;
}
