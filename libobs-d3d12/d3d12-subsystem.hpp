/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#pragma once

#include <util/windows/win-version.h>

#include <vector>
#include <string>
#include <memory>
#include <queue>

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <dxgidebug.h>
#include <util/base.h>
#include <graphics/matrix4.h>
#include <graphics/graphics.h>
#include <graphics/device-exports.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/HRError.hpp>

struct shader_var;
struct shader_sampler;
struct gs_vertex_shader;
struct gs_pixel_shader;
struct gs_staging_descriptor_pool;
struct gs_staging_descriptor;
struct gs_graphics_rootsignature;

#define MAX_UNIFORM_BUFFERS_PER_STAGE 16
#define NUM_DESCRIPTORS_PER_HEAP 256

#define GS_GPU_BUFFERUSAGE_VERTEX (1u << 0)                /**< Buffer is a vertex buffer. */
#define GS_GPU_BUFFERUSAGE_INDEX (1u << 1)                 /**< Buffer is an index buffer. */
#define GS_GPU_BUFFERUSAGE_INDIRECT (1u << 2)              /**< Buffer is an indirect buffer. */
#define GS_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ (1u << 3) /**< Buffer supports storage reads in graphics stages. */
#define GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ (1u << 4)  /**< Buffer supports storage reads in the compute stage. */
#define GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE (1u << 5) /**< Buffer supports storage writes in the compute stage. */

static inline uint32_t GetWinVer()
{
	struct win_version_info ver;
	get_win_ver(&ver);

	return (ver.major << 8) | ver.minor;
}

static inline DXGI_FORMAT ConvertGSTextureFormatResource(gs_color_format format)
{
	switch (format) {
	case GS_UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
	case GS_A8:
		return DXGI_FORMAT_A8_UNORM;
	case GS_R8:
		return DXGI_FORMAT_R8_UNORM;
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	case GS_R10G10B10A2:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	case GS_RGBA16:
		return DXGI_FORMAT_R16G16B16A16_UNORM;
	case GS_R16:
		return DXGI_FORMAT_R16_UNORM;
	case GS_RGBA16F:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case GS_RGBA32F:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case GS_RG16F:
		return DXGI_FORMAT_R16G16_FLOAT;
	case GS_RG32F:
		return DXGI_FORMAT_R32G32_FLOAT;
	case GS_R16F:
		return DXGI_FORMAT_R16_FLOAT;
	case GS_R32F:
		return DXGI_FORMAT_R32_FLOAT;
	case GS_DXT1:
		return DXGI_FORMAT_BC1_UNORM;
	case GS_DXT3:
		return DXGI_FORMAT_BC2_UNORM;
	case GS_DXT5:
		return DXGI_FORMAT_BC3_UNORM;
	case GS_R8G8:
		return DXGI_FORMAT_R8G8_UNORM;
	case GS_RGBA_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX_UNORM:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case GS_BGRA_UNORM:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	case GS_RG16:
		return DXGI_FORMAT_R16G16_UNORM;
	}

	return DXGI_FORMAT_UNKNOWN;
}

static inline DXGI_FORMAT ConvertGSTextureFormatView(gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

static inline DXGI_FORMAT ConvertGSTextureFormatViewLinear(gs_color_format format)
{
	switch (format) {
	case GS_RGBA:
		return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	case GS_BGRX:
		return DXGI_FORMAT_B8G8R8X8_UNORM_SRGB;
	case GS_BGRA:
		return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

static inline gs_color_format ConvertDXGITextureFormat(DXGI_FORMAT format)
{
	switch (format) {
	case DXGI_FORMAT_A8_UNORM:
		return GS_A8;
	case DXGI_FORMAT_R8_UNORM:
		return GS_R8;
	case DXGI_FORMAT_R8G8_UNORM:
		return GS_R8G8;
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
		return GS_RGBA;
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
		return GS_BGRX;
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
		return GS_BGRA;
	case DXGI_FORMAT_R10G10B10A2_UNORM:
		return GS_R10G10B10A2;
	case DXGI_FORMAT_R16G16B16A16_UNORM:
		return GS_RGBA16;
	case DXGI_FORMAT_R16_UNORM:
		return GS_R16;
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
		return GS_RGBA16F;
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
		return GS_RGBA32F;
	case DXGI_FORMAT_R16G16_FLOAT:
		return GS_RG16F;
	case DXGI_FORMAT_R32G32_FLOAT:
		return GS_RG32F;
	case DXGI_FORMAT_R16_FLOAT:
		return GS_R16F;
	case DXGI_FORMAT_R32_FLOAT:
		return GS_R32F;
	case DXGI_FORMAT_BC1_UNORM:
		return GS_DXT1;
	case DXGI_FORMAT_BC2_UNORM:
		return GS_DXT3;
	case DXGI_FORMAT_BC3_UNORM:
		return GS_DXT5;
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		return GS_RGBA_UNORM;
	case DXGI_FORMAT_B8G8R8X8_UNORM:
		return GS_BGRX_UNORM;
	case DXGI_FORMAT_B8G8R8A8_UNORM:
		return GS_BGRA_UNORM;
	case DXGI_FORMAT_R16G16_UNORM:
		return GS_RG16;
	}

	return GS_UNKNOWN;
}

static inline DXGI_FORMAT ConvertGSZStencilFormat(gs_zstencil_format format)
{
	switch (format) {
	case GS_ZS_NONE:
		return DXGI_FORMAT_UNKNOWN;
	case GS_Z16:
		return DXGI_FORMAT_D16_UNORM;
	case GS_Z24_S8:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;
	case GS_Z32F:
		return DXGI_FORMAT_D32_FLOAT;
	case GS_Z32F_S8X24:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	}

	return DXGI_FORMAT_UNKNOWN;
}

static inline D3D12_COMPARISON_FUNC ConvertGSDepthTest(gs_depth_test test)
{
	switch (test) {
	case GS_NEVER:
		return D3D12_COMPARISON_FUNC_NEVER;
	case GS_LESS:
		return D3D12_COMPARISON_FUNC_LESS;
	case GS_LEQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case GS_EQUAL:
		return D3D12_COMPARISON_FUNC_EQUAL;
	case GS_GEQUAL:
		return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
	case GS_GREATER:
		return D3D12_COMPARISON_FUNC_GREATER;
	case GS_NOTEQUAL:
		return D3D12_COMPARISON_FUNC_NOT_EQUAL;
	case GS_ALWAYS:
		return D3D12_COMPARISON_FUNC_ALWAYS;
	}

	return D3D12_COMPARISON_FUNC_NEVER;
}

static inline D3D12_STENCIL_OP ConvertGSStencilOp(gs_stencil_op_type op)
{
	switch (op) {
	case GS_KEEP:
		return D3D12_STENCIL_OP_KEEP;
	case GS_ZERO:
		return D3D12_STENCIL_OP_ZERO;
	case GS_REPLACE:
		return D3D12_STENCIL_OP_REPLACE;
	case GS_INCR:
		return D3D12_STENCIL_OP_INCR;
	case GS_DECR:
		return D3D12_STENCIL_OP_DECR;
	case GS_INVERT:
		return D3D12_STENCIL_OP_INVERT;
	}

	return D3D12_STENCIL_OP_KEEP;
}

static inline D3D12_BLEND ConvertGSBlendType(gs_blend_type type)
{
	switch (type) {
	case GS_BLEND_ZERO:
		return D3D12_BLEND_ZERO;
	case GS_BLEND_ONE:
		return D3D12_BLEND_ONE;
	case GS_BLEND_SRCCOLOR:
		return D3D12_BLEND_SRC_COLOR;
	case GS_BLEND_INVSRCCOLOR:
		return D3D12_BLEND_INV_SRC_COLOR;
	case GS_BLEND_SRCALPHA:
		return D3D12_BLEND_SRC_ALPHA;
	case GS_BLEND_INVSRCALPHA:
		return D3D12_BLEND_INV_SRC_ALPHA;
	case GS_BLEND_DSTCOLOR:
		return D3D12_BLEND_DEST_COLOR;
	case GS_BLEND_INVDSTCOLOR:
		return D3D12_BLEND_INV_DEST_COLOR;
	case GS_BLEND_DSTALPHA:
		return D3D12_BLEND_DEST_ALPHA;
	case GS_BLEND_INVDSTALPHA:
		return D3D12_BLEND_INV_DEST_ALPHA;
	case GS_BLEND_SRCALPHASAT:
		return D3D12_BLEND_SRC_ALPHA_SAT;
	}

	return D3D12_BLEND_ONE;
}

static inline D3D12_BLEND_OP ConvertGSBlendOpType(gs_blend_op_type type)
{
	switch (type) {
	case GS_BLEND_OP_ADD:
		return D3D12_BLEND_OP_ADD;
	case GS_BLEND_OP_SUBTRACT:
		return D3D12_BLEND_OP_SUBTRACT;
	case GS_BLEND_OP_REVERSE_SUBTRACT:
		return D3D12_BLEND_OP_REV_SUBTRACT;
	case GS_BLEND_OP_MIN:
		return D3D12_BLEND_OP_MIN;
	case GS_BLEND_OP_MAX:
		return D3D12_BLEND_OP_MAX;
	}

	return D3D12_BLEND_OP_ADD;
}

static inline D3D12_CULL_MODE ConvertGSCullMode(gs_cull_mode mode)
{
	switch (mode) {
	case GS_BACK:
		return D3D12_CULL_MODE_BACK;
	case GS_FRONT:
		return D3D12_CULL_MODE_FRONT;
	case GS_NEITHER:
		return D3D12_CULL_MODE_NONE;
	}

	return D3D12_CULL_MODE_BACK;
}

static inline D3D12_PRIMITIVE_TOPOLOGY ConvertGSTopology(gs_draw_mode mode)
{
	switch (mode) {
	case GS_POINTS:
		return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
	case GS_LINES:
		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	case GS_LINESTRIP:
		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
	case GS_TRIS:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	case GS_TRISTRIP:
		return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}

	return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
}

static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE ConvertD3D12Topology(D3D12_PRIMITIVE_TOPOLOGY tology)
{
	switch (tology) {
	case D3D_PRIMITIVE_TOPOLOGY_POINTLIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	case D3D_PRIMITIVE_TOPOLOGY_LINELIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case D3D_PRIMITIVE_TOPOLOGY_LINESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
		return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	}

	return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
}

struct VBDataPtr {
	gs_vb_data *data;

	inline VBDataPtr(gs_vb_data *data) : data(data) {}
	inline ~VBDataPtr() { gs_vbdata_destroy(data); }
};

enum class gs_type {
	gs_vertex_buffer,
	gs_index_buffer,
	gs_upload_buffer,
	gs_gpu_buffer,
	gs_uniform_buffer,
	gs_download_buffer,
	gs_texture_2d,
	gs_zstencil_buffer,
	gs_stage_surface,
	gs_sampler_state,
	gs_vertex_shader,
	gs_pixel_shader,
	gs_duplicator,
	gs_swap_chain,
	gs_timer,
	gs_timer_range,
	gs_texture_3d,
};

struct gs_obj {
	gs_device_t *device = nullptr;
	gs_type obj_type = gs_type::gs_vertex_buffer;
	gs_obj *next = nullptr;
	gs_obj **prev_next = nullptr;

	inline gs_obj() : device(nullptr), next(nullptr), prev_next(nullptr) {}

	gs_obj(gs_device_t *device, gs_type type);
	virtual ~gs_obj();
};

struct gs_descriptor_allocator {
	gs_device_t *device;

	D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeadType;
	ComPtr<ID3D12DescriptorHeap> currentHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE currentHandle;
	uint32_t descriptorSize = 0;
	uint32_t remainingFreeHandles = 0;
	std::vector<ComPtr<ID3D12DescriptorHeap>> descriptorHeapPool;

       	gs_descriptor_allocator(gs_device_t *device, D3D12_DESCRIPTOR_HEAP_TYPE type);
	~gs_descriptor_allocator();

	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t count = 1);

	ComPtr<ID3D12DescriptorHeap> RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);


	void Release(void);
};

struct gs_texture : gs_obj {
	gs_texture_type type = GS_TEXTURE_2D;
	uint32_t layerCountOrDepth = 1; // layer count for 2d, depth for 3d
	uint32_t levels = 0;
	int32_t sampleCount = 1;
	gs_color_format format = GS_BGRA;

	inline gs_texture(gs_texture_type type, uint32_t levels, gs_color_format format)
		: type(type),
		  levels(levels),
		  format(format)
	{
	}

	inline gs_texture(gs_device *device, gs_type obj_type, gs_texture_type type)
		: gs_obj(device, obj_type),
		  type(type)
	{
	}

	inline gs_texture(gs_device *device, gs_type obj_type, gs_texture_type type, uint32_t levels,
			  gs_color_format format)
		: gs_obj(device, obj_type),
		  type(type),
		  levels(levels),
		  format(format)
	{
	}
};

struct gs_texture_2d : gs_texture {
	D3D12_CPU_DESCRIPTOR_HANDLE textureCpuDescriptorHandle;
	ComPtr<ID3D12Resource> texture;
	D3D12_RESOURCE_STATES resourceState = (D3D12_RESOURCE_STATES)0;

	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetCpuDescriptorHandle[6] = {0};
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetCpuLinearDescriptorHandle[6] = {0};

	uint32_t width = 0, height = 0;
	uint32_t flags = 0;
	DXGI_FORMAT dxgiFormatResource = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatView = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatViewLinear = DXGI_FORMAT_UNKNOWN;

	bool isRenderTarget = false;
	bool isDynamic = false;
	bool genMipmaps = false;

	gs_texture_2d *pairedTexture = nullptr;
	bool twoPlane = false;
	bool chroma = false;
	bool acquired = false;

	std::vector<std::vector<uint8_t>> data;
	std::vector<D3D12_SUBRESOURCE_DATA> srd;
	D3D12_RESOURCE_DESC td;
	D3D12_HEAP_PROPERTIES heapProp;

	void InitSRD(std::vector<D3D12_SUBRESOURCE_DATA> &srd);
	void InitTexture(const uint8_t *const *data);
	void InitResourceView();
	void InitRenderTargets();
	void BackupTexture(const uint8_t *const *data);
	void GetSharedHandle(IDXGIResource *dxgi_res);
	void UpdateSubresources();

	bool Map(int32_t subresourceIndex, D3D12_MEMCPY_DEST *map);
	void Unmap(int32_t subresourceIndex);

	inline void Release()
	{
		textureCpuDescriptorHandle = { };

		memset(renderTargetCpuDescriptorHandle, 0, sizeof(renderTargetCpuDescriptorHandle));
		memset(renderTargetCpuLinearDescriptorHandle, 0, sizeof(renderTargetCpuLinearDescriptorHandle));

		texture.Release();
	}

	inline gs_texture_2d() : gs_texture(GS_TEXTURE_2D, 0, GS_UNKNOWN) {}

	gs_texture_2d(gs_device_t *device, uint32_t width, uint32_t height, gs_color_format colorFormat,
		      uint32_t levels, const uint8_t *const *data, uint32_t flags, gs_texture_type type,
		      bool gdiCompatible, bool twoPlane = false);

	gs_texture_2d(gs_device_t *device, ID3D12Resource *nv12, uint32_t flags);
	gs_texture_2d(gs_device_t *device, uint32_t handle, bool ntHandle = false);
	gs_texture_2d(gs_device_t *device, ID3D12Resource *obj);
};

struct gs_texture_3d : gs_texture {
	ComPtr<ID3D12Resource> texture;
	D3D12_RESOURCE_DESC td;

	uint32_t width = 0, height = 0, depth = 0;
	uint32_t flags = 0;
	DXGI_FORMAT dxgiFormatResource = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatView = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT dxgiFormatViewLinear = DXGI_FORMAT_UNKNOWN;
	bool isDynamic = false;
	bool isShared = false;
	bool genMipmaps = false;
	uint32_t sharedHandle = GS_INVALID_HANDLE;

	bool chroma = false;
	bool acquired = false;

	std::vector<std::vector<uint8_t>> data;
	std::vector<D3D12_SUBRESOURCE_DATA> srd;

	void InitSRD(std::vector<D3D12_SUBRESOURCE_DATA> &srd);
	void InitTexture(const uint8_t *const *data);
	void InitResourceView();
	void BackupTexture(const uint8_t *const *data);
	void GetSharedHandle(IDXGIResource *dxgi_res);

	inline void Release() {}

	inline gs_texture_3d() : gs_texture(GS_TEXTURE_3D, 0, GS_UNKNOWN) {}

	gs_texture_3d(gs_device_t *device, uint32_t width, uint32_t height, uint32_t depth, gs_color_format colorFormat,
		      uint32_t levels, const uint8_t *const *data, uint32_t flags);

	gs_texture_3d(gs_device_t *device, uint32_t handle);
};

struct gs_zstencil_buffer : gs_obj {
	ComPtr<ID3D12Resource> texture;
	D3D12_CPU_DESCRIPTOR_HANDLE zsCpuDescriptorHandle;

	uint32_t width, height;
	gs_zstencil_format format;
	DXGI_FORMAT dxgiFormat;

	void InitBuffer();

	void inline Clear()
	{
		memset(&zsCpuDescriptorHandle, 0, sizeof(zsCpuDescriptorHandle));
		texture.Release();
	}

	inline void Release()
	{
		memset(&zsCpuDescriptorHandle, 0, sizeof(zsCpuDescriptorHandle));
		texture.Release();
	}

	inline gs_zstencil_buffer() {}

	gs_zstencil_buffer(gs_device_t *device, uint32_t width, uint32_t height, gs_zstencil_format format);
};

struct gs_stage_surface : gs_obj {
	ComPtr<ID3D12Resource> texture;

	uint32_t width, height;
	gs_color_format format;
	DXGI_FORMAT dxgiFormat;

	inline void Release() { texture.Release(); }

	gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height, gs_color_format colorFormat);
	gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height, bool p010);
};

struct gs_sampler_state : gs_obj {
	gs_sampler_info info;
	D3D12_SAMPLER_DESC sd;
	gs_sampler_state(gs_device_t *device, const gs_sampler_info *info);
};

struct gs_shader_param {
	std::string name;
	gs_shader_param_type type;

	uint32_t textureID;
	struct gs_sampler_state *nextSampler = nullptr;

	int arrayCount;

	size_t pos;

	std::vector<uint8_t> curValue;
	std::vector<uint8_t> defaultValue;
	bool changed;

	gs_shader_param(shader_var &var, uint32_t &texCounter);
};

struct ShaderError {
	ComPtr<ID3D10Blob> errors;
	HRESULT hr;

	inline ShaderError(const ComPtr<ID3D10Blob> &errors, HRESULT hr) : errors(errors), hr(hr) {}
};

struct gs_shader : gs_obj {
	gs_shader_type type;
	std::vector<gs_shader_param> params;

	size_t samplerCount = 0;
	size_t textureCount = 0;
	size_t uniform32BitBufferCount = 0; // const buffer
	size_t constantSize;

	std::vector<uint8_t> data;
	std::string actuallyShaderString;

	inline void UpdateParam(std::vector<uint8_t> &constData, gs_shader_param &param, bool &upload);
	void UploadParams();

	void BuildConstantBuffer();
	void Compile(const char *shaderStr, const char *file, const char *target, ID3D10Blob **shader);

	inline gs_shader(gs_device_t *device, gs_type obj_type, gs_shader_type type)
		: gs_obj(device, obj_type),
		  type(type),
		  constantSize(0)
	{
	}

	virtual ~gs_shader() {}
};

struct ShaderSampler {
	std::string name;
	gs_sampler_state sampler;

	const D3D12_SAMPLER_DESC &GetDesc() {
		return sampler.sd;
	}

	inline ShaderSampler(const char *name, gs_device_t *device, gs_sampler_info *info)
		: name(name),
		  sampler(device, info)
	{
	}
};

struct gs_vertex_shader : gs_shader {
	/*ComPtr<ID3D11VertexShader> shader;
	ComPtr<ID3D11InputLayout> layout;*/

	gs_shader_param *world, *viewProj;

	std::vector<D3D12_INPUT_ELEMENT_DESC> layoutData;

	bool hasNormals;
	bool hasColors;
	bool hasTangents;
	uint32_t nTexUnits;

	void Rebuild(ID3D12Device *dev);

	inline void Release()
	{
		/*shader.Release();
		layout.Release();
		constants.Release();*/
	}

	inline uint32_t NumBuffersExpected() const
	{
		uint32_t count = nTexUnits + 1;
		if (hasNormals)
			count++;
		if (hasColors)
			count++;
		if (hasTangents)
			count++;

		return count;
	}

	void GetBuffersExpected(const std::vector<D3D12_INPUT_ELEMENT_DESC> &inputs);

	gs_vertex_shader(gs_device_t *device, const char *file, const char *shaderString);
};

struct gs_duplicator : gs_obj {
	ComPtr<IDXGIOutputDuplication> duplicator;
	gs_texture_2d *texture;
	bool hdr = false;
	enum gs_color_space color_space = GS_CS_SRGB;
	float sdr_white_nits = 80.f;
	int idx;
	long refs;
	bool updated;

	void Start();

	inline void Release() { duplicator.Release(); }

	gs_duplicator(gs_device_t *device, int monitor_idx);
	~gs_duplicator();
};

struct gs_pixel_shader : gs_shader {
	std::vector<std::unique_ptr<ShaderSampler>> samplers;

	void Rebuild(ID3D12Device *dev);

	inline void Release() {}

	inline void GetSamplerDescriptor(gs_samplerstate_t **descriptor)
	{
		size_t i;
		for (i = 0; i < samplers.size(); i++)
			descriptor[i] = &samplers[i]->sampler;
		for (; i < GS_MAX_TEXTURES; i++)
			descriptor[i] = NULL;
	}

	gs_pixel_shader(gs_device_t *device, const char *file, const char *shaderString);
};

struct gs_swap_chain : gs_obj {
	HWND hwnd;
	gs_init_data initData;
	DXGI_SWAP_CHAIN_DESC swapDesc = {};
	gs_color_space space;

	gs_texture_2d target[GS_MAX_TEXTURES];
	gs_zstencil_buffer zs;
	ComPtr<IDXGISwapChain3> swap;
	int32_t currentBackBufferIndex = 0;

	void InitTarget(uint32_t cx, uint32_t cy);
	void InitZStencilBuffer(uint32_t cx, uint32_t cy);
	void Resize(uint32_t cx, uint32_t cy, gs_color_format format);
	void Init();

	inline void Release()
	{
		for (int32_t i = 0; i < GS_MAX_TEXTURES; ++i)
			target[i].Release();
		zs.Release();
		swap.Clear();
	}

	gs_swap_chain(gs_device *device, const gs_init_data *data);
	virtual ~gs_swap_chain();
};

struct gs_root_parameter {
	D3D12_ROOT_PARAMETER rootParam;

	 gs_root_parameter() { rootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF; }

	~gs_root_parameter() { Release(); }

	void Release()
	{
		if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			delete[] rootParam.DescriptorTable.pDescriptorRanges;

		rootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
	}

	void InitAsConstants(UINT Register, UINT NumDwords,
			     D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParam.ShaderVisibility = Visibility;
		rootParam.Constants.Num32BitValues = NumDwords;
		rootParam.Constants.ShaderRegister = Register;
		rootParam.Constants.RegisterSpace = Space;
	}

	void InitAsConstantBuffer(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL,
				  UINT Space = 0)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		rootParam.ShaderVisibility = Visibility;
		rootParam.Descriptor.ShaderRegister = Register;
		rootParam.Descriptor.RegisterSpace = Space;
	}

	void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL,
			     UINT Space = 0)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		rootParam.ShaderVisibility = Visibility;
		rootParam.Descriptor.ShaderRegister = Register;
		rootParam.Descriptor.RegisterSpace = Space;
	}

	void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL,
			     UINT Space = 0)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		rootParam.ShaderVisibility = Visibility;
		rootParam.Descriptor.ShaderRegister = Register;
		rootParam.Descriptor.RegisterSpace = Space;
	}

	void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count,
				   D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
	{
		InitAsDescriptorTable(1, Visibility);
		SetTableRange(0, Type, Register, Count, Space);
	}

	void InitAsDescriptorTable(UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParam.ShaderVisibility = Visibility;
		rootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
		rootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
	}

	void SetTableRange(UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0)
	{
		D3D12_DESCRIPTOR_RANGE *range = const_cast<D3D12_DESCRIPTOR_RANGE *>(rootParam.DescriptorTable.pDescriptorRanges + RangeIndex);
		range->RangeType = Type;
		range->NumDescriptors = Count;
		range->BaseShaderRegister = Register;
		range->RegisterSpace = Space;
		range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	const D3D12_ROOT_PARAMETER &operator()(void) const { return rootParam; }
};


struct gs_root_signature {
	gs_device_t *device;

	UINT numParameters = 0;
	UINT numSamplers = 0;
	UINT numInitializedStaticSamplers = 0;
	uint32_t descriptorTableBitMap = 0;        // One bit is set for root parameters that are non-sampler descriptor tables
	uint32_t samplerTableBitMap = 0;           // One bit is set for root parameters that are sampler descriptor tables
	uint32_t descriptorTableSize[16] = { 0 };  // Non-sampler descriptor tables need to know their descriptor count
	std::vector<gs_root_parameter> paramArray;
	std::vector<D3D12_STATIC_SAMPLER_DESC> samplerArray;
	ComPtr<ID3D12RootSignature> signature;

	gs_root_signature(gs_device_t *device, UINT numRootParams, UINT numStaticSamplers);
	void InitStaticSampler(UINT registerIndex, const D3D12_SAMPLER_DESC &nonStaticSamplerDesc,
			       D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);
	void InitParamWith(gs_vertex_shader *vertexShader, gs_pixel_shader *pixelShader);
	void Reset(UINT numRootParams, UINT numStaticSamplers = 1);
	gs_root_parameter &RootParameter(size_t entryIndex)
	{
		return paramArray[entryIndex];
	}
	void Build(const std::wstring &name, D3D12_ROOT_SIGNATURE_FLAGS flags);
	~gs_root_signature();
	void Release();
};


struct gs_pipeline_state_obj {

	const wchar_t *m_Name;

	std::shared_ptr<gs_root_signature> rootSignature;

	ComPtr<ID3D12PipelineState> pso;
	gs_pipeline_state_obj(const wchar_t *Name) : m_Name(Name), rootSignature(nullptr), pso(nullptr) {}
	void SetRootSignature(const RootSignature &BindMappings) { m_RootSignature = &BindMappings; }

	std::shared_ptr<gs_root_signature> GetRootSignature(void) const
	{ return rootSignature;
	}

	ComPtr<ID3D12PipelineState> GetPipelineStateObject(void) const { return pso; }
};

class gs_graphics_pso :  gs_pipeline_state_obj {
	friend class CommandContext;

public:
	// Start with empty state
	GraphicsPSO(const wchar_t *Name = L"Unnamed Graphics PSO");

	void SetBlendState(const D3D12_BLEND_DESC &BlendDesc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC &RasterizerDesc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC &DepthStencilDesc);
	void SetSampleMask(UINT SampleMask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
	void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1,
				   UINT MsaaQuality = 0);
	void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT *RTVFormats, DXGI_FORMAT DSVFormat,
				    UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs);
	void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

	// These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
	void SetVertexShader(const void *Binary, size_t Size)
	{
		m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
	}
	void SetPixelShader(const void *Binary, size_t Size)
	{
		m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
	}
	void SetGeometryShader(const void *Binary, size_t Size)
	{
		m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
	}
	void SetHullShader(const void *Binary, size_t Size)
	{
		m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
	}
	void SetDomainShader(const void *Binary, size_t Size)
	{
		m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
	}

	void SetVertexShader(const D3D12_SHADER_BYTECODE &Binary) { m_PSODesc.VS = Binary; }
	void SetPixelShader(const D3D12_SHADER_BYTECODE &Binary) { m_PSODesc.PS = Binary; }
	void SetGeometryShader(const D3D12_SHADER_BYTECODE &Binary) { m_PSODesc.GS = Binary; }
	void SetHullShader(const D3D12_SHADER_BYTECODE &Binary) { m_PSODesc.HS = Binary; }
	void SetDomainShader(const D3D12_SHADER_BYTECODE &Binary) { m_PSODesc.DS = Binary; }

	// Perform validation and compute a hash value for fast state block comparisons
	void Finalize();


	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
	std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;
};

struct gs_compute_pso : gs_pipeline_state_obj {
	friend class CommandContext;

	ComputePSO(const wchar_t *Name = L"Unnamed Compute PSO");

	void SetComputeShader(const void *Binary, size_t Size)
	{
		m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
	}
	void SetComputeShader(const D3D12_SHADER_BYTECODE &Binary) { m_PSODesc.CS = Binary; }

	void Finalize();

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
};


struct gs_vertex_buffer : gs_obj {
	ComPtr<ID3D12Resource> vertexBuffer;
	ComPtr<ID3D12Resource> normalBuffer;
	ComPtr<ID3D12Resource> colorBuffer;
	ComPtr<ID3D12Resource> tangentBuffer;
	D3D12_VERTEX_BUFFER_VIEW tangentBufferView;

	std::vector<ComPtr<ID3D12Resource>> uvBuffers;

	bool dynamic;
	VBDataPtr vbd;
	size_t numVerts;
	std::vector<size_t> uvSizes;

	void FlushBuffer(ID3D12Resource *buffer, void *array, size_t elementSize);

	UINT MakeBufferList(gs_vertex_shader *shader, D3D12_VERTEX_BUFFER_VIEW *views);

	void InitBuffer(const size_t elementSize, const size_t numVerts, void *array, ID3D12Resource **buffer);

	void BuildBuffers();

	inline void Release()
	{
		vertexBuffer.Release();
		normalBuffer.Release();
		colorBuffer.Release();
		tangentBuffer.Release();
		uvBuffers.clear();
	}

	gs_vertex_buffer(gs_device_t *device, struct gs_vb_data *data, uint32_t flags);
};

/* exception-safe RAII wrapper for index buffer data (NOTE: not copy-safe) */
struct DataPtr {
	void *data;

	inline DataPtr(void *data) : data(data) {}
	inline ~DataPtr() { bfree(data); }
};

struct gs_index_buffer : gs_obj {
	ComPtr<ID3D12Resource> indexBuffer;
	D3D12_INDEX_BUFFER_VIEW view;
	bool dynamic;
	gs_index_type type;
	size_t indexSize;
	size_t num;
	DataPtr indices;

	void InitBuffer();

	inline ~gs_index_buffer() { indexBuffer.Release(); }

	inline void Release() { indexBuffer.Release(); }

	gs_index_buffer(gs_device_t *device, enum gs_index_type type, void *indices, size_t num, uint32_t flags);
};

struct BlendState {
	bool blendEnabled;
	gs_blend_type srcFactorC;
	gs_blend_type destFactorC;
	gs_blend_type srcFactorA;
	gs_blend_type destFactorA;
	gs_blend_op_type op;

	bool redEnabled;
	bool greenEnabled;
	bool blueEnabled;
	bool alphaEnabled;

	inline bool operator==(const BlendState &other) const
	{
		return blendEnabled == other.blendEnabled && srcFactorC == other.srcFactorC &&
		       destFactorC == other.destFactorC && srcFactorA == other.srcFactorA && op == other.op &&
		       redEnabled == other.redEnabled && greenEnabled == other.greenEnabled &&
		       blueEnabled == other.blueEnabled && alphaEnabled == other.alphaEnabled;
	}

	inline BlendState()
		: blendEnabled(true),
		  srcFactorC(GS_BLEND_SRCALPHA),
		  destFactorC(GS_BLEND_INVSRCALPHA),
		  srcFactorA(GS_BLEND_ONE),
		  destFactorA(GS_BLEND_INVSRCALPHA),
		  op(GS_BLEND_OP_ADD),
		  redEnabled(true),
		  greenEnabled(true),
		  blueEnabled(true),
		  alphaEnabled(true)
	{
	}

	inline BlendState(const BlendState &state) { memcpy(this, &state, sizeof(BlendState)); }
};

struct StencilSide {
	gs_depth_test test;
	gs_stencil_op_type fail;
	gs_stencil_op_type zfail;
	gs_stencil_op_type zpass;

	inline bool operator==(const StencilSide &other) const
	{
		return test == other.test && fail == other.fail && zfail == other.zfail && zpass == other.zpass;
	}

	inline bool operator!=(const StencilSide &other) const { return !(*this == other); }

	inline StencilSide() : test(GS_ALWAYS), fail(GS_KEEP), zfail(GS_KEEP), zpass(GS_KEEP) {}
};

struct ZStencilState {
	bool depthEnabled;
	bool depthWriteEnabled;
	gs_depth_test depthFunc;

	bool stencilEnabled;
	bool stencilWriteEnabled;
	StencilSide stencilFront;
	StencilSide stencilBack;

	inline ZStencilState()
		: depthEnabled(true),
		  depthWriteEnabled(true),
		  depthFunc(GS_LESS),
		  stencilEnabled(false),
		  stencilWriteEnabled(true)
	{
	}

	inline bool operator==(const ZStencilState &other) const
	{
		return depthEnabled == other.depthEnabled && depthWriteEnabled == other.depthWriteEnabled &&
		       depthFunc == other.depthFunc && stencilEnabled == other.stencilEnabled &&
		       stencilWriteEnabled == other.stencilWriteEnabled && stencilFront == other.stencilFront &&
		       stencilBack == other.stencilBack;
	}

	inline bool operator!=(const ZStencilState &other) const { return !(*this == other); }

	inline ZStencilState(const ZStencilState &state) { memcpy(this, &state, sizeof(ZStencilState)); }
};

struct RasterState {
	gs_cull_mode cullMode;

	inline bool operator==(const RasterState &other) const { return cullMode == other.cullMode; }

	inline bool operator!=(const RasterState &other) const { return !(*this == other); }

	inline RasterState() : cullMode(GS_BACK) {}

	inline RasterState(const RasterState &state) { memcpy(this, &state, sizeof(RasterState)); }
};

struct mat4float {
	float mat[16];
};

struct gs_monitor_color_info {
	bool hdr;
	UINT bits_per_color;
	ULONG sdr_white_nits;

	gs_monitor_color_info(bool hdr, int bits_per_color, ULONG sdr_white_nits)
		: hdr(hdr),
		  bits_per_color(bits_per_color),
		  sdr_white_nits(sdr_white_nits)
	{
	}
};

struct gs_graphics_pipeline {
	ComPtr<ID3D12PipelineState> pipeline_state = nullptr;

	BlendState blendState;
	RasterState rasterState;
	ZStencilState zstencilState;

	gs_vertex_shader *vertexShader = nullptr;
	gs_pixel_shader *pixelShader = nullptr;

	D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
	DXGI_FORMAT zsformat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT rtvformat = DXGI_FORMAT_UNKNOWN;

	gs_root_signature *curRootSignature;

	inline bool operator==(const gs_graphics_pipeline &other) const
	{
		return pipeline_state == other.pipeline_state && blendState == other.blendState &&
		       rasterState == other.rasterState && zstencilState == other.zstencilState &&
		       vertexShader == other.vertexShader && pixelShader == other.pixelShader &&
		       topologyType == other.topologyType && zsformat == other.zsformat &&
		       rtvformat == other.rtvformat && curRootSignature == other.curRootSignature;
	}

	inline bool operator!=(const gs_graphics_pipeline &other) const { return !(*this == other); }
	inline ~gs_graphics_pipeline()
	{
		pipeline_state.Release();
		vertexShader = nullptr;
		pixelShader = nullptr;
	}

	inline gs_graphics_pipeline() {}

	inline gs_graphics_pipeline(ID3D12Device *device, const BlendState &blend, const RasterState &raster,
				    const ZStencilState &zs, gs_vertex_shader *vertexShader_,
				    gs_pixel_shader *pixelShader_, D3D12_PRIMITIVE_TOPOLOGY_TYPE topology_,
				    DXGI_FORMAT zsformat_, DXGI_FORMAT format)
		: blendState(blend),
		  rasterState(raster),
		  zstencilState(zs),
		  vertexShader(vertexShader_),
		  pixelShader(pixelShader_),
		  topologyType(topology_),
		  zsformat(zsformat_),
		  rtvformat(format),
		  curRootSignature(device, vertexShader_, pixelShader_)
	{
	}
};

using D3D12CommandAllocatorPtr = ComPtr<ID3D12CommandAllocator>;
struct D3D12CommandAllocatorInfo {
	D3D12CommandAllocatorInfo(uint64_t value, D3D12CommandAllocatorPtr ptr) : fenceValue(value), allocator(ptr) {}
	uint64_t fenceValue;
	D3D12CommandAllocatorPtr allocator;
};

struct gs_command_allocator {
	
	D3D12_COMMAND_LIST_TYPE commandListType;

	gs_device_t *device;
	std::vector<D3D12CommandAllocatorPtr> allocators;
	std::queue<D3D12CommandAllocatorInfo> readyAllocators;

	gs_command_allocator(gs_device_t *device, D3D12_COMMAND_LIST_TYPE type);
	~gs_command_allocator();

	void Release();

	D3D12CommandAllocatorPtr RequestAllocator(uint64_t completedFenceValue);
	void DiscardAllocator(uint64_t fenceValue, D3D12CommandAllocatorPtr allocator);
};

struct gs_command_queue {
	gs_device_t *device = nullptr;
	D3D12_COMMAND_LIST_TYPE commandListType;
	std::unique_ptr<gs_command_allocator> commandAllocators;

	ComPtr<ID3D12CommandQueue> commandQueue = nullptr;
	ComPtr<ID3D12Fence> fence = nullptr;
	uint64_t nextFenceValue = 0;
	uint64_t lastCompletedFenceValue = 0;
	HANDLE fenceEventHandle = 0;

	gs_command_queue(gs_device_t *device, D3D12_COMMAND_LIST_TYPE type);
	inline ~gs_command_queue() { Release(); }

	uint64_t IncrementFence(void);
	bool IsFenceComplete(uint64_t fenceValue);
	void StallForFence(uint64_t fenceValue);
	void StallForProducer(gs_command_queue *producer);
	void WaitForFence(uint64_t fenceValue);
	void WaitForIdle(void);

	ComPtr<ID3D12CommandQueue> GetCommandQueue() { return commandQueue; }

	uint64_t GetNextFenceValue() { return nextFenceValue; }

	uint64_t ExecuteCommandList(ID3D12GraphicsCommandList *list);
	D3D12CommandAllocatorPtr RequestAllocator(void);
	void DiscardAllocator(uint64_t fenceValueForReset, D3D12CommandAllocatorPtr);
	inline void Release();
};

struct gs_command_context {
	gs_device *device = nullptr;
	ID3D12GraphicsCommandList *commandList = nullptr;
	ID3D12CommandAllocator *currentAllocator = nullptr;

	D3D12_RESOURCE_BARRIER resourceBarrierBuffer[32] = {};
	UINT numBarriersToFlush = 0;

	inline ID3D12GraphicsCommandList *CommandList() { return commandList; }

	gs_command_context(gs_device *device);
	void Reset();
	uint64_t Flush(bool waitForCompletion = false);
	uint64_t Finish(bool waitForCompletion = false);
	void TransitionResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES beforeState,
				D3D12_RESOURCE_STATES newState, bool flushImmediate = false);
};

struct gs_device {
	ComPtr<IDXGIFactory6> factory;
	ComPtr<ID3D12Device> device;
	ComPtr<IDXGIAdapter> adapter;

	uint32_t adpIdx = 0;
	bool nv12Supported = false;
	bool p010Supported = false;
	bool fastClearSupported = false;

	gs_texture_2d *curRenderTarget = nullptr;
	gs_zstencil_buffer *curZStencilBuffer = nullptr;
	int curRenderSide = 0;
	enum gs_color_space curColorSpace = GS_CS_SRGB;
	bool curFramebufferSrgb = false;
	bool curFramebufferInvalidate = false;

	gs_texture *curTextures[GS_MAX_TEXTURES];
	gs_sampler_state *curSamplers[GS_MAX_TEXTURES];
	gs_vertex_buffer *curVertexBuffer = nullptr;
	gs_index_buffer *curIndexBuffer = nullptr;
	gs_vertex_shader *curVertexShader = nullptr;
	gs_pixel_shader *curPixelShader = nullptr;
	gs_swap_chain *curSwapChain = nullptr;

	ZStencilState curZstencilState;
	RasterState curRasterState;
	BlendState curBlendState;

	// D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_TYPE_DSV
	gs_descriptor_allocator *cbvSrvUavHeapDescriptor;
	gs_descriptor_allocator *samplerHeapDescriptor;
	gs_descriptor_allocator *rtvHeapDescriptor;
	gs_descriptor_allocator *dsvHeapDescriptor;

	// D3D12_COMMAND_LIST_TYPE_COMPUTE D3D12_COMMAND_LIST_TYPE_COPY D3D12_COMMAND_LIST_TYPE_DIRECT
	gs_command_queue *computeQueue;
	gs_command_queue *copyQueue;
	gs_command_queue *graphicsQueue;


	D3D12_PRIMITIVE_TOPOLOGY curToplogy;
	gs_graphics_pipeline curPipeline;

	std::vector<gs_graphics_pipeline> graphicsPipelines;
	gs_command_context *currentCommandContext;

	gs_rect viewport;

	std::vector<mat4float> projStack;

	matrix4 curProjMatrix;
	matrix4 curViewMatrix;
	matrix4 curViewProjMatrix;

	std::vector<gs_device_loss> loss_callbacks;
	gs_obj *first_obj = nullptr;
	std::vector<std::pair<HMONITOR, gs_monitor_color_info>> monitor_to_hdr;

	void InitFactory();
	void InitAdapter(uint32_t adapterIdx);
	void InitDevice(uint32_t adapterIdx);

	void WriteGPUDescriptor(gs_gpu_descriptor_heap *gpuHeap, D3D12_CPU_DESCRIPTOR_HANDLE *cpuHandle, int32_t count,
				D3D12_GPU_DESCRIPTOR_HANDLE *gpuBaseDescriptor);

	void ConvertZStencilState(D3D12_DEPTH_STENCIL_DESC &desc, const ZStencilState &zs);
	void ConvertRasterState(D3D12_RASTERIZER_DESC &desc, const RasterState &rs);
	void ConvertBlendState(D3D12_BLEND_DESC &desc, const BlendState &bs);

	void GeneratePipelineState(gs_graphics_pipeline &pipeline);

	void LoadGraphicsPipeline(gs_graphics_pipeline &new_pipeline);
	void LoadVertexBufferData();
	void LoadSamplerDescriptors();
	void LoadTextureDescriptors();

	std::vector<gs_command_context *> contextPool;
	std::queue<gs_command_context *> availableContexts;

	gs_command_context *AllocateContext();
	void FreeContext(gs_command_context *context);

	void CopyTex(ID3D12Resource *dst, uint32_t dst_x, uint32_t dst_y, gs_texture_t *src, uint32_t src_x,
		     uint32_t src_y, uint32_t src_w, uint32_t src_h);
	void UpdateViewProjMatrix();

	void FlushOutputViews();

	void RebuildDevice();

	bool HasBadNV12Output();
	gs_monitor_color_info GetMonitorColorInfo(HMONITOR hMonitor);

	gs_device(uint32_t adapterIdx);
	~gs_device();
};

extern "C" EXPORT int device_texture_acquire_sync(gs_texture_t *tex, uint64_t key, uint32_t ms);
