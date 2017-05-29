/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include <cinttypes>
#include <util/base.h>
#include <util/platform.h>
#include <graphics/matrix3.h>
#include "d3d11-subsystem.hpp"

struct UnsupportedHWError : HRError {
	inline UnsupportedHWError(const char *str, HRESULT hr)
		: HRError(str, hr)
	{
	}
};

#ifdef _MSC_VER
/* alignment warning - despite the fact that alignment is already fixed */
#pragma warning (disable : 4316)
#endif

static inline void LogD3D11ErrorDetails(HRError error, gs_device_t *device)
{
	if (error.hr == DXGI_ERROR_DEVICE_REMOVED) {
		HRESULT DeviceRemovedReason =
				device->device->GetDeviceRemovedReason();
		blog(LOG_ERROR, "  Device Removed Reason: %08lX",
				DeviceRemovedReason);
	}
}

static const IID dxgiFactory2 =
{0x50c83a1c, 0xe072, 0x4c48, {0x87, 0xb0, 0x36, 0x30, 0xfa, 0x36, 0xa6, 0xd0}};


gs_obj::gs_obj(gs_device_t *device_, gs_type type) :
	device    (device_),
	obj_type  (type)
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

static inline void make_swap_desc(DXGI_SWAP_CHAIN_DESC &desc,
		const gs_init_data *data)
{
	memset(&desc, 0, sizeof(desc));
	desc.BufferCount       = data->num_backbuffers;
	desc.BufferDesc.Format = ConvertGSTextureFormat(data->format);
	desc.BufferDesc.Width  = data->cx;
	desc.BufferDesc.Height = data->cy;
	desc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.OutputWindow      = (HWND)data->window.hwnd;
	desc.SampleDesc.Count  = 1;
	desc.Windowed          = true;
}

void gs_swap_chain::InitTarget(uint32_t cx, uint32_t cy)
{
	HRESULT hr;

	target.width  = cx;
	target.height = cy;

	hr = swap->GetBuffer(0, __uuidof(ID3D11Texture2D),
			(void**)target.texture.Assign());
	if (FAILED(hr))
		throw HRError("Failed to get swap buffer texture", hr);

	hr = device->device->CreateRenderTargetView(target.texture, NULL,
			target.renderTarget[0].Assign());
	if (FAILED(hr))
		throw HRError("Failed to create swap render target view", hr);
}

void gs_swap_chain::InitZStencilBuffer(uint32_t cx, uint32_t cy)
{
	zs.width  = cx;
	zs.height = cy;

	if (zs.format != GS_ZS_NONE && cx != 0 && cy != 0) {
		zs.InitBuffer();
	} else {
		zs.texture.Clear();
		zs.view.Clear();
	}
}

void gs_swap_chain::Resize(uint32_t cx, uint32_t cy)
{
	RECT clientRect;
	HRESULT hr;

	target.texture.Clear();
	target.renderTarget[0].Clear();
	zs.texture.Clear();
	zs.view.Clear();

	initData.cx = cx;
	initData.cy = cy;

	if (cx == 0 || cy == 0) {
		GetClientRect(hwnd, &clientRect);
		if (cx == 0) cx = clientRect.right;
		if (cy == 0) cy = clientRect.bottom;
	}

	hr = swap->ResizeBuffers(numBuffers, cx, cy, target.dxgiFormat, 0);
	if (FAILED(hr))
		throw HRError("Failed to resize swap buffers", hr);

	InitTarget(cx, cy);
	InitZStencilBuffer(cx, cy);
}

void gs_swap_chain::Init()
{
	target.device         = device;
	target.isRenderTarget = true;
	target.format         = initData.format;
	target.dxgiFormat     = ConvertGSTextureFormat(initData.format);
	InitTarget(initData.cx, initData.cy);

	zs.device     = device;
	zs.format     = initData.zsformat;
	zs.dxgiFormat = ConvertGSZStencilFormat(initData.zsformat);
	InitZStencilBuffer(initData.cx, initData.cy);
}

gs_swap_chain::gs_swap_chain(gs_device *device, const gs_init_data *data)
	: gs_obj     (device, gs_type::gs_swap_chain),
	  numBuffers (data->num_backbuffers),
	  hwnd       ((HWND)data->window.hwnd),
	  initData   (*data)
{
	HRESULT hr;

	make_swap_desc(swapDesc, data);
	hr = device->factory->CreateSwapChain(device->device, &swapDesc,
			swap.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create swap chain", hr);

	Init();
}

void gs_device::InitCompiler()
{
	char d3dcompiler[40] = {};
	int ver = 49;

	while (ver > 30) {
		sprintf(d3dcompiler, "D3DCompiler_%02d.dll", ver);

		HMODULE module = LoadLibraryA(d3dcompiler);
		if (module) {
			d3dCompile = (pD3DCompile)GetProcAddress(module,
					"D3DCompile");

#ifdef DISASSEMBLE_SHADERS
			d3dDisassemble = (pD3DDisassemble)GetProcAddress(
					module, "D3DDisassemble");
#endif
			if (d3dCompile) {
				return;
			}

			FreeLibrary(module);
		}

		ver--;
	}

	throw "Could not find any D3DCompiler libraries";
}

void gs_device::InitFactory(uint32_t adapterIdx)
{
	HRESULT hr;
	IID factoryIID = (GetWinVer() >= 0x602) ? dxgiFactory2 :
		__uuidof(IDXGIFactory1);

	hr = CreateDXGIFactory1(factoryIID, (void**)factory.Assign());
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to create DXGIFactory", hr);

	hr = factory->EnumAdapters1(adapterIdx, &adapter);
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to enumerate DXGIAdapter", hr);
}

const static D3D_FEATURE_LEVEL featureLevels[] =
{
	D3D_FEATURE_LEVEL_11_0,
	D3D_FEATURE_LEVEL_10_1,
	D3D_FEATURE_LEVEL_10_0,
	D3D_FEATURE_LEVEL_9_3,
};

void gs_device::InitDevice(uint32_t adapterIdx)
{
	wstring adapterName;
	DXGI_ADAPTER_DESC desc;
	D3D_FEATURE_LEVEL levelUsed = D3D_FEATURE_LEVEL_9_3;
	HRESULT hr = 0;

	adpIdx = adapterIdx;

	uint32_t createFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef _DEBUG
	//createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	adapterName = (adapter->GetDesc(&desc) == S_OK) ? desc.Description :
		L"<unknown>";

	char *adapterNameUTF8;
	os_wcs_to_utf8_ptr(adapterName.c_str(), 0, &adapterNameUTF8);
	blog(LOG_INFO, "Loading up D3D11 on adapter %s (%" PRIu32 ")",
			adapterNameUTF8, adapterIdx);
	bfree(adapterNameUTF8);

	hr = D3D11CreateDevice(adapter, D3D_DRIVER_TYPE_UNKNOWN,
			NULL, createFlags, featureLevels,
			sizeof(featureLevels) / sizeof(D3D_FEATURE_LEVEL),
			D3D11_SDK_VERSION, device.Assign(),
			&levelUsed, context.Assign());
	if (FAILED(hr))
		throw UnsupportedHWError("Failed to create device", hr);

	blog(LOG_INFO, "D3D11 loaded successfully, feature level used: %u",
			(unsigned int)levelUsed);
}

static inline void ConvertStencilSide(D3D11_DEPTH_STENCILOP_DESC &desc,
		const StencilSide &side)
{
	desc.StencilFunc        = ConvertGSDepthTest(side.test);
	desc.StencilFailOp      = ConvertGSStencilOp(side.fail);
	desc.StencilDepthFailOp = ConvertGSStencilOp(side.zfail);
	desc.StencilPassOp      = ConvertGSStencilOp(side.zpass);
}

ID3D11DepthStencilState *gs_device::AddZStencilState()
{
	HRESULT hr;
	D3D11_DEPTH_STENCIL_DESC dsd;
	ID3D11DepthStencilState *state;

	dsd.DepthEnable      = zstencilState.depthEnabled;
	dsd.DepthFunc        = ConvertGSDepthTest(zstencilState.depthFunc);
	dsd.DepthWriteMask   = zstencilState.depthWriteEnabled ?
		D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	dsd.StencilEnable    = zstencilState.stencilEnabled;
	dsd.StencilReadMask  = D3D11_DEFAULT_STENCIL_READ_MASK;
	dsd.StencilWriteMask = zstencilState.stencilWriteEnabled ?
		D3D11_DEFAULT_STENCIL_WRITE_MASK : 0;
	ConvertStencilSide(dsd.FrontFace, zstencilState.stencilFront);
	ConvertStencilSide(dsd.BackFace,  zstencilState.stencilBack);

	SavedZStencilState savedState(zstencilState, dsd);
	hr = device->CreateDepthStencilState(&dsd, savedState.state.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create depth stencil state", hr);

	state = savedState.state;
	zstencilStates.push_back(savedState);

	return state;
}

ID3D11RasterizerState *gs_device::AddRasterState()
{
	HRESULT hr;
	D3D11_RASTERIZER_DESC rd;
	ID3D11RasterizerState *state;

	memset(&rd, 0, sizeof(rd));
	/* use CCW to convert to a right-handed coordinate system */
	rd.FrontCounterClockwise = true;
	rd.FillMode              = D3D11_FILL_SOLID;
	rd.CullMode              = ConvertGSCullMode(rasterState.cullMode);
	rd.DepthClipEnable       = true;
	rd.ScissorEnable         = rasterState.scissorEnabled;

	SavedRasterState savedState(rasterState, rd);
	hr = device->CreateRasterizerState(&rd, savedState.state.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create rasterizer state", hr);

	state = savedState.state;
	rasterStates.push_back(savedState);

	return state;
}

ID3D11BlendState *gs_device::AddBlendState()
{
	HRESULT hr;
	D3D11_BLEND_DESC bd;
	ID3D11BlendState *state;

	memset(&bd, 0, sizeof(bd));
	for (int i = 0; i < 8; i++) {
		bd.RenderTarget[i].BlendEnable    = blendState.blendEnabled;
		bd.RenderTarget[i].BlendOp        = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[i].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
		bd.RenderTarget[i].SrcBlend =
			ConvertGSBlendType(blendState.srcFactorC);
		bd.RenderTarget[i].DestBlend =
			ConvertGSBlendType(blendState.destFactorC);
		bd.RenderTarget[i].SrcBlendAlpha =
			ConvertGSBlendType(blendState.srcFactorA);
		bd.RenderTarget[i].DestBlendAlpha =
			ConvertGSBlendType(blendState.destFactorA);
		bd.RenderTarget[i].RenderTargetWriteMask =
			D3D11_COLOR_WRITE_ENABLE_ALL;
	}

	SavedBlendState savedState(blendState, bd);
	hr = device->CreateBlendState(&bd, savedState.state.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create blend state", hr);

	state = savedState.state;
	blendStates.push_back(savedState);

	return state;
}

void gs_device::UpdateZStencilState()
{
	ID3D11DepthStencilState *state = NULL;

	if (!zstencilStateChanged)
		return;

	for (size_t i = 0; i < zstencilStates.size(); i++) {
		SavedZStencilState &s = zstencilStates[i];
		if (memcmp(&s, &zstencilState, sizeof(zstencilState)) == 0) {
			state = s.state;
			break;
		}
	}

	if (!state)
		state = AddZStencilState();

	if (state != curDepthStencilState) {
		context->OMSetDepthStencilState(state, 0);
		curDepthStencilState = state;
	}

	zstencilStateChanged = false;
}

void gs_device::UpdateRasterState()
{
	ID3D11RasterizerState *state = NULL;

	if (!rasterStateChanged)
		return;

	for (size_t i = 0; i < rasterStates.size(); i++) {
		SavedRasterState &s = rasterStates[i];
		if (memcmp(&s, &rasterState, sizeof(rasterState)) == 0) {
			state = s.state;
			break;
		}
	}

	if (!state)
		state = AddRasterState();

	if (state != curRasterState) {
		context->RSSetState(state);
		curRasterState = state;
	}

	rasterStateChanged = false;
}

void gs_device::UpdateBlendState()
{
	ID3D11BlendState *state = NULL;

	if (!blendStateChanged)
		return;

	for (size_t i = 0; i < blendStates.size(); i++) {
		SavedBlendState &s = blendStates[i];
		if (memcmp(&s, &blendState, sizeof(blendState)) == 0) {
			state = s.state;
			break;
		}
	}

	if (!state)
		state = AddBlendState();

	if (state != curBlendState) {
		float f[4] = {1.0f, 1.0f, 1.0f, 1.0f};
		context->OMSetBlendState(state, f, 0xFFFFFFFF);
		curBlendState = state;
	}

	blendStateChanged = false;
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

	if (curVertexShader->viewProj)
		gs_shader_set_matrix4(curVertexShader->viewProj,
				&curViewProjMatrix);
}

gs_device::gs_device(uint32_t adapterIdx)
	: curToplogy           (D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED)
{
	matrix4_identity(&curProjMatrix);
	matrix4_identity(&curViewMatrix);
	matrix4_identity(&curViewProjMatrix);

	memset(&viewport, 0, sizeof(viewport));

	for (size_t i = 0; i < GS_MAX_TEXTURES; i++) {
		curTextures[i] = NULL;
		curSamplers[i] = NULL;
	}

	InitCompiler();
	InitFactory(adapterIdx);
	InitDevice(adapterIdx);
	device_set_render_target(this, NULL, NULL);
}

gs_device::~gs_device()
{
	context->ClearState();
}

const char *device_get_name(void)
{
	return "Direct3D 11";
}

int device_get_type(void)
{
	return GS_DEVICE_DIRECT3D_11;
}

const char *device_preprocessor_name(void)
{
	return "_D3D11";
}

static inline void EnumD3DAdapters(
		bool (*callback)(void*, const char*, uint32_t),
		void *param)
{
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i = 0;

	IID factoryIID = (GetWinVer() >= 0x602) ? dxgiFactory2 :
		__uuidof(IDXGIFactory1);

	hr = CreateDXGIFactory1(factoryIID, (void**)factory.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create DXGIFactory", hr);

	while (factory->EnumAdapters1(i++, adapter.Assign()) == S_OK) {
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		os_wcs_to_utf8(desc.Description, 0, name, sizeof(name));

		if (!callback(param, name, i - 1))
			break;
	}
}

bool device_enum_adapters(
		bool (*callback)(void *param, const char *name, uint32_t id),
		void *param)
{
	try {
		EnumD3DAdapters(callback, param);
		return true;

	} catch (HRError error) {
		blog(LOG_WARNING, "Failed enumerating devices: %s (%08lX)",
				error.str, error.hr);
		return false;
	}
}

static inline void LogAdapterMonitors(IDXGIAdapter1 *adapter)
{
	UINT i = 0;
	ComPtr<IDXGIOutput> output;

	while (adapter->EnumOutputs(i++, &output) == S_OK) {
		DXGI_OUTPUT_DESC desc;
		if (FAILED(output->GetDesc(&desc)))
			continue;

		RECT rect = desc.DesktopCoordinates;
		blog(LOG_INFO, "\t  output %u: "
				"pos={%d, %d}, "
				"size={%d, %d}, "
				"attached=%s",
				i,
				rect.left, rect.top,
				rect.right - rect.left, rect.bottom - rect.top,
				desc.AttachedToDesktop ? "true" : "false");
	}
}

static inline void LogD3DAdapters()
{
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i = 0;

	blog(LOG_INFO, "Available Video Adapters: ");

	IID factoryIID = (GetWinVer() >= 0x602) ? dxgiFactory2 :
		__uuidof(IDXGIFactory1);

	hr = CreateDXGIFactory1(factoryIID, (void**)factory.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create DXGIFactory", hr);

	while (factory->EnumAdapters1(i++, adapter.Assign()) == S_OK) {
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		os_wcs_to_utf8(desc.Description, 0, name, sizeof(name));
		blog(LOG_INFO, "\tAdapter %u: %s", i, name);
		blog(LOG_INFO, "\t  Dedicated VRAM: %u",
				desc.DedicatedVideoMemory);
		blog(LOG_INFO, "\t  Shared VRAM:    %u",
				desc.SharedSystemMemory);

		LogAdapterMonitors(adapter);
	}
}

int device_create(gs_device_t **p_device, uint32_t adapter)
{
	gs_device *device = NULL;
	int errorcode = GS_SUCCESS;

	try {
		blog(LOG_INFO, "---------------------------------");
		blog(LOG_INFO, "Initializing D3D11...");
		LogD3DAdapters();

		device = new gs_device(adapter);

	} catch (UnsupportedHWError error) {
		blog(LOG_ERROR, "device_create (D3D11): %s (%08lX)", error.str,
				error.hr);
		errorcode = GS_ERROR_NOT_SUPPORTED;

	} catch (HRError error) {
		blog(LOG_ERROR, "device_create (D3D11): %s (%08lX)", error.str,
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

gs_swapchain_t *device_swapchain_create(gs_device_t *device,
		const struct gs_init_data *data)
{
	gs_swap_chain *swap = NULL;

	try {
		swap = new gs_swap_chain(device, data);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_swapchain_create (D3D11): %s (%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	}

	return swap;
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	if (!device->curSwapChain) {
		blog(LOG_WARNING, "device_resize (D3D11): No active swap");
		return;
	}

	try {
		ID3D11RenderTargetView *renderView = NULL;
		ID3D11DepthStencilView *depthView  = NULL;
		int i = device->curRenderSide;

		device->context->OMSetRenderTargets(1, &renderView, depthView);
		device->curSwapChain->Resize(cx, cy);

		if (device->curRenderTarget)
			renderView = device->curRenderTarget->renderTarget[i];
		if (device->curZStencilBuffer)
			depthView  = device->curZStencilBuffer->view;
		device->context->OMSetRenderTargets(1, &renderView, depthView);

	} catch (HRError error) {
		blog(LOG_ERROR, "device_resize (D3D11): %s (%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	}
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	if (device->curSwapChain) {
		*cx = device->curSwapChain->target.width;
		*cy = device->curSwapChain->target.height;
	} else {
		blog(LOG_ERROR, "device_get_size (D3D11): no active swap");
		*cx = 0;
		*cy = 0;
	}
}

uint32_t device_get_width(const gs_device_t *device)
{
	if (device->curSwapChain) {
		return device->curSwapChain->target.width;
	} else {
		blog(LOG_ERROR, "device_get_size (D3D11): no active swap");
		return 0;
	}
}

uint32_t device_get_height(const gs_device_t *device)
{
	if (device->curSwapChain) {
		return device->curSwapChain->target.height;
	} else {
		blog(LOG_ERROR, "device_get_size (D3D11): no active swap");
		return 0;
	}
}

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width,
		uint32_t height, enum gs_color_format color_format,
		uint32_t levels, const uint8_t **data, uint32_t flags)
{
	gs_texture *texture = NULL;
	try {
		texture = new gs_texture_2d(device, width, height, color_format,
				levels, data, flags, GS_TEXTURE_2D, false,
				false);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_texture_create (D3D11): %s (%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_texture_create (D3D11): %s", error);
	}

	return texture;
}

gs_texture_t *device_cubetexture_create(gs_device_t *device, uint32_t size,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags)
{
	gs_texture *texture = NULL;
	try {
		texture = new gs_texture_2d(device, size, size, color_format,
				levels, data, flags, GS_TEXTURE_CUBE, false,
				false);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_cubetexture_create (D3D11): %s "
		                "(%08lX)",
		                error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_cubetexture_create (D3D11): %s",
				error);
	}

	return texture;
}

gs_texture_t *device_voltexture_create(gs_device_t *device, uint32_t width,
		uint32_t height, uint32_t depth,
		enum gs_color_format color_format, uint32_t levels,
		const uint8_t **data, uint32_t flags)
{
	/* TODO */
	UNUSED_PARAMETER(device);
	UNUSED_PARAMETER(width);
	UNUSED_PARAMETER(height);
	UNUSED_PARAMETER(depth);
	UNUSED_PARAMETER(color_format);
	UNUSED_PARAMETER(levels);
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(flags);
	return NULL;
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width,
		uint32_t height, enum gs_zstencil_format format)
{
	gs_zstencil_buffer *zstencil = NULL;
	try {
		zstencil = new gs_zstencil_buffer(device, width, height,
				format);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_zstencil_create (D3D11): %s (%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	}

	return zstencil;
}

gs_stagesurf_t *device_stagesurface_create(gs_device_t *device, uint32_t width,
		uint32_t height, enum gs_color_format color_format)
{
	gs_stage_surface *surf = NULL;
	try {
		surf = new gs_stage_surface(device, width, height,
				color_format);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_stagesurface_create (D3D11): %s "
		                "(%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	}

	return surf;
}

gs_samplerstate_t *device_samplerstate_create(gs_device_t *device,
		const struct gs_sampler_info *info)
{
	gs_sampler_state *ss = NULL;
	try {
		ss = new gs_sampler_state(device, info);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_samplerstate_create (D3D11): %s "
		                "(%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	}

	return ss;
}

gs_shader_t *device_vertexshader_create(gs_device_t *device,
		const char *shader_string, const char *file,
		char **error_string)
{
	gs_vertex_shader *shader = NULL;
	try {
		shader = new gs_vertex_shader(device, file, shader_string);

	} catch (HRError error) {
		blog(LOG_ERROR, "device_vertexshader_create (D3D11): %s "
		                "(%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);

	} catch (ShaderError error) {
		const char *buf = (const char*)error.errors->GetBufferPointer();
		if (error_string)
			*error_string = bstrdup(buf);
		blog(LOG_ERROR, "device_vertexshader_create (D3D11): "
		                "Compile warnings/errors for %s:\n%s",
		                file, buf);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexshader_create (D3D11): %s",
				error);
	}

	return shader;
}

gs_shader_t *device_pixelshader_create(gs_device_t *device,
		const char *shader_string, const char *file,
		char **error_string)
{
	gs_pixel_shader *shader = NULL;
	try {
		shader = new gs_pixel_shader(device, file, shader_string);

	} catch (HRError error) {
		blog(LOG_ERROR, "device_pixelshader_create (D3D11): %s "
		                "(%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);

	} catch (ShaderError error) {
		const char *buf = (const char*)error.errors->GetBufferPointer();
		if (error_string)
			*error_string = bstrdup(buf);
		blog(LOG_ERROR, "device_pixelshader_create (D3D11): "
		                "Compiler warnings/errors for %s:\n%s",
		                file, buf);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_pixelshader_create (D3D11): %s",
				error);
	}

	return shader;
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device,
		struct gs_vb_data *data, uint32_t flags)
{
	gs_vertex_buffer *buffer = NULL;
	try {
		buffer = new gs_vertex_buffer(device, data, flags);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_vertexbuffer_create (D3D11): %s "
		                "(%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexbuffer_create (D3D11): %s",
				error);
	}

	return buffer;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device,
		enum gs_index_type type, void *indices, size_t num,
		uint32_t flags)
{
	gs_index_buffer *buffer = NULL;
	try {
		buffer = new gs_index_buffer(device, type, indices, num, flags);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_indexbuffer_create (D3D11): %s (%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	}

	return buffer;
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	return texture->type;
}

void gs_device::LoadVertexBufferData()
{
	if (curVertexBuffer == lastVertexBuffer &&
	    curVertexShader == lastVertexShader)
		return;

	vector<ID3D11Buffer*> buffers;
	vector<uint32_t> strides;
	vector<uint32_t> offsets;

	if (curVertexBuffer && curVertexShader) {
		curVertexBuffer->MakeBufferList(curVertexShader,
				buffers, strides);
	} else {
		size_t buffersToClear = curVertexShader
			? curVertexShader->NumBuffersExpected() : 0;
		buffers.resize(buffersToClear);
		strides.resize(buffersToClear);
	}

	offsets.resize(buffers.size());
	context->IASetVertexBuffers(0, (UINT)buffers.size(),
			buffers.data(), strides.data(), offsets.data());

	lastVertexBuffer = curVertexBuffer;
	lastVertexShader = curVertexShader;
}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	if (device->curVertexBuffer == vertbuffer)
		return;

	device->curVertexBuffer = vertbuffer;
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	DXGI_FORMAT format;
	ID3D11Buffer *buffer;

	if (device->curIndexBuffer == indexbuffer)
		return;

	if (indexbuffer) {
		switch (indexbuffer->indexSize) {
		case 2: format = DXGI_FORMAT_R16_UINT; break;
		default:
		case 4: format = DXGI_FORMAT_R32_UINT; break;
		}

		buffer = indexbuffer->indexBuffer;
	} else {
		buffer = NULL;
		format = DXGI_FORMAT_R32_UINT;
	}

	device->curIndexBuffer = indexbuffer;
	device->context->IASetIndexBuffer(buffer, format, 0);
}

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	ID3D11ShaderResourceView *view = NULL;

	if (device->curTextures[unit] == tex)
		return;

	if (tex)
		view = tex->shaderRes;

	device->curTextures[unit] = tex;
	device->context->PSSetShaderResources(unit, 1, &view);
}

void device_load_samplerstate(gs_device_t *device,
		gs_samplerstate_t *samplerstate, int unit)
{
	ID3D11SamplerState *state = NULL;

	if (device->curSamplers[unit] == samplerstate)
		return;

	if (samplerstate)
		state = samplerstate->state;

	device->curSamplers[unit] = samplerstate;
	device->context->PSSetSamplers(unit, 1, &state);
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	ID3D11VertexShader *shader    = NULL;
	ID3D11InputLayout  *layout    = NULL;
	ID3D11Buffer       *constants = NULL;

	if (device->curVertexShader == vertshader)
		return;

	gs_vertex_shader *vs = static_cast<gs_vertex_shader*>(vertshader);
	gs_vertex_buffer *curVB = device->curVertexBuffer;

	if (vertshader) {
		if (vertshader->type != GS_SHADER_VERTEX) {
			blog(LOG_ERROR, "device_load_vertexshader (D3D11): "
			                "Specified shader is not a vertex "
			                "shader");
			return;
		}

		shader    = vs->shader;
		layout    = vs->layout;
		constants = vs->constants;
	}

	device->curVertexShader = vs;
	device->context->VSSetShader(shader, NULL, 0);
	device->context->IASetInputLayout(layout);
	device->context->VSSetConstantBuffers(0, 1, &constants);
}

static inline void clear_textures(gs_device_t *device)
{
	ID3D11ShaderResourceView *views[GS_MAX_TEXTURES];
	memset(views,               0, sizeof(views));
	memset(device->curTextures, 0, sizeof(device->curTextures));
	device->context->PSSetShaderResources(0, GS_MAX_TEXTURES, views);
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	ID3D11PixelShader  *shader    = NULL;
	ID3D11Buffer       *constants = NULL;
	ID3D11SamplerState *states[GS_MAX_TEXTURES];

	if (device->curPixelShader == pixelshader)
		return;

	gs_pixel_shader *ps = static_cast<gs_pixel_shader*>(pixelshader);

	if (pixelshader) {
		if (pixelshader->type != GS_SHADER_PIXEL) {
			blog(LOG_ERROR, "device_load_pixelshader (D3D11): "
			                "Specified shader is not a pixel "
			                "shader");
			return;
		}

		shader    = ps->shader;
		constants = ps->constants;
		ps->GetSamplerStates(states);
	} else {
		memset(states, 0, sizeof(states));
	}

	clear_textures(device);

	device->curPixelShader = ps;
	device->context->PSSetShader(shader, NULL, 0);
	device->context->PSSetConstantBuffers(0, 1, &constants);
	device->context->PSSetSamplers(0, GS_MAX_TEXTURES, states);

	for (int i = 0; i < GS_MAX_TEXTURES; i++)
		if (device->curSamplers[i] &&
				device->curSamplers[i]->state != states[i])
			device->curSamplers[i] = nullptr;
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
	if (device->curSwapChain) {
		if (!tex)
			tex = &device->curSwapChain->target;
		if (!zstencil)
			zstencil = &device->curSwapChain->zs;
	}

	if (device->curRenderTarget   == tex &&
	    device->curZStencilBuffer == zstencil)
		return;

	if (tex && tex->type != GS_TEXTURE_2D) {
		blog(LOG_ERROR, "device_set_render_target (D3D11): "
		                "texture is not a 2D texture");
		return;
	}

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	if (tex2d && !tex2d->renderTarget[0]) {
		blog(LOG_ERROR, "device_set_render_target (D3D11): "
		                "texture is not a render target");
		return;
	}

	ID3D11RenderTargetView *rt = tex2d ? tex2d->renderTarget[0] : nullptr;

	device->curRenderTarget   = tex2d;
	device->curRenderSide     = 0;
	device->curZStencilBuffer = zstencil;
	device->context->OMSetRenderTargets(1, &rt,
			zstencil ? zstencil->view : nullptr);
}

void device_set_cube_render_target(gs_device_t *device, gs_texture_t *tex,
		int side, gs_zstencil_t *zstencil)
{
	if (device->curSwapChain) {
		if (!tex) {
			tex = &device->curSwapChain->target;
			side = 0;
		}

		if (!zstencil)
			zstencil = &device->curSwapChain->zs;
	}

	if (device->curRenderTarget   == tex  &&
	    device->curRenderSide     == side &&
	    device->curZStencilBuffer == zstencil)
		return;

	if (tex->type != GS_TEXTURE_CUBE) {
		blog(LOG_ERROR, "device_set_cube_render_target (D3D11): "
		                "texture is not a cube texture");
		return;
	}

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	if (!tex2d->renderTarget[side]) {
		blog(LOG_ERROR, "device_set_cube_render_target (D3D11): "
				"texture is not a render target");
		return;
	}

	ID3D11RenderTargetView *rt = tex2d->renderTarget[0];

	device->curRenderTarget   = tex2d;
	device->curRenderSide     = side;
	device->curZStencilBuffer = zstencil;
	device->context->OMSetRenderTargets(1, &rt, zstencil->view);
}

inline void gs_device::CopyTex(ID3D11Texture2D *dst,
		uint32_t dst_x, uint32_t dst_y,
		gs_texture_t *src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h)
{
	if (src->type != GS_TEXTURE_2D)
		throw "Source texture must be a 2D texture";

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(src);

	if (dst_x == 0 && dst_y == 0 &&
	    src_x == 0 && src_y == 0 &&
	    src_w == 0 && src_h == 0) {
		context->CopyResource(dst, tex2d->texture);
	} else {
		D3D11_BOX sbox;

		sbox.left = src_x;
		if (src_w > 0)
			sbox.right = src_x + src_w;
		else
			sbox.right = tex2d->width - 1;

		sbox.top = src_y;
		if (src_h > 0)
			sbox.bottom = src_y + src_h;
		else
			sbox.bottom = tex2d->height - 1;

		sbox.front = 0;
		sbox.back = 1;

		context->CopySubresourceRegion(dst, 0, dst_x, dst_y, 0,
				tex2d->texture, 0, &sbox);
	}
}

void device_copy_texture_region(gs_device_t *device,
		gs_texture_t *dst, uint32_t dst_x, uint32_t dst_y,
		gs_texture_t *src, uint32_t src_x, uint32_t src_y,
		uint32_t src_w, uint32_t src_h)
{
	try {
		gs_texture_2d *src2d = static_cast<gs_texture_2d*>(src);
		gs_texture_2d *dst2d = static_cast<gs_texture_2d*>(dst);

		if (!src)
			throw "Source texture is NULL";
		if (!dst)
			throw "Destination texture is NULL";
		if (src->type != GS_TEXTURE_2D || dst->type != GS_TEXTURE_2D)
			throw "Source and destination textures must be a 2D "
			      "textures";
		if (dst->format != src->format)
			throw "Source and destination formats do not match";

		/* apparently casting to the same type that the variable
		 * already exists as is supposed to prevent some warning
		 * when used with the conditional operator? */
		uint32_t copyWidth = (uint32_t)src_w ?
			(uint32_t)src_w : (src2d->width - src_x);
		uint32_t copyHeight = (uint32_t)src_h ?
			(uint32_t)src_h : (src2d->height - src_y);

		uint32_t dstWidth  = dst2d->width  - dst_x;
		uint32_t dstHeight = dst2d->height - dst_y;

		if (dstWidth < copyWidth || dstHeight < copyHeight)
			throw "Destination texture region is not big "
			      "enough to hold the source region";

		if (dst_x == 0 && dst_y == 0 &&
		    src_x == 0 && src_y == 0 &&
		    src_w == 0 && src_h == 0) {
			copyWidth  = 0;
			copyHeight = 0;
		}

		device->CopyTex(dst2d->texture, dst_x, dst_y,
				src, src_x, src_y, copyWidth, copyHeight);

	} catch(const char *error) {
		blog(LOG_ERROR, "device_copy_texture (D3D11): %s", error);
	}
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst,
		gs_texture_t *src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst,
		gs_texture_t *src)
{
	try {
		gs_texture_2d *src2d = static_cast<gs_texture_2d*>(src);

		if (!src)
			throw "Source texture is NULL";
		if (src->type != GS_TEXTURE_2D)
			throw "Source texture must be a 2D texture";
		if (!dst)
			throw "Destination surface is NULL";
		if (dst->format != src->format)
			throw "Source and destination formats do not match";
		if (dst->width  != src2d->width ||
		    dst->height != src2d->height)
			throw "Source and destination must have the same "
			      "dimensions";

		device->CopyTex(dst->texture, 0, 0, src, 0, 0, 0, 0);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_copy_texture (D3D11): %s", error);
	}
}

void device_begin_scene(gs_device_t *device)
{
	clear_textures(device);
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode,
		uint32_t start_vert, uint32_t num_verts)
{
	try {
		if (!device->curVertexShader)
			throw "No vertex shader specified";

		if (!device->curPixelShader)
			throw "No pixel shader specified";

		if (!device->curVertexBuffer)
			throw "No vertex buffer specified";

		if (!device->curSwapChain && !device->curRenderTarget)
			throw "No render target or swap chain to render to";

		gs_effect_t *effect = gs_get_effect();
		if (effect)
			gs_effect_update_params(effect);

		device->LoadVertexBufferData();
		device->UpdateBlendState();
		device->UpdateRasterState();
		device->UpdateZStencilState();
		device->UpdateViewProjMatrix();
		device->curVertexShader->UploadParams();
		device->curPixelShader->UploadParams();

	} catch (const char *error) {
		blog(LOG_ERROR, "device_draw (D3D11): %s", error);
		return;

	} catch (HRError error) {
		blog(LOG_ERROR, "device_draw (D3D11): %s (%08lX)", error.str,
				error.hr);
		LogD3D11ErrorDetails(error, device);
		return;
	}

	D3D11_PRIMITIVE_TOPOLOGY newTopology = ConvertGSTopology(draw_mode);
	if (device->curToplogy != newTopology) {
		device->context->IASetPrimitiveTopology(newTopology);
		device->curToplogy = newTopology;
	}

	if (device->curIndexBuffer) {
		if (num_verts == 0)
			num_verts = (uint32_t)device->curIndexBuffer->num;
		device->context->DrawIndexed(num_verts, start_vert, 0);
	} else {
		if (num_verts == 0)
			num_verts = (uint32_t)device->curVertexBuffer->numVerts;
		device->context->Draw(num_verts, start_vert);
	}
}

void device_end_scene(gs_device_t *device)
{
	/* does nothing in D3D11 */
	UNUSED_PARAMETER(device);
}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	gs_texture_t  *target = device->curRenderTarget;
	gs_zstencil_t *zs     = device->curZStencilBuffer;
	bool is_cube = device->curRenderTarget ?
		(device->curRenderTarget->type == GS_TEXTURE_CUBE) : false;

	if (device->curSwapChain) {
		if (target == &device->curSwapChain->target)
			target = NULL;
		if (zs == &device->curSwapChain->zs)
			zs = NULL;
	}

	device->curSwapChain = swapchain;

	if (is_cube)
		device_set_cube_render_target(device, target,
				device->curRenderSide, zs);
	else
		device_set_render_target(device, target, zs);
}

void device_clear(gs_device_t *device, uint32_t clear_flags,
		const struct vec4 *color, float depth, uint8_t stencil)
{
	int side = device->curRenderSide;
	if ((clear_flags & GS_CLEAR_COLOR) != 0 && device->curRenderTarget)
		device->context->ClearRenderTargetView(
				device->curRenderTarget->renderTarget[side],
				color->ptr);

	if (device->curZStencilBuffer) {
		uint32_t flags = 0;
		if ((clear_flags & GS_CLEAR_DEPTH) != 0)
			flags |= D3D11_CLEAR_DEPTH;
		if ((clear_flags & GS_CLEAR_STENCIL) != 0)
			flags |= D3D11_CLEAR_STENCIL;

		if (flags && device->curZStencilBuffer->view)
			device->context->ClearDepthStencilView(
					device->curZStencilBuffer->view,
					flags, depth, stencil);
	}
}

void device_present(gs_device_t *device)
{
	HRESULT hr;

	if (device->curSwapChain) {
		hr = device->curSwapChain->swap->Present(0, 0);
		if (hr == DXGI_ERROR_DEVICE_REMOVED ||
		    hr == DXGI_ERROR_DEVICE_RESET) {
			device->RebuildDevice();
		}
	} else {
		blog(LOG_WARNING, "device_present (D3D11): No active swap");
	}
}

void device_flush(gs_device_t *device)
{
	device->context->Flush();
}

void device_set_cull_mode(gs_device_t *device, enum gs_cull_mode mode)
{
	if (mode == device->rasterState.cullMode)
		return;

	device->rasterState.cullMode = mode;
	device->rasterStateChanged = true;
}

enum gs_cull_mode device_get_cull_mode(const gs_device_t *device)
{
	return device->rasterState.cullMode;
}

void device_enable_blending(gs_device_t *device, bool enable)
{
	if (enable == device->blendState.blendEnabled)
		return;

	device->blendState.blendEnabled = enable;
	device->blendStateChanged = true;
}

void device_enable_depth_test(gs_device_t *device, bool enable)
{
	if (enable == device->zstencilState.depthEnabled)
		return;

	device->zstencilState.depthEnabled = enable;
	device->zstencilStateChanged = true;
}

void device_enable_stencil_test(gs_device_t *device, bool enable)
{
	if (enable == device->zstencilState.stencilEnabled)
		return;

	device->zstencilState.stencilEnabled = enable;
	device->zstencilStateChanged = true;
}

void device_enable_stencil_write(gs_device_t *device, bool enable)
{
	if (enable == device->zstencilState.stencilWriteEnabled)
		return;

	device->zstencilState.stencilWriteEnabled = enable;
	device->zstencilStateChanged = true;
}

void device_enable_color(gs_device_t *device, bool red, bool green,
		bool blue, bool alpha)
{
	if (device->blendState.redEnabled   == red   &&
	    device->blendState.greenEnabled == green &&
	    device->blendState.blueEnabled  == blue  &&
	    device->blendState.alphaEnabled == alpha)
		return;

	device->blendState.redEnabled   = red;
	device->blendState.greenEnabled = green;
	device->blendState.blueEnabled  = blue;
	device->blendState.alphaEnabled = alpha;
	device->blendStateChanged       = true;
}

void device_blend_function(gs_device_t *device, enum gs_blend_type src,
		enum gs_blend_type dest)
{
	if (device->blendState.srcFactorC  == src &&
	    device->blendState.destFactorC == dest &&
	    device->blendState.srcFactorA  == src &&
	    device->blendState.destFactorA == dest)
		return;

	device->blendState.srcFactorC = src;
	device->blendState.destFactorC= dest;
	device->blendState.srcFactorA = src;
	device->blendState.destFactorA= dest;
	device->blendStateChanged     = true;
}

void device_blend_function_separate(gs_device_t *device,
		enum gs_blend_type src_c, enum gs_blend_type dest_c,
		enum gs_blend_type src_a, enum gs_blend_type dest_a)
{
	if (device->blendState.srcFactorC  == src_c &&
	    device->blendState.destFactorC == dest_c &&
	    device->blendState.srcFactorA  == src_a &&
	    device->blendState.destFactorA == dest_a)
		return;

	device->blendState.srcFactorC  = src_c;
	device->blendState.destFactorC = dest_c;
	device->blendState.srcFactorA  = src_a;
	device->blendState.destFactorA = dest_a;
	device->blendStateChanged      = true;
}

void device_depth_function(gs_device_t *device, enum gs_depth_test test)
{
	if (device->zstencilState.depthFunc == test)
		return;

	device->zstencilState.depthFunc = test;
	device->zstencilStateChanged    = true;
}

static inline void update_stencilside_test(gs_device_t *device,
		StencilSide &side, gs_depth_test test)
{
	if (side.test == test)
		return;

	side.test = test;
	device->zstencilStateChanged = true;
}

void device_stencil_function(gs_device_t *device, enum gs_stencil_side side,
		enum gs_depth_test test)
{
	int sideVal = (int)side;

	if (sideVal & GS_STENCIL_FRONT)
		update_stencilside_test(device,
				device->zstencilState.stencilFront, test);
	if (sideVal & GS_STENCIL_BACK)
		update_stencilside_test(device,
				device->zstencilState.stencilBack, test);
}

static inline void update_stencilside_op(gs_device_t *device, StencilSide &side,
		enum gs_stencil_op_type fail, enum gs_stencil_op_type zfail,
		enum gs_stencil_op_type zpass)
{
	if (side.fail == fail && side.zfail == zfail && side.zpass == zpass)
		return;

	side.fail  = fail;
	side.zfail = zfail;
	side.zpass = zpass;
	device->zstencilStateChanged = true;
}

void device_stencil_op(gs_device_t *device, enum gs_stencil_side side,
		enum gs_stencil_op_type fail, enum gs_stencil_op_type zfail,
		enum gs_stencil_op_type zpass)
{
	int sideVal = (int)side;

	if (sideVal & GS_STENCIL_FRONT)
		update_stencilside_op(device,
				device->zstencilState.stencilFront,
				fail, zfail, zpass);
	if (sideVal & GS_STENCIL_BACK)
		update_stencilside_op(device,
				device->zstencilState.stencilBack,
				fail, zfail, zpass);
}

void device_set_viewport(gs_device_t *device, int x, int y, int width,
		int height)
{
	D3D11_VIEWPORT vp;
	memset(&vp, 0, sizeof(vp));
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = (float)x;
	vp.TopLeftY = (float)y;
	vp.Width    = (float)width;
	vp.Height   = (float)height;
	device->context->RSSetViewports(1, &vp);

	device->viewport.x  = x;
	device->viewport.y  = y;
	device->viewport.cx = width;
	device->viewport.cy = height;
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect)
{
	memcpy(rect, &device->viewport, sizeof(gs_rect));
}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	D3D11_RECT d3drect;

	device->rasterState.scissorEnabled = (rect != NULL);

	if (rect != NULL) {
		d3drect.left   = rect->x;
		d3drect.top    = rect->y;
		d3drect.right  = rect->x + rect->cx;
		d3drect.bottom = rect->y + rect->cy;
		device->context->RSSetScissorRects(1, &d3drect);
	}

	device->rasterStateChanged = true;
}

void device_ortho(gs_device_t *device, float left, float right, float top,
		float bottom, float zNear, float zFar)
{
	matrix4 *dst = &device->curProjMatrix;

	float rml = right-left;
	float bmt = bottom-top;
	float fmn = zFar-zNear;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x =         2.0f /  rml;
	dst->t.x = (left+right) / -rml;

	dst->y.y =         2.0f / -bmt;
	dst->t.y = (bottom+top) /  bmt;

	dst->z.z =         1.0f /  fmn;
	dst->t.z =        zNear / -fmn;

	dst->t.w = 1.0f;
}

void device_frustum(gs_device_t *device, float left, float right, float top,
		float bottom, float zNear, float zFar)
{
	matrix4 *dst = &device->curProjMatrix;

	float rml    = right-left;
	float bmt    = bottom-top;
	float fmn    = zFar-zNear;
	float nearx2 = 2.0f*zNear;

	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);

	dst->x.x =       nearx2 /  rml;
	dst->z.x = (left+right) / -rml;

	dst->y.y =       nearx2 / -bmt;
	dst->z.y = (bottom+top) /  bmt;

	dst->z.z =         zFar /  fmn;
	dst->t.z = (zNear*zFar) / -fmn;

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
	if (!device->projStack.size())
		return;

	mat4float *mat = device->projStack.data();
	size_t end = device->projStack.size()-1;

	/* XXX - does anyone know a better way of doing this? */
	memcpy(&device->curProjMatrix, mat+end, sizeof(matrix4));
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
	if (tex->type != GS_TEXTURE_2D)
		return 0;

	return static_cast<const gs_texture_2d*>(tex)->width;
}

uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return 0;

	return static_cast<const gs_texture_2d*>(tex)->height;
}

enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return GS_UNKNOWN;

	return static_cast<const gs_texture_2d*>(tex)->format;
}

bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr, uint32_t *linesize)
{
	HRESULT hr;

	if (tex->type != GS_TEXTURE_2D)
		return false;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);

	D3D11_MAPPED_SUBRESOURCE map;
	hr = tex2d->device->context->Map(tex2d->texture, 0,
			D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (FAILED(hr))
		return false;

	*ptr = (uint8_t*)map.pData;
	*linesize = map.RowPitch;
	return true;
}

void gs_texture_unmap(gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	tex2d->device->context->Unmap(tex2d->texture, 0);
}

void *gs_texture_get_obj(gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return nullptr;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	return tex2d->texture.Get();
}


void gs_cubetexture_destroy(gs_texture_t *cubetex)
{
	delete cubetex;
}

uint32_t gs_cubetexture_get_size(const gs_texture_t *cubetex)
{
	if (cubetex->type != GS_TEXTURE_CUBE)
		return 0;

	const gs_texture_2d *tex = static_cast<const gs_texture_2d*>(cubetex);
	return tex->width;
}

enum gs_color_format gs_cubetexture_get_color_format(
		const gs_texture_t *cubetex)
{
	if (cubetex->type != GS_TEXTURE_CUBE)
		return GS_UNKNOWN;

	const gs_texture_2d *tex = static_cast<const gs_texture_2d*>(cubetex);
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

enum gs_color_format gs_stagesurface_get_color_format(
		const gs_stagesurf_t *stagesurf)
{
	return stagesurf->format;
}

bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data,
		uint32_t *linesize)
{
	D3D11_MAPPED_SUBRESOURCE map;
	if (FAILED(stagesurf->device->context->Map(stagesurf->texture, 0,
			D3D11_MAP_READ, 0, &map)))
		return false;

	*data = (uint8_t*)map.pData;
	*linesize = map.RowPitch;
	return true;
}

void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf)
{
	stagesurf->device->context->Unmap(stagesurf->texture, 0);
}


void gs_zstencil_destroy(gs_zstencil_t *zstencil)
{
	delete zstencil;
}


void gs_samplerstate_destroy(gs_samplerstate_t *samplerstate)
{
	if (!samplerstate)
		return;

	if (samplerstate->device)
		for (int i = 0; i < GS_MAX_TEXTURES; i++)
			if (samplerstate->device->curSamplers[i] ==
					samplerstate)
				samplerstate->device->curSamplers[i] = nullptr;

	delete samplerstate;
}


void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer)
{
	if (vertbuffer && vertbuffer->device->lastVertexBuffer == vertbuffer)
		vertbuffer->device->lastVertexBuffer = nullptr;
	delete vertbuffer;
}

void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer)
{
	if (!vertbuffer->dynamic) {
		blog(LOG_ERROR, "gs_vertexbuffer_flush: vertex buffer is "
		                "not dynamic");
		return;
	}

	vertbuffer->FlushBuffer(vertbuffer->vertexBuffer,
			vertbuffer->vbd.data->points, sizeof(vec3));

	if (vertbuffer->normalBuffer)
		vertbuffer->FlushBuffer(vertbuffer->normalBuffer,
				vertbuffer->vbd.data->normals, sizeof(vec3));

	if (vertbuffer->tangentBuffer)
		vertbuffer->FlushBuffer(vertbuffer->tangentBuffer,
				vertbuffer->vbd.data->tangents, sizeof(vec3));

	if (vertbuffer->colorBuffer)
		vertbuffer->FlushBuffer(vertbuffer->colorBuffer,
				vertbuffer->vbd.data->colors, sizeof(uint32_t));

	for (size_t i = 0; i < vertbuffer->uvBuffers.size(); i++) {
		gs_tvertarray &tv = vertbuffer->vbd.data->tvarray[i];
		vertbuffer->FlushBuffer(vertbuffer->uvBuffers[i],
				tv.array, tv.width*sizeof(float));
	}
}

struct gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer)
{
	return vertbuffer->vbd.data;
}


void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer)
{
	delete indexbuffer;
}

void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer)
{
	HRESULT hr;

	if (!indexbuffer->dynamic)
		return;

	D3D11_MAPPED_SUBRESOURCE map;
	hr = indexbuffer->device->context->Map(indexbuffer->indexBuffer, 0,
			D3D11_MAP_WRITE_DISCARD, 0, &map);
	if (FAILED(hr))
		return;

	memcpy(map.pData, indexbuffer->indices.data,
			indexbuffer->num * indexbuffer->indexSize);

	indexbuffer->device->context->Unmap(indexbuffer->indexBuffer, 0);
}

void *gs_indexbuffer_get_data(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->indices.data;
}

size_t gs_indexbuffer_get_num_indices(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->num;
}

enum gs_index_type gs_indexbuffer_get_type(const gs_indexbuffer_t *indexbuffer)
{
	return indexbuffer->type;
}

extern "C" EXPORT bool device_gdi_texture_available(void)
{
	return true;
}

extern "C" EXPORT bool device_shared_texture_available(void)
{
	return true;
}

extern "C" EXPORT gs_texture_t *device_texture_create_gdi(gs_device_t *device,
		uint32_t width, uint32_t height)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, width, height, GS_BGRA,
				1, nullptr, GS_RENDER_TARGET, GS_TEXTURE_2D,
				true, false);
	} catch (HRError error) {
		blog(LOG_ERROR, "device_texture_create_gdi (D3D11): %s (%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_texture_create_gdi (D3D11): %s", error);
	}

	return texture;
}

static inline bool TextureGDICompatible(gs_texture_2d *tex2d, const char *func)
{
	if (!tex2d->isGDICompatible) {
		blog(LOG_ERROR, "%s (D3D11): Texture is not GDI compatible",
				func);
		return false;
	}

	return true;
}

extern "C" EXPORT void *gs_texture_get_dc(gs_texture_t *tex)
{
	HDC hDC = nullptr;

	if (tex->type != GS_TEXTURE_2D)
		return nullptr;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	if (!TextureGDICompatible(tex2d, "gs_texture_get_dc"))
		return nullptr;

	if (!tex2d->gdiSurface)
		return nullptr;

	tex2d->gdiSurface->GetDC(true, &hDC);
	return hDC;
}

extern "C" EXPORT void gs_texture_release_dc(gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d*>(tex);
	if (!TextureGDICompatible(tex2d, "gs_texture_release_dc"))
		return;

	tex2d->gdiSurface->ReleaseDC(nullptr);
}

extern "C" EXPORT gs_texture_t *device_texture_open_shared(gs_device_t *device,
		uint32_t handle)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, handle);
	} catch (HRError error) {
		blog(LOG_ERROR, "gs_texture_open_shared (D3D11): %s (%08lX)",
				error.str, error.hr);
		LogD3D11ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "gs_texture_open_shared (D3D11): %s", error);
	}

	return texture;
}
