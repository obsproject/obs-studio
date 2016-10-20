#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <VersionHelpers.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "get-graphics-offsets.h"

static const IID dxgi_factory2_iid =
{0x50c83a1c, 0xe072, 0x4c48, {0x87, 0xb0, 0x36, 0x30, 0xfa, 0x36, 0xa6, 0xd0}};

typedef HRESULT (WINAPI *create_factory1_t)(REFIID, void**);
typedef PFN_D3D11_CREATE_DEVICE create_device_t;

struct dxgi_info {
	HMODULE             module;
	HWND                hwnd;
	ID3D11Device        *device;
	ID3D11DeviceContext *context;
	IDXGIFactory1       *factory;
	IDXGIAdapter1       *adapter;
	IDXGISwapChain      *swap;
};

const static D3D_FEATURE_LEVEL feature_levels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
};

static inline bool dxgi_init(dxgi_info &info)
{
	IDXGIFactory2 *factory2 = nullptr;
	HMODULE       d3d11_module;
	HRESULT       hr;

	create_factory1_t create_factory1;
	create_device_t create_device;

	info.hwnd = CreateWindowExA(0, DUMMY_WNDCLASS, "d3d11 get-offset window",
			WS_POPUP, 0, 0, 2, 2, nullptr, nullptr,
			GetModuleHandleA(nullptr), nullptr);
	if (!info.hwnd) {
		return false;
	}

	info.module = LoadLibraryA("dxgi.dll");
	if (!info.module) {
		return false;
	}

	d3d11_module = LoadLibraryA("d3d11.dll");
	if (!d3d11_module) {
		return false;
	}

	create_factory1 = (create_factory1_t)GetProcAddress(info.module,
			"CreateDXGIFactory1");
	if (!create_factory1) {
		return false;
	}

	create_device = (create_device_t)GetProcAddress(d3d11_module,
			"D3D11CreateDevice");
	if (!create_device) {
		return false;
	}

	IID factory_iid = IsWindows8OrGreater() ? dxgi_factory2_iid :
		__uuidof(IDXGIFactory1);

	hr = create_factory1(factory_iid, (void**)&info.factory);
	if (FAILED(hr)) {
		return false;
	}

	if (IsWindows8OrGreater())
		factory2 = reinterpret_cast<IDXGIFactory2*>(info.factory);

	hr = info.factory->EnumAdapters1(0, &info.adapter);
	if (FAILED(hr)) {
		return false;
	}

	hr = create_device(info.adapter, D3D_DRIVER_TYPE_UNKNOWN,
			nullptr, 0, feature_levels,
			sizeof(feature_levels) / sizeof(feature_levels[0]),
			D3D11_SDK_VERSION, &info.device, nullptr,
			&info.context);
	if (FAILED(hr)) {
		return false;
	}

	if (factory2) {
		DXGI_SWAP_CHAIN_DESC1 desc = {};
		desc.BufferCount           = 2;
		desc.BufferUsage           = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		desc.Width                 = 2;
		desc.Height                = 2;
		desc.Format                = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count      = 1;
		desc.SwapEffect            = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;

		hr = factory2->CreateSwapChainForHwnd(info.device,
				info.hwnd, &desc, nullptr, nullptr,
				(IDXGISwapChain1**)&info.swap);
		if (FAILED(hr)) {
			return false;
		}

		return true;
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

	hr = info.factory->CreateSwapChain(info.device, &desc, &info.swap);
	if (FAILED(hr)) {
		return false;
	}

	return true;
}

static inline void dxgi_free(dxgi_info &info)
{
	if (info.swap)
		info.swap->Release();
	if (info.factory)
		info.factory->Release();
	if (info.adapter)
		info.adapter->Release();
	if (info.device)
		info.device->Release();
	if (info.context)
		info.context->Release();
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

		if (IsWindows8OrGreater())
			offsets->present1 = vtable_offset(info.module,
					info.swap, 22);
	}

	dxgi_free(info);
}
