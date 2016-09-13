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

#ifdef _WIN64

#define CMP_SIZE 21

static const uint8_t mask[CMP_SIZE] =
{0xF8, 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0x00,
 0xF8, 0xF8, 0x00, 0x00, 0x00, 0x00};

static const uint8_t mask_cmp[CMP_SIZE] =
{0x48, 0x8B, 0x80, 0x00, 0x00, 0x00, 0x00,
 0x39, 0x80, 0x00, 0x00, 0x00, 0x00,
 0x75, 0x00,
 0x40, 0xB8, 0x00, 0x00, 0x00, 0x00};
#else

#define CMP_SIZE 19

static const uint8_t mask[CMP_SIZE] =
{0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0xC0, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0x00,
 0xFF, 0x00, 0x00, 0x00, 0x00};

static const uint8_t mask_cmp[CMP_SIZE] =
{0x8B, 0x80, 0x00, 0x00, 0x00, 0x00,
 0x39, 0x80, 0x00, 0x00, 0x00, 0x00,
 0x75, 0x00,
 0x68, 0x00, 0x00, 0x00, 0x00};
#endif

#define MAX_FUNC_SCAN_BYTES 200

static inline bool pattern_matches(uint8_t *byte)
{
	for (size_t i = 0; i < CMP_SIZE; i++) {
		if ((byte[i] & mask[i]) != mask_cmp[i])
			return false;
	}

	return true;
}

void get_d3d9_offsets(struct d3d9_offsets *offsets)
{
	d3d9_info info    = {};
	bool      success = d3d9_init(info);

	if (success) {
		uint8_t **vt = *(uint8_t***)info.device;
		uint8_t *crr = vt[125];

		offsets->present = vtable_offset(info.module, info.device, 17);
		offsets->present_ex = vtable_offset(info.module, info.device,
				121);
		offsets->present_swap = vtable_offset(info.module, info.swap,
				3);

		for (size_t i = 0; i < MAX_FUNC_SCAN_BYTES; i++) {
			if (pattern_matches(&crr[i])) {
#define get_offset(x) *(uint32_t*)&crr[i + x]
#ifdef _WIN64
				uint32_t off1 = get_offset(3);
				uint32_t off2 = get_offset(9);
#else
				uint32_t off1 = get_offset(2);
				uint32_t off2 = get_offset(8);
#endif

				/* check to make sure offsets are within
				 * expected values */
				if (off1 > 0xFFFF || off2 > 0xFFFF)
					break;

				/* check to make sure offsets actually point
				 * toward expected data */
#ifdef _MSC_VER
				__try {
					uint8_t *ptr = (uint8_t*)(info.device);

					uint8_t *d3d9_ptr =
						*(uint8_t**)(ptr + off1);
					if (d3d9_ptr != (uint8_t*)info.d3d9ex)
						break;

					BOOL &is_d3d9ex =
						*(BOOL*)(d3d9_ptr + off2);
					if (is_d3d9ex != TRUE)
						break;

				} __except(EXCEPTION_EXECUTE_HANDLER) {
					break;
				}
#endif

				offsets->d3d9_clsoff = off1;
				offsets->is_d3d9ex_clsoff = off2;
				break;
			}
		}
	}

	d3d9_free(info);
}
