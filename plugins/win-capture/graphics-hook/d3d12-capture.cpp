#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include "graphics-hook.h"

#if COMPILE_D3D12_HOOK

#include <d3d11on12.h>
#include <d3d12.h>
#include <dxgi1_2.h>

#include "dxgi-helpers.hpp"
#include "../funchook.h"

struct d3d12_data {
	ID3D12Device                   *device; /* do not release */
	uint32_t                       base_cx;
	uint32_t                       base_cy;
	uint32_t                       cx;
	uint32_t                       cy;
	DXGI_FORMAT                    format;
	bool                           using_shtex : 1;
	bool                           using_scale : 1;
	bool                           multisampled : 1;

	ID3D11Device                   *device11;
	ID3D11DeviceContext            *context11;
	ID3D11On12Device               *device11on12;

	union {
		struct {
			struct shtex_data      *shtex_info;
			ID3D11Resource         *backbuffer11;
			ID3D11Texture2D        *copy_tex;
			HANDLE                 handle;
		};
	};
};

static struct d3d12_data data = {};

void d3d12_free(void)
{
	if (data.copy_tex)
		data.copy_tex->Release();
	if (data.backbuffer11)
		data.backbuffer11->Release();
	if (data.device11)
		data.device11->Release();
	if (data.context11)
		data.context11->Release();
	if (data.device11on12)
		data.device11on12->Release();

	capture_free();

	memset(&data, 0, sizeof(data));

	hlog("----------------- d3d12 capture freed ----------------");
}

static bool create_d3d12_tex(ID3D12Resource *backbuffer)
{
	D3D11_RESOURCE_FLAGS rf11 = {};
	HRESULT hr;

	hr = data.device11on12->CreateWrappedResource(backbuffer, &rf11,
			D3D12_RESOURCE_STATE_COPY_SOURCE,
			D3D12_RESOURCE_STATE_PRESENT,
			__uuidof(ID3D11Resource),
			(void**)&data.backbuffer11);
	if (FAILED(hr)) {
		hlog_hr("create_d3d12_tex: failed to create backbuffer11",
				hr);
		return false;
	}

	D3D11_TEXTURE2D_DESC desc11 = {};
	desc11.Width = data.cx;
	desc11.Height = data.cy;
	desc11.MipLevels = 1;
	desc11.ArraySize = 1;
	desc11.Format = data.format;
	desc11.SampleDesc.Count = 1;
	desc11.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc11.MiscFlags = D3D11_RESOURCE_MISC_SHARED;

	hr = data.device11->CreateTexture2D(&desc11, nullptr, &data.copy_tex);
	if (FAILED(hr)) {
		hlog_hr("create_d3d12_tex: creation of d3d11 copy tex failed",
				hr);
		return false;
	}

	data.device11on12->ReleaseWrappedResources(&data.backbuffer11, 1);

	IDXGIResource *dxgi_res;
	hr = data.copy_tex->QueryInterface(__uuidof(IDXGIResource),
			(void**)&dxgi_res);
	if (FAILED(hr)) {
		hlog_hr("create_d3d12_tex: failed to query "
			"IDXGIResource interface from texture", hr);
		return false;
	}

	hr = dxgi_res->GetSharedHandle(&data.handle);
	dxgi_res->Release();
	if (FAILED(hr)) {
		hlog_hr("create_d3d12_tex: failed to get shared handle", hr);
		return false;
	}

	return true;
}

typedef PFN_D3D11ON12_CREATE_DEVICE create_11_on_12_t;

const static D3D_FEATURE_LEVEL feature_levels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
};

static bool d3d12_init_11on12(void)
{
	static HMODULE d3d11 = nullptr;
	static create_11_on_12_t create_11_on_12 = nullptr;
	static bool initialized_11 = false;
	static bool initialized_func = false;
	HRESULT hr;

	if (!initialized_11 && !d3d11) {
		d3d11 = load_system_library("d3d11.dll");
		if (!d3d11) {
			hlog("d3d12_init_11on12: failed to load d3d11");
		}
		initialized_11 = true;
	}

	if (!d3d11) {
		return false;
	}

	if (!initialized_func && !create_11_on_12) {
		create_11_on_12 = (create_11_on_12_t)GetProcAddress(d3d11,
				"D3D11On12CreateDevice");
		if (!create_11_on_12) {
			hlog("d3d12_init_11on12: Failed to get "
					"D3D11On12CreateDevice address");
		}

		initialized_func = true;
	}

	if (!create_11_on_12) {
		return false;
	}

	hr = create_11_on_12(data.device, 0, nullptr, 0,
			nullptr, 0, 0,
			&data.device11, &data.context11, nullptr);
	if (FAILED(hr)) {
		hlog_hr("d3d12_init_11on12: failed to create 11 device", hr);
		return false;
	}

	data.device11->QueryInterface(__uuidof(ID3D11On12Device),
			(void**)&data.device11on12);
	if (FAILED(hr)) {
		hlog_hr("d3d12_init_11on12: failed to query 11on12 device", hr);
		return false;
	}

	return true;
}

static bool d3d12_shtex_init(HWND window, ID3D12Resource *backbuffer)
{
	if (!d3d12_init_11on12()) {
		return false;
	}
	if (!create_d3d12_tex(backbuffer)) {
		return false;
	}
	if (!capture_init_shtex(&data.shtex_info, window,
				data.base_cx, data.base_cy, data.cx, data.cy,
				data.format, false, (uintptr_t)data.handle)) {
		return false;
	}

	hlog("d3d12 shared texture capture successful");
	return true;
}

static inline bool d3d12_init_format(IDXGISwapChain *swap, HWND &window)
{
	DXGI_SWAP_CHAIN_DESC desc;
	HRESULT hr;

	hr = swap->GetDesc(&desc);
	if (FAILED(hr)) {
		hlog_hr("d3d12_init_format: swap->GetDesc failed", hr);
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

static void d3d12_init(IDXGISwapChain *swap, ID3D12Resource *backbuffer)
{
	bool success = true;
	HWND window;
	HRESULT hr;

	data.using_scale = global_hook_info->use_scale;

	hr = swap->GetDevice(__uuidof(ID3D12Device), (void**)&data.device);
	if (FAILED(hr)) {
		hlog_hr("d3d12_init: failed to get device from swap", hr);
		return;
	}

	data.device->Release();

	if (!d3d12_init_format(swap, window)) {
		return;
	}
	if (data.using_scale) {
		hlog("d3d12_init: scaling currently unsupported; ignoring");
	}
	if (success) {
		if (global_hook_info->force_shmem) {
			hlog("d3d12_init: shared memory capture currently "
					"unsupported; ignoring");
		}

		success = d3d12_shtex_init(window, backbuffer);
	}

	if (!success)
		d3d12_free();
}

static inline void d3d12_copy_texture(ID3D11Resource *dst, ID3D11Resource *src)
{
	if (data.multisampled) {
		data.context11->ResolveSubresource(dst, 0, src, 0, data.format);
	} else {
		data.context11->CopyResource(dst, src);
	}
}

static inline void d3d12_shtex_capture()
{
	data.device11on12->AcquireWrappedResources(&data.backbuffer11, 1);
	d3d12_copy_texture(data.copy_tex, data.backbuffer11);
	data.device11on12->ReleaseWrappedResources(&data.backbuffer11, 1);
	data.context11->Flush();
}

void d3d12_capture(void *swap_ptr, void *backbuffer_ptr)
{
	IUnknown *unk_backbuffer = (IUnknown*)backbuffer_ptr;
	IDXGISwapChain *swap = (IDXGISwapChain*)swap_ptr;
	ID3D12Resource *backbuffer;
	HRESULT hr;

	if (capture_should_stop()) {
		d3d12_free();
	}
	if (capture_should_init()) {
		hr = unk_backbuffer->QueryInterface(__uuidof(ID3D12Resource),
				(void**)&backbuffer);
		if (FAILED(hr)) {
			hlog_hr("d3d12_capture: failed to get backbuffer", hr);
			return;
		}

		d3d12_init(swap, backbuffer);
		backbuffer->Release();
	}
	if (capture_ready()) {
		d3d12_shtex_capture();
	}
}

#endif
