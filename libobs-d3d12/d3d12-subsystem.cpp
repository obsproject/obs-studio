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

#include <cassert>
#include <cinttypes>
#include <optional>
#include <util/base.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/util.hpp>
#include <graphics/matrix3.h>
#include <winternl.h>
#include "d3d12-subsystem.hpp"
#include <d3dkmthk.h>

struct UnsupportedHWError : HRError {
	inline UnsupportedHWError(const char *str, HRESULT hr)
		: HRError(str, hr)
	{
	}
};

#ifdef _MSC_VER
/* alignment warning - despite the fact that alignment is already fixed */
#pragma warning(disable : 4316)
#endif

static inline void LogD3D12ErrorDetails(HRError error, gs_device_t *device)
{
	if (error.hr == DXGI_ERROR_DEVICE_REMOVED) {
		HRESULT DeviceRemovedReason =
			device->device->GetDeviceRemovedReason();
		blog(LOG_ERROR, "  Device Removed Reason: %08lX",
		     DeviceRemovedReason);
	}
}

gs_obj::gs_obj(gs_device_t *device_, gs_type type)
	: device(device_),
	  obj_type(type)
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

static enum gs_color_space get_next_space(gs_device_t *device, HWND hwnd,
					  DXGI_SWAP_EFFECT effect)
{
	enum gs_color_space next_space = GS_CS_SRGB;
	if (effect == DXGI_SWAP_EFFECT_FLIP_DISCARD) {
		const HMONITOR hMonitor =
			MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		if (hMonitor) {
			const gs_monitor_color_info info =
				device->GetMonitorColorInfo(hMonitor);
			if (info.hdr)
				next_space = GS_CS_709_SCRGB;
			else if (info.bits_per_color > 8)
				next_space = GS_CS_SRGB_16F;
		}
	}

	return next_space;
}

static enum gs_color_format
get_swap_format_from_space(gs_color_space space, gs_color_format sdr_format)
{
	gs_color_format format = sdr_format;
	switch (space) {
	case GS_CS_SRGB_16F:
	case GS_CS_709_SCRGB:
		format = GS_RGBA16F;
	}

	return format;
}

static inline enum gs_color_space
make_swap_desc(gs_device *device, DXGI_SWAP_CHAIN_DESC &desc,
	       const gs_init_data *data, DXGI_SWAP_EFFECT effect, UINT flags)
{
	const HWND hwnd = (HWND)data->window.hwnd;
	const enum gs_color_space space = get_next_space(device, hwnd, effect);
	const gs_color_format format =
		get_swap_format_from_space(space, data->format);

	memset(&desc, 0, sizeof(desc));
	desc.BufferDesc.Width = data->cx;
	desc.BufferDesc.Height = data->cy;
	desc.BufferDesc.Format = ConvertGSTextureFormatView(format);
	desc.SampleDesc.Count = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = data->num_backbuffers;
	desc.OutputWindow = hwnd;
	desc.Windowed = TRUE;
	desc.SwapEffect = effect;
	desc.Flags = flags;

	return space;
}

void gs_device::InitFactory()
{
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to create DXGIFactory6", hr);
}

void gs_device::InitAdapter(uint32_t adapterIdx)
{
	HRESULT hr = factory->EnumAdapterByGpuPreference(
		adapterIdx, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&adapter));
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to enumerate DXGIAdapter4", hr);
}

bool gs_device::HasBadNV12Output()
try {
	return false;
} catch (const HRError &error) {
	blog(LOG_WARNING, "HasBadNV12Output failed: %s (%08lX)", error.str,
	     error.hr);
	return false;
} catch (const char *error) {
	blog(LOG_WARNING, "HasBadNV12Output failed: %s", error);
	return false;
}

void gs_device::InitDevice(uint32_t adapterIdx) {
	DXGI_ADAPTER_DESC3 desc;
	HRESULT hr = adapter->GetDesc3(&desc);
	if (hr != S_OK) {
		throw UnsupportedHWError("Adapter4 Failed to GetDesc", hr);
	}
	// todo log desc info
	hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_2, IID_PPV_ARGS(&device));
	if (FAILED(hr)) {
		throw UnsupportedHWError("Failed to D3D12CreateDevice", hr);
	}
}

void gs_device::UpdatePipelineState() {

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
}

void gs_device::FlushOutputViews() {}

gs_device::gs_device(uint32_t adapterIdx)
	: curToplogy(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
{
	matrix4_identity(&curProjMatrix);
	matrix4_identity(&curViewMatrix);
	matrix4_identity(&curViewProjMatrix);

	memset(&viewport, 0, sizeof(viewport));

	for (size_t i = 0; i < GS_MAX_TEXTURES; i++) {
		curTextures[i] = NULL;
		curSamplers[i] = NULL;
	}

	InitFactory();
	InitAdapter(adapterIdx);
	InitDevice(adapterIdx);
	device_set_render_target(this, NULL, NULL);
}

gs_device::~gs_device() {}

const char *device_get_name(void)
{
	return "Direct3D 12";
}

int device_get_type(void)
{
	return GS_DEVICE_DIRECT3D_12;
}

const char *device_preprocessor_name(void)
{
	return "_D3D12";
}

int device_create(gs_device_t **p_device, uint32_t adapter)
{
	gs_device *device = NULL;
	int errorcode = GS_SUCCESS;

	try {
		blog(LOG_INFO, "----------------------------------");
		blog(LOG_INFO, "Initializing D3D12...");
		device = new gs_device(adapter);
	} catch (const UnsupportedHWError &error) {
		blog(LOG_ERROR, "device_create (D3D12): %s (%08lX)", error.str,
		     error.hr);
		errorcode = GS_ERROR_NOT_SUPPORTED;

	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_create (D3D12): %s (%08lX)", error.str,
		     error.hr);
		errorcode = GS_ERROR_FAIL;
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

void *device_get_device_obj(gs_device_t *device)
{
	return (void *)device->device.Get();
}

gs_swapchain_t *device_swapchain_create(gs_device_t *device,
					const struct gs_init_data *data)
{
	/* not implement */
	return nullptr;
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	/* not implement */
}

enum gs_color_space device_get_color_space(gs_device_t *device)
{
	/* not implement */
	return gs_color_space::GS_CS_SRGB;
}

void device_update_color_space(gs_device_t *device)
{
	/* not implement */
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	/* not implement */
}

uint32_t device_get_width(const gs_device_t *device)
{
	/* not implement */
	return 0;
}

uint32_t device_get_height(const gs_device_t *device)
{
	/* not implement */
	return 0;
}

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width,
				    uint32_t height,
				    enum gs_color_format color_format,
				    uint32_t levels, const uint8_t **data,
				    uint32_t flags)
{
	/* not implement */
	return nullptr;
}

gs_texture_t *device_cubetexture_create(gs_device_t *device, uint32_t size,
					enum gs_color_format color_format,
					uint32_t levels, const uint8_t **data,
					uint32_t flags)
{
	/* not implement */
	return nullptr;
}

gs_texture_t *device_voltexture_create(gs_device_t *device, uint32_t width,
				       uint32_t height, uint32_t depth,
				       enum gs_color_format color_format,
				       uint32_t levels,
				       const uint8_t *const *data,
				       uint32_t flags)
{
	/* not implement */
	return nullptr;
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width,
				      uint32_t height,
				      enum gs_zstencil_format format)
{
	/* not implement */
	return nullptr;
}

gs_stagesurf_t *device_stagesurface_create(gs_device_t *device, uint32_t width,
					   uint32_t height,
					   enum gs_color_format color_format)
{
	/* not implement */
	return nullptr;
}

gs_samplerstate_t *
device_samplerstate_create(gs_device_t *device,
			   const struct gs_sampler_info *info)
{
	/* not implement */
	return nullptr;
}

gs_shader_t *device_vertexshader_create(gs_device_t *device,
					const char *shader_string,
					const char *file, char **error_string)
{
	/* not implement */
	return nullptr;
}

gs_shader_t *device_pixelshader_create(gs_device_t *device,
				       const char *shader_string,
				       const char *file, char **error_string)
{
	/* not implement */
	return nullptr;
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device,
					    struct gs_vb_data *data,
					    uint32_t flags)
{
	/* not implement */
	return nullptr;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device,
					    enum gs_index_type type,
					    void *indices, size_t num,
					    uint32_t flags)
{ /* not implement */
	return nullptr;
}

gs_timer_t *device_timer_create(gs_device_t *device)
{
	/* not implement */
	return nullptr;
}

gs_timer_range_t *device_timer_range_create(gs_device_t *device)
{
	/* not implement */
	return nullptr;
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	/* not implement */
	return gs_texture_type::GS_TEXTURE_2D;
}

void gs_device::LoadVertexBufferData() {}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	/* not implement */
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	/* not implement */
}

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	/* not implement */
}

void device_load_texture_srgb(gs_device_t *device, gs_texture_t *tex, int unit)
{
	/* not implement */
}

void device_load_samplerstate(gs_device_t *device,
			      gs_samplerstate_t *samplerstate, int unit)
{
	/* not implement */
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	/* not implement */
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	/* not implement */
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
	if (device->curRenderTarget == &device->curSwapChain->target)
		return NULL;

	return device->curRenderTarget;
}

gs_zstencil_t *device_get_zstencil_target(const gs_device_t *device)
{
	if (device->curZStencilBuffer == &device->curSwapChain->zs)
		return NULL;

	return device->curZStencilBuffer;
}

void device_set_render_target(gs_device_t *device, gs_texture_t *tex,
			      gs_zstencil_t *zstencil)
{
	/* not implement */
}

void device_set_render_target_with_color_space(gs_device_t *device,
					       gs_texture_t *tex,
					       gs_zstencil_t *zstencil,
					       enum gs_color_space space)
{
	/* not implement */
}

void device_set_cube_render_target(gs_device_t *device, gs_texture_t *tex,
				   int side, gs_zstencil_t *zstencil)
{
	/* not implement */
}

void device_enable_framebuffer_srgb(gs_device_t *device, bool enable)
{
	/* not implement */
}

bool device_framebuffer_srgb_enabled(gs_device_t *device)
{
	return device->curFramebufferSrgb;
}
void device_copy_texture_region(gs_device_t *device, gs_texture_t *dst,
				uint32_t dst_x, uint32_t dst_y,
				gs_texture_t *src, uint32_t src_x,
				uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	/* not implement */
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst,
			 gs_texture_t *src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst,
			  gs_texture_t *src)
{
	/* not implement */
}

void device_begin_frame(gs_device_t *device)
{
	/* not implement */
}

void device_begin_scene(gs_device_t *device)
{
	/* not implement */
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode,
		 uint32_t start_vert, uint32_t num_verts)
{
	/* not implement */
}

void device_end_scene(gs_device_t *device)
{
	/* does nothing in D3D11 */
	UNUSED_PARAMETER(device);
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	/* not implement */
}

void device_clear(gs_device_t *device, uint32_t clear_flags,
		  const struct vec4 *color, float depth, uint8_t stencil)
{
	/* not implement */
}

bool device_is_present_ready(gs_device_t *device)
{
	/* not implement */
	return false;
}

void device_present(gs_device_t *device)
{
	/* not implement */
}

void device_flush(gs_device_t *device)
{
	/* not implement */
}

void device_set_cull_mode(gs_device_t *device, enum gs_cull_mode mode)
{
	/* not implement */
}

enum gs_cull_mode device_get_cull_mode(const gs_device_t *device)
{
	/* not implement */
	return gs_cull_mode::GS_NEITHER;
}

void device_enable_blending(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_depth_test(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_stencil_test(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_stencil_write(gs_device_t *device, bool enable)
{
	/* not implement */
}

void device_enable_color(gs_device_t *device, bool red, bool green, bool blue,
			 bool alpha)
{
	/* not implement */
}

void device_blend_function(gs_device_t *device, enum gs_blend_type src,
			   enum gs_blend_type dest)
{
	/* not implement */
}

void device_blend_function_separate(gs_device_t *device,
				    enum gs_blend_type src_c,
				    enum gs_blend_type dest_c,
				    enum gs_blend_type src_a,
				    enum gs_blend_type dest_a)
{
	/* not implement */
}

void device_blend_op(gs_device_t *device, enum gs_blend_op_type op)
{
	/* not implement */
}

void device_depth_function(gs_device_t *device, enum gs_depth_test test)
{
	/* not implement */
}

static inline void update_stencilside_test(gs_device_t *device,
					   StencilSide &side,
					   gs_depth_test test)
{
	/* not implement */
}

void device_stencil_function(gs_device_t *device, enum gs_stencil_side side,
			     enum gs_depth_test test)
{
	/* not implement */
}

static inline void update_stencilside_op(gs_device_t *device, StencilSide &side,
					 enum gs_stencil_op_type fail,
					 enum gs_stencil_op_type zfail,
					 enum gs_stencil_op_type zpass)
{
	/* not implement */
}

void device_stencil_op(gs_device_t *device, enum gs_stencil_side side,
		       enum gs_stencil_op_type fail,
		       enum gs_stencil_op_type zfail,
		       enum gs_stencil_op_type zpass)
{
	/* not implement */
}

void device_set_viewport(gs_device_t *device, int x, int y, int width,
			 int height)
{
	/* not implement */
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect)
{
	memcpy(rect, &device->viewport, sizeof(gs_rect));
}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	/* not implement */
}

void device_ortho(gs_device_t *device, float left, float right, float top,
		  float bottom, float zNear, float zFar)
{
	matrix4 *dst = &device->curProjMatrix;

	float rml = right - left;
	float bmt = bottom - top;
	float fmn = zFar - zNear;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x = 2.0f / rml;
	dst->t.x = (left + right) / -rml;

	dst->y.y = 2.0f / -bmt;
	dst->t.y = (bottom + top) / bmt;

	dst->z.z = 1.0f / fmn;
	dst->t.z = zNear / -fmn;

	dst->t.w = 1.0f;
}

void device_frustum(gs_device_t *device, float left, float right, float top,
		    float bottom, float zNear, float zFar)
{
	matrix4 *dst = &device->curProjMatrix;

	float rml = right - left;
	float bmt = bottom - top;
	float fmn = zFar - zNear;
	float nearx2 = 2.0f * zNear;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x = nearx2 / rml;
	dst->z.x = (left + right) / -rml;

	dst->y.y = nearx2 / -bmt;
	dst->z.y = (bottom + top) / bmt;

	dst->z.z = zFar / fmn;
	dst->t.z = (zNear * zFar) / -fmn;

	dst->z.w = 1.0f;
}

void device_projection_push(gs_device_t *device)
{
	mat4float mat;
	memcpy(&mat, &device->curProjMatrix, sizeof(matrix4));
	device->projStack.push_back(mat);
}

void device_projection_pop(gs_device_t *device)
{
	if (device->projStack.empty())
		return;

	const mat4float &mat = device->projStack.back();
	memcpy(&device->curProjMatrix, &mat, sizeof(matrix4));
	device->projStack.pop_back();
}

void gs_swapchain_destroy(gs_swapchain_t *swapchain)
{
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
	/* not implement */
	return 0;
}

uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	/* not implement */
	return 0;
}

enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return GS_UNKNOWN;

	return static_cast<const gs_texture_2d *>(tex)->format;
}

bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr, uint32_t *linesize)
{
	/* not implement */
	return false;
}

void gs_texture_unmap(gs_texture_t *tex)
{
	/* not implement */
}

void *gs_texture_get_obj(gs_texture_t *tex)
{
	/* not implement */
	return nullptr;
}

void gs_cubetexture_destroy(gs_texture_t *cubetex)
{
	delete cubetex;
}

uint32_t gs_cubetexture_get_size(const gs_texture_t *cubetex)
{
	/* not implement */
	return 0;
}

enum gs_color_format
gs_cubetexture_get_color_format(const gs_texture_t *cubetex)
{
	if (cubetex->type != GS_TEXTURE_CUBE)
		return GS_UNKNOWN;

	const gs_texture_2d *tex = static_cast<const gs_texture_2d *>(cubetex);
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
	delete stagesurf;
}

uint32_t gs_stagesurface_get_width(const gs_stagesurf_t *stagesurf)
{
	return stagesurf->width;
}

uint32_t gs_stagesurface_get_height(const gs_stagesurf_t *stagesurf)
{
	return stagesurf->height;
}

enum gs_color_format
gs_stagesurface_get_color_format(const gs_stagesurf_t *stagesurf)
{
	return stagesurf->format;
}

bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data,
			 uint32_t *linesize)
{
	/* not implement */
	return true;
}

void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf)
{
	/* not implement */
}

void gs_zstencil_destroy(gs_zstencil_t *zstencil)
{
	/* not implement */
}

void gs_samplerstate_destroy(gs_samplerstate_t *samplerstate)
{
	/* not implement */
}

void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer)
{
	if (vertbuffer && vertbuffer->device->lastVertexBuffer == vertbuffer)
		vertbuffer->device->lastVertexBuffer = nullptr;
	delete vertbuffer;
}

static inline void gs_vertexbuffer_flush_internal(gs_vertbuffer_t *vertbuffer,
						  const gs_vb_data *data)
{
	/* not implement */
}

void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer)
{
	/* not implement */
}

void gs_vertexbuffer_flush_direct(gs_vertbuffer_t *vertbuffer,
				  const gs_vb_data *data)
{
	gs_vertexbuffer_flush_internal(vertbuffer, data);
}

struct gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer)
{
	/* not implement */
	return nullptr;
}

void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer)
{
	delete indexbuffer;
}

static inline void gs_indexbuffer_flush_internal(gs_indexbuffer_t *indexbuffer,
						 const void *data)
{
	/* not implement */
}

void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer)
{
	/* not implement */
}

void gs_indexbuffer_flush_direct(gs_indexbuffer_t *indexbuffer,
				 const void *data)
{
	gs_indexbuffer_flush_internal(indexbuffer, data);
}

void *gs_indexbuffer_get_data(const gs_indexbuffer_t *indexbuffer)
{
	/* not implement */
	return nullptr;
}

size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *indexbuffer)
{
	/* not implement */
	return 0;
}

enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *indexbuffer)
{
	/* not implement */
	return gs_index_type::GS_UNSIGNED_LONG;
}

void gs_timer_destroy(gs_timer_t *timer)
{
	/* not implement */
}

void gs_timer_begin(gs_timer_t *timer)
{
	/* not implement */
}

void gs_timer_end(gs_timer_t *timer)
{
	/* not implement */
}

bool gs_timer_get_data(gs_timer_t *timer, uint64_t *ticks)
{
	/* not implement */
	return false;
}

void gs_timer_range_destroy(gs_timer_range_t *range)
{
	delete range;
}

void gs_timer_range_begin(gs_timer_range_t *range)
{
	/* not implement */
}

void gs_timer_range_end(gs_timer_range_t *range)
{
	/* not implement */
}

bool gs_timer_range_get_data(gs_timer_range_t *range, bool *disjoint,
			     uint64_t *frequency)
{
	/* not implement */
	return false;
}

gs_timer::gs_timer(gs_device_t *device) : gs_obj(device, gs_type::gs_timer)
{
	/* not implement */
}

gs_timer_range::gs_timer_range(gs_device_t *device)
	: gs_obj(device, gs_type::gs_timer_range)
{
	/* not implement */
}

extern "C" EXPORT bool device_gdi_texture_available(void)
{
	return true;
}

extern "C" EXPORT bool device_shared_texture_available(void)
{
	return true;
}

extern "C" EXPORT bool device_nv12_available(gs_device_t *device)
{
	return device->nv12Supported;
}

extern "C" EXPORT bool device_p010_available(gs_device_t *device)
{
	return device->p010Supported;
}

extern "C" EXPORT bool device_is_monitor_hdr(gs_device_t *device, void *monitor)
{
	/* not implement */
	return false;
}

extern "C" EXPORT void device_debug_marker_begin(gs_device_t *,
						 const char *markername,
						 const float color[4])
{
	/* not implement */
}

extern "C" EXPORT void device_debug_marker_end(gs_device_t *)
{
	/* not implement */
}

extern "C" EXPORT gs_texture_t *
device_texture_create_gdi(gs_device_t *device, uint32_t width, uint32_t height)
{
	/* not implement */
	return nullptr;
}

static inline bool TextureGDICompatible(gs_texture_2d *tex2d, const char *func)
{
	/* not implement */
	return false;
}

extern "C" EXPORT void *gs_texture_get_dc(gs_texture_t *tex)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT void gs_texture_release_dc(gs_texture_t *tex)
{
	/* not implement */
}

extern "C" EXPORT gs_texture_t *device_texture_open_shared(gs_device_t *device,
							   uint32_t handle)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT gs_texture_t *
device_texture_open_nt_shared(gs_device_t *device, uint32_t handle)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT uint32_t device_texture_get_shared_handle(gs_texture_t *tex)
{
	/* not implement */
	return 0;
}

extern "C" EXPORT gs_texture_t *device_texture_wrap_obj(gs_device_t *device,
							void *obj)
{
	/* not implement */
	return nullptr;
}

int device_texture_acquire_sync(gs_texture_t *tex, uint64_t key, uint32_t ms)
{
	/* not implement */
	return 0;
}

extern "C" EXPORT int device_texture_release_sync(gs_texture_t *tex,
						  uint64_t key)
{
	/* not implement */

	return -1;
}

extern "C" EXPORT bool
device_texture_create_nv12(gs_device_t *device, gs_texture_t **p_tex_y,
			   gs_texture_t **p_tex_uv, uint32_t width,
			   uint32_t height, uint32_t flags)
{
	/* not implement */
	return true;
}

extern "C" EXPORT bool
device_texture_create_p010(gs_device_t *device, gs_texture_t **p_tex_y,
			   gs_texture_t **p_tex_uv, uint32_t width,
			   uint32_t height, uint32_t flags)
{
	/* not implement */
	return true;
}

extern "C" EXPORT gs_stagesurf_t *
device_stagesurface_create_nv12(gs_device_t *device, uint32_t width,
				uint32_t height)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT gs_stagesurf_t *
device_stagesurface_create_p010(gs_device_t *device, uint32_t width,
				uint32_t height)
{
	/* not implement */
	return nullptr;
}

extern "C" EXPORT void
device_register_loss_callbacks(gs_device_t *device,
			       const gs_device_loss *callbacks)
{
	/* not implement */
}

extern "C" EXPORT void device_unregister_loss_callbacks(gs_device_t *device,
							void *data)
{
	/* not implement */
}

uint32_t gs_get_adapter_count(void)
{
	/* not implement */
	return 0;
}

extern "C" EXPORT bool device_can_adapter_fast_clear(gs_device_t *device)
{
	/* not implement */
	return false;
}
