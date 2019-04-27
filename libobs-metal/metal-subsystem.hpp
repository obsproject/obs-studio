#pragma once

#include <util/AlignedNew.hpp>

#include <vector>
#include <stack>
#include <queue>
#include <string>
#include <mutex>
#include <memory>

#include <util/base.h>
#include <graphics/matrix4.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>

#ifdef __OBJC__
#import <AppKit/NSView.h>
#import <QuartzCore/CoreAnimation.h>
#import <Metal/Metal.h>

struct shader_var;
struct gs_vertex_shader;


static inline MTLPixelFormat ConvertGSTextureFormat(gs_color_format format)
{
	switch (format) {
	case GS_UNKNOWN:     return MTLPixelFormatInvalid;
	case GS_A8:          return MTLPixelFormatA8Unorm;
	case GS_R8:          return MTLPixelFormatR8Unorm;
	case GS_RGBA:        return MTLPixelFormatRGBA8Unorm;
	case GS_BGRX:        return MTLPixelFormatBGRA8Unorm;
	case GS_BGRA:        return MTLPixelFormatBGRA8Unorm;
	case GS_R10G10B10A2: return MTLPixelFormatRGB10A2Unorm;
	case GS_RGBA16:      return MTLPixelFormatRGBA16Unorm;
	case GS_R16:         return MTLPixelFormatR16Unorm;
	case GS_RGBA16F:     return MTLPixelFormatRGBA16Float;
	case GS_RGBA32F:     return MTLPixelFormatRGBA32Float;
	case GS_RG16F:       return MTLPixelFormatRG16Float;
	case GS_RG32F:       return MTLPixelFormatRG32Float;
	case GS_R16F:        return MTLPixelFormatR16Float;
	case GS_R32F:        return MTLPixelFormatR32Float;
	case GS_DXT1:        return MTLPixelFormatBC1_RGBA;
	case GS_DXT3:        return MTLPixelFormatBC2_RGBA;
	case GS_DXT5:        return MTLPixelFormatBC3_RGBA;
	}

	return MTLPixelFormatInvalid;
}

static inline gs_color_format ConvertMTLTextureFormat(MTLPixelFormat format)
{
	switch ((NSUInteger)format) {
	case MTLPixelFormatA8Unorm:       return GS_A8;
	case MTLPixelFormatR8Unorm:       return GS_R8;
	case MTLPixelFormatRGBA8Unorm:    return GS_RGBA;
	case MTLPixelFormatBGRA8Unorm:    return GS_BGRA;
	case MTLPixelFormatRGB10A2Unorm:  return GS_R10G10B10A2;
	case MTLPixelFormatRGBA16Unorm:   return GS_RGBA16;
	case MTLPixelFormatR16Unorm:      return GS_R16;
	case MTLPixelFormatRGBA16Float:   return GS_RGBA16F;
	case MTLPixelFormatRGBA32Float:   return GS_RGBA32F;
	case MTLPixelFormatRG16Float:     return GS_RG16F;
	case MTLPixelFormatRG32Float:     return GS_RG32F;
	case MTLPixelFormatR16Float:      return GS_R16F;
	case MTLPixelFormatR32Float:      return GS_R32F;
	case MTLPixelFormatBC1_RGBA:      return GS_DXT1;
	case MTLPixelFormatBC2_RGBA:      return GS_DXT3;
	case MTLPixelFormatBC3_RGBA:      return GS_DXT5;
	}

	return GS_UNKNOWN;
}

static inline gs_color_format ConvertOSTypePixelFormat(OSType format)
{
	if (format == 'BGRA') return GS_BGRA;
	if (format == 'w30r') return GS_R10G10B10A2;
	return GS_UNKNOWN;
}

static inline MTLCompareFunction ConvertGSDepthTest(gs_depth_test test)
{
	switch (test) {
	case GS_NEVER:    return MTLCompareFunctionNever;
	case GS_LESS:     return MTLCompareFunctionLess;
	case GS_LEQUAL:   return MTLCompareFunctionLessEqual;
	case GS_EQUAL:    return MTLCompareFunctionEqual;
	case GS_GEQUAL:   return MTLCompareFunctionGreaterEqual;
	case GS_GREATER:  return MTLCompareFunctionGreater;
	case GS_NOTEQUAL: return MTLCompareFunctionNotEqual;
	case GS_ALWAYS:   return MTLCompareFunctionAlways;
	}

	return MTLCompareFunctionNever;
}

static inline MTLStencilOperation ConvertGSStencilOp(gs_stencil_op_type op)
{
	switch (op) {
	case GS_KEEP:    return MTLStencilOperationKeep;
	case GS_ZERO:    return MTLStencilOperationZero;
	case GS_REPLACE: return MTLStencilOperationReplace;
	case GS_INCR:    return MTLStencilOperationIncrementWrap;
	case GS_DECR:    return MTLStencilOperationDecrementWrap;
	case GS_INVERT:  return MTLStencilOperationInvert;
	}

	return MTLStencilOperationKeep;
}

static inline MTLBlendFactor ConvertGSBlendType(gs_blend_type type)
{
	switch (type) {
	case GS_BLEND_ZERO:
		return MTLBlendFactorZero;
	case GS_BLEND_ONE:
		return MTLBlendFactorOne;
	case GS_BLEND_SRCCOLOR:
		return MTLBlendFactorSourceColor;
	case GS_BLEND_INVSRCCOLOR:
		return MTLBlendFactorOneMinusSourceColor;
	case GS_BLEND_SRCALPHA:
		return MTLBlendFactorSourceAlpha;
	case GS_BLEND_INVSRCALPHA:
		return MTLBlendFactorOneMinusSourceAlpha;
	case GS_BLEND_DSTCOLOR:
		return MTLBlendFactorDestinationColor;
	case GS_BLEND_INVDSTCOLOR:
		return MTLBlendFactorOneMinusDestinationColor;
	case GS_BLEND_DSTALPHA:
		return MTLBlendFactorDestinationAlpha;
	case GS_BLEND_INVDSTALPHA:
		return MTLBlendFactorOneMinusDestinationAlpha;
	case GS_BLEND_SRCALPHASAT:
		return MTLBlendFactorSourceAlphaSaturated;
	}

	return MTLBlendFactorOne;
}

static inline MTLCullMode ConvertGSCullMode(gs_cull_mode mode)
{
	switch (mode) {
	case GS_BACK:    return MTLCullModeBack;
	case GS_FRONT:   return MTLCullModeFront;
	case GS_NEITHER: return MTLCullModeNone;
	}

	return MTLCullModeBack;
}

static inline MTLPrimitiveType ConvertGSTopology(gs_draw_mode mode)
{
	switch (mode) {
	case GS_POINTS:    return MTLPrimitiveTypePoint;
	case GS_LINES:     return MTLPrimitiveTypeLine;
	case GS_LINESTRIP: return MTLPrimitiveTypeLineStrip;
	case GS_TRIS:      return MTLPrimitiveTypeTriangle;
	case GS_TRISTRIP:  return MTLPrimitiveTypeTriangleStrip;
	}

	return MTLPrimitiveTypePoint;
}

static inline MTLViewport ConvertGSRectToMTLViewport(gs_rect rect)
{
	MTLViewport ret;
	ret.originX = rect.x;
	ret.originY = rect.y;
	ret.width   = rect.cx;
	ret.height  = rect.cy;
	ret.znear   = 0.0;
	ret.zfar    = 1.0;
	return ret;
}

static inline MTLScissorRect ConvertGSRectToMTLScissorRect(gs_rect rect)
{
	MTLScissorRect ret;
	ret.x      = rect.x;
	ret.y      = rect.y;
	ret.width  = rect.cx;
	ret.height = rect.cy;
	return ret;
}

enum class gs_type {
	gs_vertex_buffer,
	gs_index_buffer,
	gs_texture_2d,
	gs_zstencil_buffer,
	gs_stage_surface,
	gs_sampler_state,
	gs_vertex_shader,
	gs_pixel_shader,
	gs_swap_chain,
};

struct gs_obj {
	gs_device_t *device;
	gs_type obj_type;
	gs_obj *next;
	gs_obj **prev_next;

	inline gs_obj() :
		device    (nullptr),
		next      (nullptr),
		prev_next (nullptr)
	{
	}

	gs_obj(gs_device_t *device, gs_type type);
	virtual ~gs_obj();
};

struct gs_vertex_buffer : gs_obj {
	const bool                 isDynamic;
	const std::unique_ptr<gs_vb_data, decltype(&gs_vbdata_destroy)> vbData;

	id<MTLBuffer>              vertexBuffer;
	id<MTLBuffer>              normalBuffer;
	id<MTLBuffer>              colorBuffer;
	id<MTLBuffer>              tangentBuffer;
	std::vector<id<MTLBuffer>> uvBuffers;

	inline id<MTLBuffer> PrepareBuffer(void *array, size_t elementSize,
			NSString *name);
	void PrepareBuffers(const gs_vb_data *data);

	void MakeBufferList(gs_vertex_shader *shader,
			std::vector<id<MTLBuffer>> &buffers);

	inline id<MTLBuffer> InitBuffer(size_t elementSize, void *array,
			const char *name);
	void InitBuffers();

	inline void Release()
	{
		vertexBuffer = nil;
		normalBuffer = nil;
		colorBuffer  = nil;
		tangentBuffer = nil;
		uvBuffers.clear();
	}
	void Rebuild();

	gs_vertex_buffer(gs_device_t *device, struct gs_vb_data *data,
			uint32_t flags);
};

struct gs_index_buffer : gs_obj {
	const gs_index_type type;
	const bool          isDynamic;
	const std::unique_ptr<void, decltype(&bfree)> indices;
	const size_t        num = 0, len = 0;
	const MTLIndexType  indexType;

	id<MTLBuffer>       indexBuffer;

	void PrepareBuffer(void *new_indices);
	void InitBuffer();

	inline void Release() {indexBuffer = nil;}
	void Rebuild();

	gs_index_buffer(gs_device_t *device, enum gs_index_type type,
			void *indices, size_t num, uint32_t flags);
};

struct gs_texture : gs_obj {
	const gs_texture_type type   = GS_TEXTURE_2D;
	const uint32_t        levels;
	const gs_color_format format = GS_UNKNOWN;

	inline gs_texture(gs_device *device, gs_type obj_type,
			gs_texture_type type,
			uint32_t levels, gs_color_format format)
		: gs_obj (device, obj_type),
		  type   (type),
		  levels (levels),
		  format (format)
	{
	}
};

struct gs_texture_2d : gs_texture {
	const uint32_t       width = 0, height = 0, bytePerRow = 0;
	const bool           isRenderTarget = false;
	const bool           isDynamic      = false;
	const bool           genMipmaps     = false;
	const bool           isShared       = false;
	const MTLPixelFormat mtlPixelFormat = MTLPixelFormatInvalid;

	MTLTextureDescriptor *textureDesc = nil;
	id<MTLTexture>       texture = nil;

	std::vector<std::vector<uint8_t>> data;

	void GenerateMipmap();
	void BackupTexture(const uint8_t **data);
	void UploadTexture();
	void InitTexture();

	inline void Release() {texture = nil;}
	void Rebuild();

	gs_texture_2d(gs_device_t *device, uint32_t width, uint32_t height,
			gs_color_format colorFormat, uint32_t levels,
			const uint8_t **data, uint32_t flags,
			gs_texture_type type);

	gs_texture_2d(gs_device_t *device, id<MTLTexture> texture);
};

struct gs_zstencil_buffer : gs_obj {
	const uint32_t           width = 0, height = 0;
	const gs_zstencil_format format = GS_ZS_NONE;

	MTLTextureDescriptor     *textureDesc;
	id<MTLTexture>           texture;

	void InitBuffer();

	inline void Release() {texture = nil;}
	inline void Rebuild() {InitBuffer();}

	gs_zstencil_buffer(gs_device_t *device, uint32_t width,
			uint32_t height, gs_zstencil_format format);
};

struct gs_stage_surface : gs_obj {
	const uint32_t        width = 0, height = 0, bytePerRow = 0;
	const gs_color_format format = GS_UNKNOWN;

	MTLTextureDescriptor  *textureDesc;
	id<MTLTexture>        texture;

	std::vector<uint8_t>  data;

	void DownloadTexture();
	void InitTexture();

	inline void Release() {texture = nil;}
	inline void Rebuild() {InitTexture();};

	gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height,
			gs_color_format colorFormat);
};

struct gs_sampler_state : gs_obj {
	const gs_sampler_info info;

	MTLSamplerDescriptor  *samplerDesc;
	id<MTLSamplerState>   samplerState;

	void InitSampler();

	inline void Release() {samplerState = nil;}
	inline void Rebuild() {InitSampler();}

	gs_sampler_state(gs_device_t *device, const gs_sampler_info *info);
};

struct gs_shader_param {
	const std::string          name;
	const gs_shader_param_type type;
	const int                  arrayCount;

	struct gs_sampler_state    *nextSampler = nullptr;

	uint32_t                   textureID;
	size_t                     pos;

	std::vector<uint8_t>       curValue;
	std::vector<uint8_t>       defaultValue;
	bool                       changed;

	gs_shader_param(shader_var &var, uint32_t &texCounter);
};

struct ShaderError {
	const std::string error;

	inline ShaderError(NSError *error)
		: error (error.localizedDescription.UTF8String)
	{
	}
};

struct gs_shader : gs_obj {
	const gs_shader_type         type;
	std::string                  source;
	id<MTLLibrary>               library;
	id<MTLFunction>              function;
	std::vector<gs_shader_param> params;

	size_t                       constantSize = 0;

	std::vector<uint8_t>         data;

	inline void UpdateParam(uint8_t *data, gs_shader_param &param);
	void UploadParams(id<MTLRenderCommandEncoder> commandEncoder);

	void BuildConstantBuffer();
	void Compile(std::string shaderStr);

	inline void Release()
	{
		function = nil;
		library = nil;
	}
	void Rebuild();

	inline gs_shader(gs_device_t *device, gs_type obj_type,
			gs_shader_type type)
		: gs_obj       (device, obj_type),
		  type         (type)
	{
	}
};

struct gs_vertex_shader : gs_shader {
	MTLVertexDescriptor *vertexDesc;

	bool     hasNormals;
	bool     hasColors;
	bool     hasTangents;
	uint32_t texUnits;

	gs_shader_param *world, *viewProj;

	inline uint32_t NumBuffersExpected() const
	{
		uint32_t count = texUnits + 1;
		if (hasNormals)  count++;
		if (hasColors)   count++;
		if (hasTangents) count++;

		return count;
	}

	gs_vertex_shader(gs_device_t *device, const char *file,
			const char *shaderString);
};

struct ShaderSampler {
	std::string      name;
	gs_sampler_state sampler;

	inline ShaderSampler(const char *name, gs_device_t *device,
			gs_sampler_info *info)
		: name    (name),
		  sampler (device, info)
	{
	}
};

struct gs_pixel_shader : gs_shader {
	std::vector<std::unique_ptr<ShaderSampler>> samplers;

	inline void GetSamplerStates(gs_sampler_state **states)
	{
		size_t i;
		for (i = 0; i < samplers.size(); i++)
			states[i] = &samplers[i]->sampler;
		for (; i < GS_MAX_TEXTURES; i++)
			states[i] = nullptr;
	}

	gs_pixel_shader(gs_device_t *device, const char *file,
			const char *shaderString);
};

struct gs_swap_chain : gs_obj {
	uint32_t            numBuffers;
	NSView              *view;
	CAMetalLayer        *metalLayer;

	gs_init_data        initData;
	id<CAMetalDrawable> nextDrawable;
	std::unique_ptr<gs_texture_2d> nextTarget;

	gs_texture_2d *GetTarget();
	gs_texture_2d *NextTarget();
	void Resize(uint32_t cx, uint32_t cy);


	inline void Release()
	{
		nextTarget.reset();
		nextDrawable = nil;
	}
	void Rebuild();

	gs_swap_chain(gs_device *device, const gs_init_data *data);
};

struct ClearState {
	uint32_t    flags;
	struct vec4 color;
	float       depth;
	uint8_t     stencil;

	inline ClearState()
		: flags   (0),
		  color   ({}),
		  depth   (0.0f),
		  stencil (0)
	{
	}
};

struct BlendState {
	bool          blendEnabled;
	gs_blend_type srcFactorC;
	gs_blend_type destFactorC;
	gs_blend_type srcFactorA;
	gs_blend_type destFactorA;

	bool          redEnabled;
	bool          greenEnabled;
	bool          blueEnabled;
	bool          alphaEnabled;

	inline BlendState()
		: blendEnabled (true),
		  srcFactorC   (GS_BLEND_SRCALPHA),
		  destFactorC  (GS_BLEND_INVSRCALPHA),
		  srcFactorA   (GS_BLEND_ONE),
		  destFactorA  (GS_BLEND_ONE),
		  redEnabled   (true),
		  greenEnabled (true),
		  blueEnabled  (true),
		  alphaEnabled (true)
	{
	}

	inline BlendState(const BlendState &state)
	{
		memcpy(this, &state, sizeof(BlendState));
	}
};

struct RasterState {
	gs_rect        viewport;
	gs_cull_mode   cullMode;
	bool           scissorEnabled;
	gs_rect        scissorRect;

	MTLViewport    mtlViewport;
	MTLCullMode    mtlCullMode;
	MTLScissorRect mtlScissorRect;

	inline RasterState()
		: viewport       (),
		  cullMode       (GS_BACK),
		  scissorEnabled (false),
		  scissorRect    (),
		  mtlCullMode    (MTLCullModeBack)
	{
	}

	inline RasterState(const RasterState &state)
	{
		memcpy(this, &state, sizeof(RasterState));
	}
};

struct StencilSide {
	gs_depth_test test;
	gs_stencil_op_type fail;
	gs_stencil_op_type zfail;
	gs_stencil_op_type zpass;

	inline StencilSide()
		: test  (GS_ALWAYS),
		  fail  (GS_KEEP),
		  zfail (GS_KEEP),
		  zpass (GS_KEEP)
	{
	}
};

struct ZStencilState {
	bool          depthEnabled;
	bool          depthWriteEnabled;
	gs_depth_test depthFunc;

	bool          stencilEnabled;
	bool          stencilWriteEnabled;
	StencilSide   stencilFront;
	StencilSide   stencilBack;

	MTLDepthStencilDescriptor *dsd;

	inline ZStencilState()
		: depthEnabled        (true),
		  depthWriteEnabled   (true),
		  depthFunc           (GS_LESS),
		  stencilEnabled      (false),
		  stencilWriteEnabled (true)
	{
		dsd = [[MTLDepthStencilDescriptor alloc] init];
	}

	inline ZStencilState(const ZStencilState &state)
	{
		memcpy(this, &state, sizeof(ZStencilState));
	}
};

struct gs_device {
	uint32_t                    devIdx = 0;
	uint16_t                    featureSetFamily = 0;
	uint16_t                    featureSetVersion = 0;

	MTLRenderPassDescriptor     *passDesc;
	MTLRenderPipelineDescriptor *pipelineDesc;
	
	id<MTLDevice>               device;
	id<MTLCommandQueue>         commandQueue;
	id<MTLCommandBuffer>        commandBuffer;
	id<MTLRenderPipelineState>  pipelineState;
	id<MTLDepthStencilState>    depthStencilState;

	gs_texture_2d               *curRenderTarget = nullptr;
	int                         curRenderSide = 0;
	gs_zstencil_buffer          *curZStencilBuffer = nullptr;
	gs_texture                  *curTextures[GS_MAX_TEXTURES] = {};
	gs_sampler_state            *curSamplers[GS_MAX_TEXTURES] = {};
	gs_vertex_buffer            *curVertexBuffer = nullptr;
	gs_index_buffer             *curIndexBuffer = nullptr;
	gs_vertex_shader            *curVertexShader = nullptr;
	gs_pixel_shader             *curPixelShader = nullptr;
	gs_swap_chain               *curSwapChain = nullptr;
	gs_stage_surface            *curStageSurface = nullptr;

	gs_vertex_buffer            *lastVertexBuffer = nullptr;
	gs_vertex_shader            *lastVertexShader = nullptr;

	gs_texture_2d               *preserveClearTarget = nullptr;
	std::stack<std::pair<gs_texture_2d *, ClearState>> clearStates;

	bool                        piplineStateChanged = false;
	BlendState                  blendState;
	RasterState                 rasterState;
	ZStencilState               zstencilState;

	std::stack<matrix4>         projStack;

	matrix4                     curProjMatrix;
	matrix4                     curViewMatrix;
	matrix4                     curViewProjMatrix;

	gs_obj                      *first_obj = nullptr;

	std::mutex                  mutexObj;
	std::vector<id<MTLBuffer>>  curBufferPool;
	std::queue<std::vector<id<MTLBuffer>>> bufferPools;
	std::vector<id<MTLBuffer>>  unusedBufferPool;

	void InitDevice(uint32_t adapterIdx);

	/* Create Draw Command */
	void SetClear();
	void LoadSamplers(id<MTLRenderCommandEncoder> commandEncoder);
	void LoadRasterState(id<MTLRenderCommandEncoder> commandEncoder);
	void LoadZStencilState(id<MTLRenderCommandEncoder> commandEncoder);
	void UpdateViewProjMatrix();
	void UploadVertexBuffer(id<MTLRenderCommandEncoder> commandEncoder);
	void UploadTextures(id<MTLRenderCommandEncoder> commandEncoder);
	void UploadSamplers(id<MTLRenderCommandEncoder> commandEncoder);
	void DrawPrimitives(id<MTLRenderCommandEncoder> commandEncoder,
			gs_draw_mode drawMode,
			uint32_t startVert, uint32_t numVerts);
	void Draw(gs_draw_mode drawMode, uint32_t startVert, uint32_t numVerts);

	/* Buffer Management */
	id<MTLBuffer> GetBuffer(void *data, size_t length);
	void PushResources();
	void ReleaseResources();

	/* Other */
	void RebuildDevice();
	inline void CopyTex(id<MTLTexture> dst,
			uint32_t dst_x, uint32_t dst_y,
			gs_texture_t *src, uint32_t src_x, uint32_t src_y,
			uint32_t src_w, uint32_t src_h);

	gs_device(uint32_t adapterIdx);
};
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef libobs_metal_EXPORTS
void device_clear_textures(gs_device_t *device);
#endif

#ifdef __cplusplus
}
#endif
