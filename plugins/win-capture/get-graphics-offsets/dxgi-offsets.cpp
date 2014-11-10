#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <d3d10.h>
#include <dxgi.h>
#include "get-graphics-offsets.h"

typedef HRESULT (WINAPI *d3d10create_t)(IDXGIAdapter*, D3D10_DRIVER_TYPE,
		HMODULE, UINT, UINT, DXGI_SWAP_CHAIN_DESC*,
		IDXGISwapChain**, IUnknown**);

struct dxgi_info {
	HMODULE        module;
	HWND           hwnd;
	IDXGISwapChain *swap;
};

static inline bool dxgi_init(dxgi_info &info)
{
	HMODULE       d3d10_module;
	d3d10create_t create;
	IUnknown      *device;
	HRESULT       hr;

	info.hwnd = CreateWindowExA(0, DUMMY_WNDCLASS, "d3d10 get-offset window",
			WS_POPUP, 0, 0, 2, 2, nullptr, nullptr,
			GetModuleHandleA(nullptr), nullptr);
	if (!info.hwnd) {
		return false;
	}

	info.module = LoadLibraryA("dxgi.dll");
	if (!info.module) {
		return false;
	}

	d3d10_module = LoadLibraryA("d3d10.dll");
	if (!d3d10_module) {
		return false;
	}

	create = (d3d10create_t)GetProcAddress(d3d10_module,
			"D3D10CreateDeviceAndSwapChain");
	if (!create) {
		return false;
	}

	DXGI_SWAP_CHAIN_DESC desc = {};
	desc.BufferCount          = 2;
	desc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.Width     = 2;
	desc.BufferDesc.Height    = 2;
	desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow         = info.hwnd;
	desc.SampleDesc.Count     = 1;
	desc.Windowed             = true;

	hr = create(nullptr, D3D10_DRIVER_TYPE_NULL, nullptr, 0,
			D3D10_SDK_VERSION, &desc, &info.swap, &device);
	if (FAILED(hr)) {
		return false;
	}

	device->Release();
	return true;
}

static inline void dxgi_free(dxgi_info &info)
{
	if (info.swap)
		info.swap->Release();
	if (info.hwnd)
		DestroyWindow(info.hwnd);
}

void get_dxgi_offsets(struct dxgi_offsets *offsets)
{
	dxgi_info info    = {};
	bool      success = dxgi_init(info);

	if (success) {
		offsets->present = vtable_offset(info.module, info.swap, 8);
		offsets->resize  = vtable_offset(info.module, info.swap, 13);
	}

	dxgi_free(info);
}
