#include <d3d11.h>
#include <dxgi.h>

#include "dxgi-helpers.hpp"
#include "graphics-hook.h"
#include "../funchook.h"

struct d3d11_data {
	ID3D11Device                   *device; /* do not release */
	ID3D11DeviceContext            *context; /* do not release */
	uint32_t                       base_cx;
	uint32_t                       base_cy;
	uint32_t                       cx;
	uint32_t                       cy;
	DXGI_FORMAT                    format;
	bool                           using_shtex;
	bool                           using_scale;
	bool                           multisampled;

	ID3D11Texture2D                *scale_tex;
	ID3D11ShaderResourceView       *scale_resource;

	ID3D11VertexShader             *vertex_shader;
	ID3D11InputLayout              *vertex_layout;
	ID3D11PixelShader              *pixel_shader;

	ID3D11SamplerState             *sampler_state;
	ID3D11BlendState               *blend_state;
	ID3D11DepthStencilState        *zstencil_state;
	ID3D11RasterizerState          *raster_state;

	ID3D11Buffer                   *vertex_buffer;

	union {
		/* shared texture */
		struct {
			struct shtex_data      *shtex_info;
			ID3D11Texture2D        *texture;
			ID3D11RenderTargetView *render_target;
			HANDLE                 handle;
		};
		/* shared memory */
		struct {
			ID3D11Texture2D        *copy_surfaces[NUM_BUFFERS];
			ID3D11Texture2D        *textures[NUM_BUFFERS];
			ID3D11RenderTargetView *render_targets[NUM_BUFFERS];
			bool                   texture_ready[NUM_BUFFERS];
			bool                   texture_mapped[NUM_BUFFERS];
			uint32_t               pitch;
			struct shmem_data      *shmem_info;
			int                    cur_tex;
			int                    copy_wait;
		};
	};
};

static struct d3d11_data data = {};

void d3d11_free(void)
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
					data.context->Unmap(
							data.copy_surfaces[i],
							0);
				data.copy_surfaces[i]->Release();
			}
			if (data.textures[i])
				data.textures[i]->Release();
			if (data.render_targets[i])
				data.render_targets[i]->Release();
		}
	}

	memset(&data, 0, sizeof(data));

	hlog("----------------- d3d11 capture freed ----------------");
}

static bool create_d3d11_stage_surface(ID3D11Texture2D **tex)
{
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc      = {};
	desc.Width                     = data.cx;
	desc.Height                    = data.cy;
	desc.Format                    = data.format;
	desc.MipLevels                 = 1;
	desc.ArraySize                 = 1;
	desc.SampleDesc.Count          = 1;
	desc.Usage                     = D3D11_USAGE_STAGING;
	desc.CPUAccessFlags            = D3D11_CPU_ACCESS_READ;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		hlog_hr("create_d3d11_stage_surface: failed to create texture",
				hr);
		return false;
	}

	return true;
}

static bool create_d3d11_tex(uint32_t cx, uint32_t cy,
		ID3D11Texture2D **tex,
		ID3D11ShaderResourceView **resource,
		ID3D11RenderTargetView **render_target,
		HANDLE *handle)
{
	UINT flags = 0;
	UINT misc_flags = 0;
	HRESULT hr;

	if (!!resource)
		flags |= D3D11_BIND_SHADER_RESOURCE;
	if (!!render_target)
		flags |= D3D11_BIND_RENDER_TARGET;
	if (!!handle)
		misc_flags |= D3D11_RESOURCE_MISC_SHARED;

	D3D11_TEXTURE2D_DESC desc      = {};
	desc.Width                     = cx;
	desc.Height                    = cy;
	desc.MipLevels                 = 1;
	desc.ArraySize                 = 1;
	desc.Format                    = data.format;
	desc.BindFlags                 = flags;
	desc.SampleDesc.Count          = 1;
	desc.Usage                     = D3D11_USAGE_DEFAULT;
	desc.MiscFlags                 = misc_flags;

	hr = data.device->CreateTexture2D(&desc, nullptr, tex);
	if (FAILED(hr)) {
		hlog_hr("create_d3d11_tex: failed to create texture", hr);
		return false;
	}

	if (!!resource) {
		D3D11_SHADER_RESOURCE_VIEW_DESC res_desc = {};
		res_desc.Format = data.format;
		res_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		res_desc.Texture2D.MipLevels = 1;

		hr = data.device->CreateShaderResourceView(*tex, &res_desc,
				resource);
		if (FAILED(hr)) {
			hlog_hr("create_d3d11_tex: failed to create resource "
			        "view", hr);
			return false;
		}
	}

	if (!!render_target) {
		hr = data.device->CreateRenderTargetView(*tex, nullptr,
				render_target);
		if (FAILED(hr)) {
			hlog_hr("create_d3d11_tex: failed to create render "
			        "target view", hr);
			return false;
		}
	}

	if (!!handle) {
		IDXGIResource *dxgi_res;
		hr = (*tex)->QueryInterface(__uuidof(IDXGIResource),
				(void**)&dxgi_res);
		if (FAILED(hr)) {
			hlog_hr("create_d3d11_tex: failed to query "
			        "IDXGIResource interface from texture", hr);
			return false;
		}

		hr = dxgi_res->GetSharedHandle(handle);
		dxgi_res->Release();
		if (FAILED(hr)) {
			hlog_hr("create_d3d11_tex: failed to get shared handle",
					hr);
			return false;
		}
	}

	return true;
}

static inline bool d3d11_init_format(IDXGISwapChain *swap, HWND &window)
{
	DXGI_SWAP_CHAIN_DESC desc;
	HRESULT hr;

	hr = swap->GetDesc(&desc);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_format: swap->GetDesc failed", hr);
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

static inline bool d3d11_init_vertex_shader(void)
{
	D3D11_INPUT_ELEMENT_DESC desc[2];
	uint8_t *vs_data;
	size_t size;
	HRESULT hr;

	vs_data = get_d3d1x_vertex_shader(&size);

	hr = data.device->CreateVertexShader(vs_data, size, nullptr,
			&data.vertex_shader);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_vertex_shader: failed to create shader",
				hr);
		return false;
	}

	desc[0].SemanticName = "SV_Position";
	desc[0].SemanticIndex = 0;
	desc[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	desc[0].InputSlot = 0;
	desc[0].AlignedByteOffset = 0;
	desc[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	desc[0].InstanceDataStepRate = 0;

	desc[1].SemanticName = "TEXCOORD";
	desc[1].SemanticIndex = 0;
	desc[1].Format = DXGI_FORMAT_R32G32_FLOAT;
	desc[1].InputSlot = 0;
	desc[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
	desc[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	desc[1].InstanceDataStepRate = 0;

	hr = data.device->CreateInputLayout(desc, 2, vs_data, size,
			&data.vertex_layout);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_vertex_shader: failed to create layout",
				hr);
		return false;
	}

	return true;
}

static inline bool d3d11_init_pixel_shader(void)
{
	uint8_t *ps_data;
	size_t size;
	HRESULT hr;

	ps_data = get_d3d1x_pixel_shader(&size);

	hr = data.device->CreatePixelShader(ps_data, size, nullptr,
			&data.pixel_shader);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_pixel_shader: failed to create shader", hr);
		return false;
	}

	return true;
}

static inline bool d3d11_init_sampler_state(void)
{
	HRESULT hr;

	D3D11_SAMPLER_DESC desc        = {};
	desc.Filter                    = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	desc.AddressU                  = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressV                  = D3D11_TEXTURE_ADDRESS_CLAMP;
	desc.AddressW                  = D3D11_TEXTURE_ADDRESS_CLAMP;

	hr = data.device->CreateSamplerState(&desc, &data.sampler_state);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_sampler_state: failed to create sampler "
		        "state", hr);
		return false;
	}

	return true;
}

static inline bool d3d11_init_blend_state(void)
{
	D3D11_BLEND_DESC desc = {};
	HRESULT hr;

	for (size_t i = 0; i < 8; i++)
		desc.RenderTarget[i].RenderTargetWriteMask =
			D3D11_COLOR_WRITE_ENABLE_ALL;

	hr = data.device->CreateBlendState(&desc, &data.blend_state);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_blend_state: failed to create blend state",
				hr);
		return false;
	}

	return true;
}

static inline bool d3d11_init_zstencil_state(void)
{
	D3D11_DEPTH_STENCIL_DESC desc = {}; /* defaults all to off */
	HRESULT hr;

	hr = data.device->CreateDepthStencilState(&desc, &data.zstencil_state);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_zstencil_state: failed to create "
		        "zstencil state", hr);
		return false;
	}

	return true;
}

static inline bool d3d11_init_raster_state(void)
{
	D3D11_RASTERIZER_DESC desc = {};
	HRESULT hr;

	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_NONE;

	hr = data.device->CreateRasterizerState(&desc, &data.raster_state);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_raster_state: failed to create raster "
		        "state", hr);
		return false;
	}

	return true;
}

#define NUM_VERTS 4

static inline bool d3d11_init_vertex_buffer(void)
{
	HRESULT hr;
	const vertex verts[NUM_VERTS] = {
		{{-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
		{{-1.0f, -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		{{ 1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
		{{ 1.0f, -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}}
	};

	D3D11_BUFFER_DESC desc;
	desc.ByteWidth                 = sizeof(vertex) * NUM_VERTS;
	desc.Usage                     = D3D11_USAGE_DEFAULT;
	desc.BindFlags                 = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags            = 0;
	desc.MiscFlags                 = 0;

	D3D11_SUBRESOURCE_DATA srd     = {};
	srd.pSysMem                    = (const void*)verts;

	hr = data.device->CreateBuffer(&desc, &srd, &data.vertex_buffer);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init_vertex_buffer: failed to create vertex "
		        "buffer", hr);
		return false;
	}

	return true;
}

static bool d3d11_init_scaling(void)
{
	bool success;

	success = create_d3d11_tex(data.base_cx, data.base_cy,
			&data.scale_tex, &data.scale_resource, nullptr,
			nullptr);
	if (!success) {
		hlog("d3d11_init_scaling: failed to create scale texture");
		return false;
	}

	if (!d3d11_init_vertex_shader()) {
		return false;
	}
	if (!d3d11_init_pixel_shader()) {
		return false;
	}
	if (!d3d11_init_sampler_state()) {
		return false;
	}
	if (!d3d11_init_blend_state()) {
		return false;
	}
	if (!d3d11_init_zstencil_state()) {
		return false;
	}
	if (!d3d11_init_raster_state()) {
		return false;
	}
	if (!d3d11_init_vertex_buffer()) {
		return false;
	}

	return true;
}

static bool d3d11_shmem_init_buffers(size_t idx)
{
	bool success;

	success = create_d3d11_stage_surface(&data.copy_surfaces[idx]);
	if (!success) {
		hlog("d3d11_shmem_init_buffers: failed to create copy surface");
		return false;
	}

	if (idx == 0) {
		D3D11_MAPPED_SUBRESOURCE map = {};
		HRESULT hr;

		hr = data.context->Map(data.copy_surfaces[idx], 0,
				D3D11_MAP_READ, 0, &map);
		if (FAILED(hr)) {
			hlog_hr("d3d11_shmem_init_buffers: failed to get "
			        "pitch", hr);
			return false;
		}

		data.pitch = map.RowPitch;
		data.context->Unmap(data.copy_surfaces[idx], 0);
	}

	success = create_d3d11_tex(data.cx, data.cy, &data.textures[idx],
			nullptr, &data.render_targets[idx], nullptr);
	if (!success) {
		hlog("d3d11_shmem_init_buffers: failed to create texture");
		return false;
	}

	return true;
}

static bool d3d11_shmem_init(HWND window)
{
	data.using_shtex = false;

	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		if (!d3d11_shmem_init_buffers(i)) {
			return false;
		}
	}
	if (!capture_init_shmem(&data.shmem_info, window,
				data.base_cx, data.base_cy, data.cx, data.cy,
				data.pitch, data.format, false)) {
		return false;
	}

	hlog("d3d11 memory capture successful");
	return true;
}

static bool d3d11_shtex_init(HWND window)
{
	ID3D11ShaderResourceView *resource = nullptr;
	bool success;

	data.using_shtex = true;

	success = create_d3d11_tex(data.cx, data.cy, &data.texture, &resource,
			&data.render_target, &data.handle);
	if (resource)
		resource->Release();

	if (!success) {
		hlog("d3d11_shtex_init: failed to create texture");
		return false;
	}
	if (!capture_init_shtex(&data.shtex_info, window,
				data.base_cx, data.base_cy, data.cx, data.cy,
				data.format, false, (uintptr_t)data.handle)) {
		return false;
	}

	hlog("d3d11 shared texture capture successful");
	return true;
}

static void d3d11_init(IDXGISwapChain *swap)
{
	bool success = true;
	HWND window;
	HRESULT hr;

	data.using_scale = global_hook_info->use_scale;

	hr = swap->GetDevice(__uuidof(ID3D11Device), (void**)&data.device);
	if (FAILED(hr)) {
		hlog_hr("d3d11_init: failed to get device from swap", hr);
		return;
	}

	data.device->Release();

	data.device->GetImmediateContext(&data.context);
	data.context->Release();

	if (!d3d11_init_format(swap, window)) {
		return;
	}
	if (data.using_scale && !d3d11_init_scaling()) {
		hlog("d3d11_init: failed to initialize scaling");
		success = false;
	}
	if (success) {
		if (global_hook_info->force_shmem) {
			success = d3d11_shmem_init(window);
		} else {
			success = d3d11_shtex_init(window);
		}
	}

	if (!success)
		d3d11_free();
}

#define MAX_RENDER_TARGETS             D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT
#define MAX_SO_TARGETS                 4
#define MAX_CLASS_INSTS                256

struct d3d11_state {
	ID3D11GeometryShader           *geom_shader;
	ID3D11InputLayout              *vertex_layout;
	D3D11_PRIMITIVE_TOPOLOGY       topology;
	ID3D11Buffer                   *vertex_buffer;
	UINT                           vb_stride;
	UINT                           vb_offset;
	ID3D11BlendState               *blend_state;
	float                          blend_factor[4];
	UINT                           sample_mask;
	ID3D11DepthStencilState        *zstencil_state;
	UINT                           zstencil_ref;
	ID3D11RenderTargetView         *render_targets[MAX_RENDER_TARGETS];
	ID3D11DepthStencilView         *zstencil_view;
	ID3D11SamplerState             *sampler_state;
	ID3D11PixelShader              *pixel_shader;
	ID3D11ShaderResourceView       *resource;
	ID3D11RasterizerState          *raster_state;
	UINT                           num_viewports;
	D3D11_VIEWPORT                 *viewports;
	ID3D11Buffer                   *stream_output_targets[MAX_SO_TARGETS];
	ID3D11VertexShader             *vertex_shader;
	ID3D11ClassInstance            *gs_class_instances[MAX_CLASS_INSTS];
	ID3D11ClassInstance            *ps_class_instances[MAX_CLASS_INSTS];
	ID3D11ClassInstance            *vs_class_instances[MAX_CLASS_INSTS];
	UINT                           gs_class_inst_count;
	UINT                           ps_class_inst_count;
	UINT                           vs_class_inst_count;
};

static inline void d3d11_save_state(struct d3d11_state *state)
{
	state->gs_class_inst_count = MAX_CLASS_INSTS;
	state->ps_class_inst_count = MAX_CLASS_INSTS;
	state->vs_class_inst_count = MAX_CLASS_INSTS;

	data.context->GSGetShader(&state->geom_shader,
			state->gs_class_instances,
			&state->gs_class_inst_count);
	data.context->IAGetInputLayout(&state->vertex_layout);
	data.context->IAGetPrimitiveTopology(&state->topology);
	data.context->IAGetVertexBuffers(0, 1, &state->vertex_buffer,
			&state->vb_stride, &state->vb_offset);
	data.context->OMGetBlendState(&state->blend_state, state->blend_factor,
			&state->sample_mask);
	data.context->OMGetDepthStencilState(&state->zstencil_state,
			&state->zstencil_ref);
	data.context->OMGetRenderTargets(MAX_RENDER_TARGETS,
			state->render_targets, &state->zstencil_view);
	data.context->PSGetSamplers(0, 1, &state->sampler_state);
	data.context->PSGetShader(&state->pixel_shader,
			state->ps_class_instances,
			&state->ps_class_inst_count);
	data.context->PSGetShaderResources(0, 1, &state->resource);
	data.context->RSGetState(&state->raster_state);
	data.context->RSGetViewports(&state->num_viewports, nullptr);
	if (state->num_viewports) {
		state->viewports = (D3D11_VIEWPORT*)malloc(
				sizeof(D3D11_VIEWPORT) * state->num_viewports);
		data.context->RSGetViewports(&state->num_viewports,
				state->viewports);
	}
	data.context->SOGetTargets(MAX_SO_TARGETS,
			state->stream_output_targets);
	data.context->VSGetShader(&state->vertex_shader,
			state->vs_class_instances,
			&state->vs_class_inst_count);
}

static inline void safe_release(IUnknown *p)
{
	if (p) p->Release();
}

#define SO_APPEND ((UINT)-1)

static inline void d3d11_restore_state(struct d3d11_state *state)
{
	UINT so_offsets[MAX_SO_TARGETS] =
		{SO_APPEND, SO_APPEND, SO_APPEND, SO_APPEND};

	data.context->GSSetShader(state->geom_shader,
			state->gs_class_instances,
			state->gs_class_inst_count);
	data.context->IASetInputLayout(state->vertex_layout);
	data.context->IASetPrimitiveTopology(state->topology);
	data.context->IASetVertexBuffers(0, 1, &state->vertex_buffer,
			&state->vb_stride, &state->vb_offset);
	data.context->OMSetBlendState(state->blend_state, state->blend_factor,
			state->sample_mask);
	data.context->OMSetDepthStencilState(state->zstencil_state,
			state->zstencil_ref);
	data.context->OMSetRenderTargets(MAX_RENDER_TARGETS,
			state->render_targets,
			state->zstencil_view);
	data.context->PSSetSamplers(0, 1, &state->sampler_state);
	data.context->PSSetShader(state->pixel_shader,
			state->ps_class_instances,
			state->ps_class_inst_count);
	data.context->PSSetShaderResources(0, 1, &state->resource);
	data.context->RSSetState(state->raster_state);
	data.context->RSSetViewports(state->num_viewports, state->viewports);
	data.context->SOSetTargets(MAX_SO_TARGETS,
			state->stream_output_targets, so_offsets);
	data.context->VSSetShader(state->vertex_shader,
			state->vs_class_instances,
			state->vs_class_inst_count);
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
	for (size_t i = 0; i < state->gs_class_inst_count; i++)
		state->gs_class_instances[i]->Release();
	for (size_t i = 0; i < state->ps_class_inst_count; i++)
		state->ps_class_instances[i]->Release();
	for (size_t i = 0; i < state->vs_class_inst_count; i++)
		state->vs_class_instances[i]->Release();
	free(state->viewports);
	memset(state, 0, sizeof(*state));
}

static inline void d3d11_setup_pipeline(ID3D11RenderTargetView *target,
		ID3D11ShaderResourceView *resource)
{
	const float factor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
	D3D11_VIEWPORT viewport = {0};
	UINT stride = sizeof(vertex);
	void *emptyptr = nullptr;
	UINT zero = 0;

	viewport.Width = (float)data.cx;
	viewport.Height = (float)data.cy;
	viewport.MaxDepth = 1.0f;

	data.context->GSSetShader(nullptr, nullptr, 0);
	data.context->IASetInputLayout(data.vertex_layout);
	data.context->IASetPrimitiveTopology(
			D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	data.context->IASetVertexBuffers(0, 1, &data.vertex_buffer, &stride,
			&zero);
	data.context->OMSetBlendState(data.blend_state, factor, 0xFFFFFFFF);
	data.context->OMSetDepthStencilState(data.zstencil_state, 0);
	data.context->OMSetRenderTargets(1, &target, nullptr);
	data.context->PSSetSamplers(0, 1, &data.sampler_state);
	data.context->PSSetShader(data.pixel_shader, nullptr, 0);
	data.context->PSSetShaderResources(0, 1, &resource);
	data.context->RSSetState(data.raster_state);
	data.context->RSSetViewports(1, &viewport);
	data.context->SOSetTargets(1, (ID3D11Buffer**)&emptyptr, &zero);
	data.context->VSSetShader(data.vertex_shader, nullptr, 0);
}

static inline void d3d11_scale_texture(ID3D11RenderTargetView *target,
		ID3D11ShaderResourceView *resource)
{
	static struct d3d11_state old_state = {0};

	d3d11_save_state(&old_state);
	d3d11_setup_pipeline(target, resource);
	data.context->Draw(4, 0);
	d3d11_restore_state(&old_state);
}

static inline void d3d11_copy_texture(ID3D11Resource *dst, ID3D11Resource *src)
{
	if (data.multisampled) {
		data.context->ResolveSubresource(dst, 0, src, 0, data.format);
	} else {
		data.context->CopyResource(dst, src);
	}
}

static inline void d3d11_shtex_capture(ID3D11Resource *backbuffer)
{
	if (data.using_scale) {
		d3d11_copy_texture(data.scale_tex, backbuffer);
		d3d11_scale_texture(data.render_target, data.scale_resource);
	} else {
		d3d11_copy_texture(data.texture, backbuffer);
	}
}

static inline void d3d11_shmem_queue_copy()
{
	for (size_t i = 0; i < NUM_BUFFERS; i++) {
		D3D11_MAPPED_SUBRESOURCE map;
		HRESULT hr;

		if (data.texture_ready[i]) {
			data.texture_ready[i] = false;

			hr = data.context->Map(data.copy_surfaces[i], 0,
					D3D11_MAP_READ, 0, &map);
			if (SUCCEEDED(hr)) {
				data.texture_mapped[i] = true;
				shmem_copy_data(i, map.pData);
			}
			break;
		}
	}
}

static inline void d3d11_shmem_capture(ID3D11Resource *backbuffer)
{
	int next_tex;

	d3d11_shmem_queue_copy();

	next_tex = (data.cur_tex == NUM_BUFFERS - 1) ?  0 : data.cur_tex + 1;

	if (data.using_scale) {
		d3d11_copy_texture(data.scale_tex, backbuffer);
		d3d11_scale_texture(data.render_targets[data.cur_tex],
				data.scale_resource);
	} else {
		d3d11_copy_texture(data.textures[data.cur_tex], backbuffer);
	}

	if (data.copy_wait < NUM_BUFFERS - 1) {
		data.copy_wait++;
	} else {
		ID3D11Texture2D *src = data.textures[next_tex];
		ID3D11Texture2D *dst = data.copy_surfaces[next_tex];

		if (shmem_texture_data_lock(next_tex)) {
			data.context->Unmap(dst, 0);
			data.texture_mapped[next_tex] = false;
			shmem_texture_data_unlock(next_tex);
		}

		d3d11_copy_texture(dst, src);
		data.texture_ready[next_tex] = true;
	}

	data.cur_tex = next_tex;
}

void d3d11_capture(void *swap_ptr, void *backbuffer_ptr, bool)
{
	IDXGIResource *dxgi_backbuffer = (IDXGIResource*)backbuffer_ptr;
	IDXGISwapChain *swap = (IDXGISwapChain*)swap_ptr;

	HRESULT hr;
	if (capture_should_stop()) {
		d3d11_free();
	}
	if (capture_should_init()) {
		d3d11_init(swap);
	}
	if (capture_ready()) {
		ID3D11Resource *backbuffer;

		hr = dxgi_backbuffer->QueryInterface(__uuidof(ID3D11Resource),
				(void**)&backbuffer);
		if (FAILED(hr)) {
			hlog_hr("d3d11_shtex_capture: failed to get "
			        "backbuffer", hr);
			return;
		}

		if (data.using_shtex)
			d3d11_shtex_capture(backbuffer);
		else
			d3d11_shmem_capture(backbuffer);

		backbuffer->Release();
	}
}
