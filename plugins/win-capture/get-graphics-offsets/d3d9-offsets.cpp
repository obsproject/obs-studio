#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <windows.h>
#include <d3d9.h>
#include "get-graphics-offsets.h"

typedef HRESULT (WINAPI *d3d9createex_t)(UINT, IDirect3D9Ex**);

struct d3d9_info {
	HMODULE                        module;
	HWND                           hwnd;
	IDirect3D9Ex                   *d3d9ex;
	IDirect3DDevice9Ex             *device;
	IDirect3DSwapChain9            *swap;
};

static inline bool d3d9_init(d3d9_info &info)
{
	d3d9createex_t create;
	HRESULT        hr;

	info.hwnd = CreateWindowExA(0, DUMMY_WNDCLASS, "d3d9 get-offset window",
			WS_POPUP, 0, 0, 1, 1, nullptr, nullptr,
			GetModuleHandleA(nullptr), nullptr);
	if (!info.hwnd) {
		return false;
	}

	info.module = LoadLibraryA("d3d9.dll");
	if (!info.module) {
		return false;
	}

	create = (d3d9createex_t)GetProcAddress(info.module,
			"Direct3DCreate9Ex");
	if (!create) {
		return false;
	}

	hr = create(D3D_SDK_VERSION, &info.d3d9ex);
	if (FAILED(hr)) {
		return false;
	}

	D3DPRESENT_PARAMETERS pp = {};
	pp.Windowed              = true;
	pp.SwapEffect            = D3DSWAPEFFECT_FLIP;
	pp.BackBufferFormat      = D3DFMT_A8R8G8B8;
	pp.BackBufferWidth       = 2;
	pp.BackBufferHeight      = 2;
	pp.BackBufferCount       = 1;
	pp.hDeviceWindow         = info.hwnd;
	pp.PresentationInterval  = D3DPRESENT_INTERVAL_IMMEDIATE;

	hr = info.d3d9ex->CreateDeviceEx(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
			info.hwnd,
			D3DCREATE_HARDWARE_VERTEXPROCESSING |
			D3DCREATE_NOWINDOWCHANGES, &pp, nullptr, &info.device);
	if (FAILED(hr)) {
		return false;
	}

	hr = info.device->GetSwapChain(0, &info.swap);
	if (FAILED(hr)) {
		return false;
	}

	return true;
}

static inline void d3d9_free(d3d9_info &info)
{
	if (info.swap)
		info.swap->Release();
	if (info.device)
		info.device->Release();
	if (info.d3d9ex)
		info.d3d9ex->Release();
	if (info.hwnd)
		DestroyWindow(info.hwnd);
}

void get_d3d9_offsets(struct d3d9_offsets *offsets)
{
	d3d9_info info    = {};
	bool      success = d3d9_init(info);

	if (success) {
		offsets->present = vtable_offset(info.module, info.device, 17);
		offsets->present_ex = vtable_offset(info.module, info.device,
				121);
		offsets->present_swap = vtable_offset(info.module, info.swap,
				3);
	}

	d3d9_free(info);
}
