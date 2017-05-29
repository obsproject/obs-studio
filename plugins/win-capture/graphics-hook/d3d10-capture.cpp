#include <d3d10.h>
#include <dxgi.h>

#include "dxgi-helpers.hpp"
#include "graphics-hook.h"
#include "../funchook.h"

struct d3d10_data {
	ID3D10Device                   *device; /* do not release */
	uint32_t                       base_cx;
	uint32_t                       base_cy;
	uint32_t                       cx;
	uint32_t                       cy;
	DXGI_FORMAT                    format;
	bool                           using_shtex;
	bool                           using_scale;
	bool                           multisampled;

	ID3D10Texture2D                *scale_tex;
	ID3D10ShaderResourceView       *scale_resource;

	ID3D10VertexShader             *vertex_shader;
	ID3D10InputLayout              *vertex_layout;
	ID3D10PixelShader              *pixel_shader;

	ID3D10SamplerState             *sampler_state;
	ID3D10BlendState               *blend_state;
	ID3D10DepthStencilState        *zstencil_state;
	ID3D10RasterizerState          *raster_state;

	ID3D10Buffer                   *vertex_buffer;

	union {
		/* shared texture */
		struct {
			struct shtex_data      *shtex_info;
			ID3D10Texture2D        *texture;
			ID3D10RenderTargetView *render_target;
			HANDLE                 handle;
		};
		/* shared memory */
		struct {
			struct shmem_data      *shmem_info;
			ID3D10Texture2D        *copy_surfaces[NUM_BUFFERS];
			ID3D10Texture2D        *textures[NUM_BUFFERS];
			ID3D10RenderTargetView *render_targets[NUM_BUFFERS];
			bool                   texture_ready[NUM_BUFFERS];
			bool                   texture_mapped[NUM_BUFFERS];
			uint32_t               pitch;
			int                    cur_tex;
			int                    copy_wait;
		};
	};
};

static struct d3d10_data data = {};

void d3d10_free(void)
{
	if (data.scale_tex)
		data.scale_tex->Release();
	if (data.scale_resource)
		data.scale_resource->Release();
	if (data.vertex_shader)
		data.vertex_shader->Release();
	if (data.vertex_layout)
		data.vertex_layout->Release();
	if (data.pixel_shader)
		data.pixel_shader->Release();
	if (data.sampler_state)
		data.sampler_state->Release();
	if (data.blend_state)
		data.blend_state->Release();
	if (data.zstencil_state)
		data.zstencil_state->Release();
	if (data.raster_state)
		data.raster_state->Release();
	if (data.vertex_buffer)
		data.vertex_buffer->Release();

	capture_free();

	if (data.using_shtex) {
		if (data.texture)
			data.texture->Release();
		if (data.render_target)
			data.render_target->Release();
	} else {
		for (size_t i = 0; i < NUM_BUFFERS; i++) {
			if (data.copy_surfaces[i]) {
				if (data.texture_mapped[i])
					data.copy_surfaces[i]->Unmap(0);
				data.copy_surfaces[i]->Release();
			}
			if (data.textures[i])
				data.textures[i]->Release();
			if (data.render_targets[i])
				data.render_targets[i]->Release();
		}
	}

	memset(&data, 0, sizeof(data));

	hlog("----------------- d3d10 capture freed ----------------");
}

static bool create_d3d10_stage_surface(ID3D10Texture2D **tex)
{
	HRESULT hr;

	D3D10_TEXTURE2D_DESC desc      = {};
	desc.Width                     = data.cx;
	desc.Height                    = data.cy;
	desc.Format                    = data.format;
	desc.MipLevels                 = 1;
	desc.ArraySize                 = 1;
	desc.SampleDesc.Count          = 1;
	desc.Usage                     = D3D10_USAGE_STAGING;
	desc.CPUAccessFlags            = D3D10_CPU_ACCESS_READ;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		hlog_hr("create_d3d10_stage_surface: failed to create texture",
				hr);
		return false;
	}

	return true;
}

static bool create_d3d10_tex(uint32_t cx, uint32_t cy,
		ID3D10Texture2D **tex,
		ID3D10ShaderResourceView **resource,
		ID3D10RenderTargetView **render_target,
		HANDLE *handle)
{
	UINT flags = 0;
	UINT misc_flags = 0;
	HRESULT hr;

	if (!!resource)
		flags |= D3D10_BIND_SHADER_RESOURCE;
	if (!!render_target)
		flags |= D3D10_BIND_RENDER_TARGET;
	if (!!handle)
		misc_flags |= D3D10_RESOURCE_MISC_SHARED;

	D3D10_TEXTURE2D_DESC desc      = {};
	desc.Width                     = cx;
	desc.Height                    = cy;
	desc.MipLevels                 = 1;
	desc.ArraySize                 = 1;
	desc.Format                    = data.format;
	desc.BindFlags                 = flags;
	desc.SampleDesc.Count          = 1;
	desc.Usage                     = D3D10_USAGE_DEFAULT;
	desc.MiscFlags                 = misc_flags;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		hlog_hr("create_d3d10_tex: failed to create texture", hr);
		return false;
	}

	if (!!resource) {
		D3D10_SHADER_RESOURCE_VIEW_DESC res_desc = {};
		res_desc.Format = data.format;
		res_desc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
		res_desc.Texture2D.MipLevels = 1;

		hr = data.device->CreateShaderResourceView(*tex, &res_desc,
				resource);
		if (FAILED(hr)) {
			hlog_hr("create_d3d10_tex: failed to create resource "
			        "view", hr);
			return false;
		}
	}

	if (!!render_target) {
		hr = data.device->CreateRenderTargetView(*tex, nullptr,
				render_target);
		if (FAILED(hr)) {
			hlog_hr("create_d3d10_tex: failed to create render "
			        "target view", hr);
			return false;
		}
	}

	if (!!handle) {
		IDXGIResource *dxgi_res;
		hr = (*tex)->QueryInterface(__uuidof(IDXGIResource),
				(void**)&dxgi_res);
		if (FAILED(hr)) {
			hlog_hr("create_d3d10_tex: failed to query "
			        "IDXGIResource interface from texture", hr);
			return false;
		}

		hr = dxgi_res->GetSharedHandle(handle);
		dxgi_res->Release();
		if (FAILED(hr)) {
			hlog_hr("create_d3d10_tex: failed to get shared handle",
					hr);
			return false;
		}
	}

	return true;
}

static inline bool d3d10_init_format(IDXGISwapChain *swap, HWND &window)
{
	DXGI_SWAP_CHAIN_DESC desc;
	HRESULT hr;

	hr = swap->GetDesc(&desc);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_format: swap->GetDesc failed", hr);
		return false;
	}

	data.format = fix_dxgi_format(desc.BufferDesc.Format);
	data.multisampled = desc.SampleDesc.Count > 1;
	window = desc.OutputWindow;
	data.base_cx = desc.BufferDesc.Width;
	data.base_cy = desc.BufferDesc.Height;

	if (data.using_scale) {
		data.cx = global_hook_info->cx;
		data.cy = global_hook_info->cy;
	} else {
		data.cx = desc.BufferDesc.Width;
		data.cy = desc.BufferDesc.Height;
	}
	return true;
}

static inline bool d3d10_init_vertex_shader(void)
{
	D3D10_INPUT_ELEMENT_DESC desc[2];
	uint8_t *vs_data;
	size_t size;
	HRESULT hr;

	vs_data = get_d3d1x_vertex_shader(&size);

	hr = data.device->CreateVertexShader(vs_data, size,
			&data.vertex_shader);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_vertex_shader: failed to create shader",
				hr);
		return false;
	}

	desc[0].SemanticName = "SV_Position";
	desc[0].SemanticIndex = 0;
	desc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc[0].InputSlot = 0;
	desc[0].AlignedByteOffset = 0;
	desc[0].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	desc[0].InstanceDataStepRate = 0;

	desc[1].SemanticName = "TEXCOORD";
	desc[1].SemanticIndex = 0;
	desc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	desc[1].InputSlot = 0;
	desc[1].AlignedByteOffset = D3D10_APPEND_ALIGNED_ELEMENT;
	desc[1].InputSlotClass = D3D10_INPUT_PER_VERTEX_DATA;
	desc[1].InstanceDataStepRate = 0;

	hr = data.device->CreateInputLayout(desc, 2, vs_data, size,
			&data.vertex_layout);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_vertex_shader: failed to create layout",
				hr);
		return false;
	}

	return true;
}

static inline bool d3d10_init_pixel_shader(void)
{
	uint8_t *ps_data;
	size_t size;
	HRESULT hr;

	ps_data = get_d3d1x_pixel_shader(&size);

	hr = data.device->CreatePixelShader(ps_data, size, &data.pixel_shader);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_pixel_shader: failed to create shader", hr);
		return false;
	}

	return true;
}

static inline bool d3d10_init_sampler_state(void)
{
	HRESULT hr;

	D3D10_SAMPLER_DESC desc        = {};
	desc.Filter                    = D3D10_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU                  = D3D10_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV                  = D3D10_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW                  = D3D10_TEXTURE_ADDRESS_CLAMP;

	hr = data.device->CreateSamplerState(&desc, &data.sampler_state);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_sampler_state: failed to create sampler "
		        "state", hr);
		return false;
	}

	return true;
}

static inline bool d3d10_init_blend_state(void)
{
	D3D10_BLEND_DESC desc = {};
	HRESULT hr;

	for (size_t i = 0; i < 8; i++)
		desc.RenderTargetWriteMask[i] = D3D10_COLOR_WRITE_ENABLE_ALL;

	hr = data.device->CreateBlendState(&desc, &data.blend_state);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_blend_state: failed to create blend state",
				hr);
		return false;
	}

	return true;
}

static inline bool d3d10_init_zstencil_state(void)
{
	D3D10_DEPTH_STENCIL_DESC desc = {}; /* defaults all to off */
	HRESULT hr;

	hr = data.device->CreateDepthStencilState(&desc, &data.zstencil_state);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_zstencil_state: failed to create "
		        "zstencil state", hr);
		return false;
	}

	return true;
}

static inline bool d3d10_init_raster_state(void)
{
	D3D10_RASTERIZER_DESC desc = {};
	HRESULT hr;

	desc.FillMode = D3D10_FILL_SOLID;
	desc.CullMode = D3D10_CULL_NONE;

	hr = data.device->CreateRasterizerState(&desc, &data.raster_state);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_raster_state: failed to create raster "
		        "state", hr);
		return false;
	}

	return true;
}

#define NUM_VERTS 4

static inline bool d3d10_init_vertex_buffer(void)
{
	HRESULT hr;
	const vertex verts[NUM_VERTS] = {
		{{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}
	};

	D3D10_BUFFER_DESC desc;
	desc.ByteWidth                 = sizeof(vertex) * NUM_VERTS;
	desc.Usage                     = D3D10_USAGE_DEFAULT;
	desc.BindFlags                 = D3D10_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags            = 0;
	desc.MiscFlags                 = 0;

	D3D10_SUBRESOURCE_DATA srd     = {};
	srd.pSysMem                    = (const void*)verts;

	hr = data.device->CreateBuffer(&desc, &srd, &data.vertex_buffer);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init_vertex_buffer: failed to create vertex "
		        "buffer", hr);
		return false;
	}

	return true;
}

static bool d3d10_init_scaling(void)
{
	bool success;

	success = create_d3d10_tex(data.base_cx, data.base_cy,
			&data.scale_tex, &data.scale_resource, nullptr,
			nullptr);
	if (!success) {
		hlog("d3d10_init_scaling: failed to create scale texture");
		return false;
	}

	if (!d3d10_init_vertex_shader()) {
		return false;
	}
	if (!d3d10_init_pixel_shader()) {
		return false;
	}
	if (!d3d10_init_sampler_state()) {
		return false;
	}
	if (!d3d10_init_blend_state()) {
		return false;
	}
	if (!d3d10_init_zstencil_state()) {
		return false;
	}
	if (!d3d10_init_raster_state()) {
		return false;
	}
	if (!d3d10_init_vertex_buffer()) {
		return false;
	}

	return true;
}

static bool d3d10_shmem_init_buffers(size_t idx)
{
	bool success;

	success = create_d3d10_stage_surface(&data.copy_surfaces[idx]);
	if (!success) {
		hlog("d3d10_shmem_init_buffers: failed to create copy surface");
		return false;
	}

	if (idx == 0) {
		D3D10_MAPPED_TEXTURE2D map = {};
		HRESULT hr;

		hr = data.copy_surfaces[idx]->Map(0, D3D10_MAP_READ, 0, &map);
		if (FAILED(hr)) {
			hlog_hr("d3d10_shmem_init_buffers: failed to get "
			        "pitch", hr);
			return false;
		}

		data.pitch = map.RowPitch;
		data.copy_surfaces[idx]->Unmap(0);
	}

	success = create_d3d10_tex(data.cx, data.cy, &data.textures[idx],
			nullptr, &data.render_targets[idx], nullptr);
	if (!success) {
		hlog("d3d10_shmem_init_buffers: failed to create texture");
		return false;
	}

	return true;
}

static bool d3d10_shmem_init(HWND window)
{
	data.using_shtex = false;

	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		if (!d3d10_shmem_init_buffers(i)) {
			return false;
		}
	}
	if (!capture_init_shmem(&data.shmem_info, window,
				data.base_cx, data.base_cy, data.cx, data.cy,
				data.pitch, data.format, false)) {
		return false;
	}

	hlog("d3d10 memory capture successful");
	return true;
}

static bool d3d10_shtex_init(HWND window)
{
	ID3D10ShaderResourceView *resource = nullptr;
	bool success;

	data.using_shtex = true;

	success = create_d3d10_tex(data.cx, data.cy, &data.texture, &resource,
			&data.render_target, &data.handle);
	if (resource)
		resource->Release();

	if (!success) {
		hlog("d3d10_shtex_init: failed to create texture");
		return false;
	}
	if (!capture_init_shtex(&data.shtex_info, window,
				data.base_cx, data.base_cy, data.cx, data.cy,
				data.format, false, (uintptr_t)data.handle)) {
		return false;
	}

	hlog("d3d10 shared texture capture successful");
	return true;
}

static void d3d10_init(IDXGISwapChain *swap)
{
	bool success = true;
	HWND window;
	HRESULT hr;

	data.using_scale = global_hook_info->use_scale;

	hr = swap->GetDevice(__uuidof(ID3D10Device), (void**)&data.device);
	if (FAILED(hr)) {
		hlog_hr("d3d10_init: failed to get device from swap", hr);
		return;
	}

	/* remove the unneeded extra reference */
	data.device->Release();

	if (!d3d10_init_format(swap, window)) {
		return;
	}
	if (data.using_scale && !d3d10_init_scaling()) {
		hlog("d3d10_init: failed to initialize scaling");
		success = false;
	}
	if (success) {
		if (global_hook_info->force_shmem) {
			success = d3d10_shmem_init(window);
		} else {
			success = d3d10_shtex_init(window);
		}
	}

	if (!success)
		d3d10_free();
}

#define MAX_RENDER_TARGETS             D3D10_SIMULTANEOUS_RENDER_TARGET_COUNT
#define MAX_SO_TARGETS                 4

struct d3d10_state {
	ID3D10GeometryShader           *geom_shader;
	ID3D10InputLayout              *vertex_layout;
	D3D10_PRIMITIVE_TOPOLOGY       topology;
	ID3D10Buffer                   *vertex_buffer;
	UINT                           vb_stride;
	UINT                           vb_offset;
	ID3D10BlendState               *blend_state;
	float                          blend_factor[4];
	UINT                           sample_mask;
	ID3D10DepthStencilState        *zstencil_state;
	UINT                           zstencil_ref;
	ID3D10RenderTargetView         *render_targets[MAX_RENDER_TARGETS];
	ID3D10DepthStencilView         *zstencil_view;
	ID3D10SamplerState             *sampler_state;
	ID3D10PixelShader              *pixel_shader;
	ID3D10ShaderResourceView       *resource;
	ID3D10RasterizerState          *raster_state;
	UINT                           num_viewports;
	D3D10_VIEWPORT                 *viewports;
	ID3D10Buffer                   *stream_output_targets[MAX_SO_TARGETS];
	UINT                           so_offsets[MAX_SO_TARGETS];
	ID3D10VertexShader             *vertex_shader;
};

static inline void d3d10_save_state(struct d3d10_state *state)
{
	data.device->GSGetShader(&state->geom_shader);
	data.device->IAGetInputLayout(&state->vertex_layout);
	data.device->IAGetPrimitiveTopology(&state->topology);
	data.device->IAGetVertexBuffers(0, 1, &state->vertex_buffer,
			&state->vb_stride, &state->vb_offset);
	data.device->OMGetBlendState(&state->blend_state, state->blend_factor,
			&state->sample_mask);
	data.device->OMGetDepthStencilState(&state->zstencil_state,
			&state->zstencil_ref);
	data.device->OMGetRenderTargets(MAX_RENDER_TARGETS,
			state->render_targets, &state->zstencil_view);
	data.device->PSGetSamplers(0, 1, &state->sampler_state);
	data.device->PSGetShader(&state->pixel_shader);
	data.device->PSGetShaderResources(0, 1, &state->resource);
	data.device->RSGetState(&state->raster_state);
	data.device->RSGetViewports(&state->num_viewports, nullptr);
	if (state->num_viewports) {
		state->viewports = (D3D10_VIEWPORT*)malloc(
				sizeof(D3D10_VIEWPORT) * state->num_viewports);
		data.device->RSGetViewports(&state->num_viewports,
				state->viewports);
	}
	data.device->SOGetTargets(MAX_SO_TARGETS, state->stream_output_targets,
			state->so_offsets);
	data.device->VSGetShader(&state->vertex_shader);
}

static inline void safe_release(IUnknown *p)
{
	if (p) p->Release();
}

static inline void d3d10_restore_state(struct d3d10_state *state)
{
	data.device->GSSetShader(state->geom_shader);
	data.device->IASetInputLayout(state->vertex_layout);
	data.device->IASetPrimitiveTopology(state->topology);
	data.device->IASetVertexBuffers(0, 1, &state->vertex_buffer,
			&state->vb_stride, &state->vb_offset);
	data.device->OMSetBlendState(state->blend_state, state->blend_factor,
			state->sample_mask);
	data.device->OMSetDepthStencilState(state->zstencil_state,
			state->zstencil_ref);
	data.device->OMSetRenderTargets(MAX_RENDER_TARGETS,
			state->render_targets, state->zstencil_view);
	data.device->PSSetSamplers(0, 1, &state->sampler_state);
	data.device->PSSetShader(state->pixel_shader);
	data.device->PSSetShaderResources(0, 1, &state->resource);
	data.device->RSSetState(state->raster_state);
	data.device->RSSetViewports(state->num_viewports, state->viewports);
	data.device->SOSetTargets(MAX_SO_TARGETS, state->stream_output_targets,
			state->so_offsets);
	data.device->VSSetShader(state->vertex_shader);
	safe_release(state->geom_shader);
	safe_release(state->vertex_layout);
	safe_release(state->vertex_buffer);
	safe_release(state->blend_state);
	safe_release(state->zstencil_state);
	for (size_t i = 0; i < MAX_RENDER_TARGETS; i++)
		safe_release(state->render_targets[i]);
	safe_release(state->zstencil_view);
	safe_release(state->sampler_state);
	safe_release(state->pixel_shader);
	safe_release(state->resource);
	safe_release(state->raster_state);
	for (size_t i = 0; i < MAX_SO_TARGETS; i++)
		safe_release(state->stream_output_targets[i]);
	safe_release(state->vertex_shader);
	free(state->viewports);
	memset(state, 0, sizeof(*state));
}

static inline void d3d10_setup_pipeline(ID3D10RenderTargetView *target,
		ID3D10ShaderResourceView *resource)
{
	D3D10_VIEWPORT viewport = {0};
	const float factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	void *emptyptr = nullptr;
	UINT stride = sizeof(vertex);
	UINT zero = 0;

	viewport.Width = data.cx;
	viewport.Height = data.cy;
	viewport.MaxDepth = 1.0f;

	data.device->GSSetShader(nullptr);
	data.device->IASetInputLayout(data.vertex_layout);
	data.device->IASetPrimitiveTopology(
			D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	data.device->IASetVertexBuffers(0, 1, &data.vertex_buffer, &stride,
			&zero);
	data.device->OMSetBlendState(data.blend_state, factor, 0xFFFFFFFF);
	data.device->OMSetDepthStencilState(data.zstencil_state, 0);
	data.device->OMSetRenderTargets(1, &target, nullptr);
	data.device->PSSetSamplers(0, 1, &data.sampler_state);
	data.device->PSSetShader(data.pixel_shader);
	data.device->PSSetShaderResources(0, 1, &resource);
	data.device->RSSetState(data.raster_state);
	data.device->RSSetViewports(1, &viewport);
	data.device->SOSetTargets(1, (ID3D10Buffer**)&emptyptr, &zero);
	data.device->VSSetShader(data.vertex_shader);
}

static inline void d3d10_scale_texture(ID3D10RenderTargetView *target,
		ID3D10ShaderResourceView *resource)
{
	struct d3d10_state old_state = {};

	d3d10_save_state(&old_state);
	d3d10_setup_pipeline(target, resource);
	data.device->Draw(4, 0);
	d3d10_restore_state(&old_state);
}

static inline void d3d10_copy_texture(ID3D10Resource *dst, ID3D10Resource *src)
{
	if (data.multisampled) {
		data.device->ResolveSubresource(dst, 0, src, 0, data.format);
	} else {
		data.device->CopyResource(dst, src);
	}
}

static inline void d3d10_shtex_capture(ID3D10Resource *backbuffer)
{
	if (data.using_scale) {
		d3d10_copy_texture(data.scale_tex, backbuffer);
		d3d10_scale_texture(data.render_target, data.scale_resource);
	} else {
		d3d10_copy_texture(data.texture, backbuffer);
	}
}

static inline void d3d10_shmem_queue_copy()
{
	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		D3D10_MAPPED_TEXTURE2D map;
		HRESULT hr;

		if (data.texture_ready[i]) {
			data.texture_ready[i] = false;

			hr = data.copy_surfaces[i]->Map(0, D3D10_MAP_READ,
					0, &map);
			if (SUCCEEDED(hr)) {
				data.texture_mapped[i] = true;
				shmem_copy_data(i, map.pData);
			}
			break;
		}
	}
}

static inline void d3d10_shmem_capture(ID3D10Resource *backbuffer)
{
	int next_tex;

	d3d10_shmem_queue_copy();

	next_tex = (data.cur_tex == NUM_BUFFERS - 1) ?  0 : data.cur_tex + 1;

	if (data.using_scale) {
		d3d10_copy_texture(data.scale_tex, backbuffer);
		d3d10_scale_texture(data.render_targets[data.cur_tex],
				data.scale_resource);
	} else {
		d3d10_copy_texture(data.textures[data.cur_tex], backbuffer);
	}

	if (data.copy_wait < NUM_BUFFERS - 1) {
		data.copy_wait++;
	} else {
		ID3D10Texture2D *src = data.textures[next_tex];
		ID3D10Texture2D *dst = data.copy_surfaces[next_tex];

		if (shmem_texture_data_lock(next_tex)) {
			dst->Unmap(0);
			data.texture_mapped[next_tex] = false;
			shmem_texture_data_unlock(next_tex);
		}

		d3d10_copy_texture(dst, src);
		data.texture_ready[next_tex] = true;
	}

	data.cur_tex = next_tex;
}

void d3d10_capture(void *swap_ptr, void *backbuffer_ptr, bool)
{
	IDXGIResource *dxgi_backbuffer = (IDXGIResource*)backbuffer_ptr;
	IDXGISwapChain *swap = (IDXGISwapChain*)swap_ptr;

	HRESULT hr;
	if (capture_should_stop()) {
		d3d10_free();
	}
	if (capture_should_init()) {
		d3d10_init(swap);
	}
	if (capture_ready()) {
		ID3D10Resource *backbuffer;

		hr = dxgi_backbuffer->QueryInterface(__uuidof(ID3D10Resource),
				(void**)&backbuffer);
		if (FAILED(hr)) {
			hlog_hr("d3d10_shtex_capture: failed to get "
			        "backbuffer", hr);
			return;
		}

		if (data.using_shtex)
			d3d10_shtex_capture(backbuffer);
		else
			d3d10_shmem_capture(backbuffer);

		backbuffer->Release();
	}
}
