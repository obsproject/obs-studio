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
#include <algorithm>

#include <util/base.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/util.hpp>
#include <graphics/matrix3.h>
#include <winternl.h>
#include "d3d12-subsystem.hpp"
#include <shellscalingapi.h>
#include <d3dkmthk.h>

struct UnsupportedHWError : HRError {
	inline UnsupportedHWError(const char *str, HRESULT hr) : HRError(str, hr) {}
};

static inline void LogD3D12ErrorDetails(HRError error, gs_device_t *device)
{
	if (error.hr == DXGI_ERROR_DEVICE_REMOVED) {
		HRESULT DeviceRemovedReason = device->d3d12Instance->GetDevice()->GetDeviceRemovedReason();
		blog(LOG_ERROR, "  Device Removed Reason: %08lX", DeviceRemovedReason);
	}
}

gs_obj::gs_obj(gs_device_t *device_, gs_type type) : device(device_), obj_type(type)
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

static enum gs_color_space get_next_space(gs_device_t *device, HWND hwnd, DXGI_SWAP_EFFECT effect)
{
	enum gs_color_space next_space = GS_CS_SRGB;
	if (effect == DXGI_SWAP_EFFECT_FLIP_DISCARD) {
		const HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
		if (hMonitor) {
			const auto info = D3D12Graphics::D3D12DeviceInstance::GetMonitorColorInfo(hMonitor);
			if (info.m_HDR)
				next_space = GS_CS_709_SCRGB;
			else if (info.m_BitsPerColor > 8)
				next_space = GS_CS_SRGB_16F;
		}
	}

	return next_space;
}

static enum gs_color_format get_swap_format_from_space(gs_color_space space, gs_color_format sdr_format)
{
	gs_color_format format = sdr_format;
	switch (space) {
	case GS_CS_SRGB_16F:
	case GS_CS_709_SCRGB:
		format = GS_RGBA16F;
	}

	return format;
}

static inline enum gs_color_space make_swap_desc(gs_device *device, DXGI_SWAP_CHAIN_DESC1 &desc,
						 const gs_init_data *data,
						 DXGI_SWAP_EFFECT effect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
						 UINT flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH)
{
	const HWND hwnd = (HWND)data->window.hwnd;
	const enum gs_color_space space = get_next_space(device, hwnd, effect);
	const gs_color_format format = get_swap_format_from_space(space, data->format);

	memset(&desc, 0, sizeof(desc));

	desc.Width = data->cx;
	desc.Height = data->cy;
	desc.Format = ConvertGSTextureFormatView(format);
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	desc.BufferCount = data->num_backbuffers;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Scaling = DXGI_SCALING_NONE;
	desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	return space;
}

void gs_swap_chain::Release()
{
	device->context->Flush(true);
}

void gs_swap_chain::Resize(uint32_t cx, uint32_t cy, gs_color_format format)
{
	RECT clientRect;
	HRESULT hr = 0;

	initData.cx = cx;
	initData.cy = cy;

	if (cx == 0 || cy == 0) {
		GetClientRect(hwnd, &clientRect);
		if (cx == 0)
			cx = clientRect.right;
		if (cy == 0)
			cy = clientRect.bottom;
	}
	const DXGI_FORMAT dxgi_format = ConvertGSTextureFormatView(format);

	for (uint32_t i = 0; i < initData.num_backbuffers; ++i) {
		target[i].Release();
		target[i].Destroy();
	}
	hr = swap->ResizeBuffers(initData.num_backbuffers, cx, cy, dxgi_format, swapDesc.Flags);
	if (FAILED(hr)) {
		HRESULT reason = device->d3d12Instance->GetDevice()->GetDeviceRemovedReason();
		__debugbreak();
		throw HRError("Failed to resize swap buffers", hr);
	}

	if (bEnableHDROutput) {
		hr = swap->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020);
		if (FAILED(hr)) {
			__debugbreak();
			throw HRError("Failed to set color space", hr);
		}
	}

	for (uint32_t i = 0; i < initData.num_backbuffers; ++i) {
		target[i].CreateTargetFromSwapChain(device, swap, i, format, initData);
	}

	if (initData.zsformat != GS_ZS_NONE) {
		zs = std::make_unique<gs_zstencil_buffer>(device, initData.cx, initData.cy, initData.zsformat);
	}
	currentBackBufferIndex = swap->GetCurrentBackBufferIndex();
}

gs_swap_chain::gs_swap_chain(gs_device *device, const gs_init_data *data)
	: gs_obj(device, gs_type::gs_swap_chain),
	  hwnd((HWND)data->window.hwnd),
	  initData(*data),
	  space(GS_CS_SRGB)
{
	initData.num_backbuffers = initData.num_backbuffers > 3 ? initData.num_backbuffers : 3;
	space = make_swap_desc(device, swapDesc, &initData);

	ComPtr<IDXGISwapChain1> swap1;
	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	HRESULT hr = device->d3d12Instance->GetDxgiFactory()->CreateSwapChainForHwnd(
		device->d3d12Instance->GetCommandManager().GetCommandQueue(), hwnd, &swapDesc, &fsSwapChainDesc,
		nullptr, &swap1);
	if (FAILED(hr)) {
		HRESULT reason = device->d3d12Instance->GetDevice()->GetDeviceRemovedReason();
		throw HRError("Failed to create swap chain", hr);
	}

	swap = ComQIPtr<IDXGISwapChain4>(swap1);
	if (!swap)
		throw HRError("Failed to create swap chain3", hr);

	/* Ignore Alt+Enter */
	device->d3d12Instance->GetDxgiFactory()->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER);

	const gs_color_format format =
		get_swap_format_from_space(get_next_space(device, hwnd, swapDesc.SwapEffect), initData.format);

	for (uint32_t i = 0; i < initData.num_backbuffers; ++i) {
		target[i].CreateTargetFromSwapChain(device, swap, i, format, initData);
	}

	if (initData.zsformat != GS_ZS_NONE) {
		zs = std::make_unique<gs_zstencil_buffer>(device, initData.cx, initData.cy, initData.zsformat);
	}

	currentBackBufferIndex = swap->GetCurrentBackBufferIndex();

	// HDR Enable
	bool enableHDROutput = CheckHDRSupport();
	if (enableHDROutput && space == GS_CS_709_SCRGB &&
	    SUCCEEDED(swap->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020))) {
		bEnableHDROutput = true;
	}
}

bool gs_swap_chain::CheckHDRSupport()
{
	ComPtr<IDXGIOutput> output;
	ComPtr<IDXGIOutput6> output6;
	DXGI_OUTPUT_DESC1 outputDesc;
	HRESULT hr = swap->GetContainingOutput(&output);
	if (FAILED(hr)) {
		return false;
	}

	output6 = ComQIPtr<IDXGIOutput6>(output);
	if (!output6) {
		return false;
	}

	hr = output6->GetDesc1(&outputDesc);
	if (FAILED(hr)) {
		return false;
	}

	if (outputDesc.ColorSpace = DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
		return true;
	}

	/* UINT colorSpaceSupport = 0;
	swap->CheckColorSpaceSupport(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020, &colorSpaceSupport);

	if (colorSpaceSupport & DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) {
		return true;
	}*/

	return false;
}

gs_swap_chain::~gs_swap_chain()
{
	Release();
}

static bool FastClearSupported(UINT vendorId, uint64_t version)
{
	/* Always true for non-NVIDIA GPUs */
	if (vendorId != 0x10de)
		return true;

	const uint16_t aa = (version >> 48) & 0xffff;
	const uint16_t bb = (version >> 32) & 0xffff;
	const uint16_t ccccc = (version >> 16) & 0xffff;
	const uint16_t ddddd = version & 0xffff;

	/* Check for NVIDIA driver version >= 31.0.15.2737 */
	return aa >= 31 && bb >= 0 && ccccc >= 15 && ddddd >= 2737;
}

void gs_device::InitDevice(uint32_t adapterIdx)
{
	d3d12Instance = new D3D12Graphics::D3D12DeviceInstance();
	d3d12Instance->Initialize(adapterIdx);
	auto device11 = d3d12Instance->GetDevice();

	fastClearSupported = d3d12Instance->FastClearSupported();
	nv12Supported = d3d12Instance->IsNV12TextureSupported();
	p010Supported = d3d12Instance->IsP010TextureSupported();

	blog(LOG_INFO, "D3D12 loaded successfully");
}

static inline void ConvertStencilSide(D3D12_DEPTH_STENCILOP_DESC &desc, const StencilSide &side)
{
	desc.StencilFunc = ConvertGSDepthTest(side.test);
	desc.StencilFailOp = ConvertGSStencilOp(side.fail);
	desc.StencilDepthFailOp = ConvertGSStencilOp(side.zfail);
	desc.StencilPassOp = ConvertGSStencilOp(side.zpass);
}

D3D12_DEPTH_STENCIL_DESC gs_device::ConvertZStencilState(const ZStencilState &zs)
{
	D3D12_DEPTH_STENCIL_DESC desc;
	memset(&desc, 0, sizeof(desc));

	desc.DepthEnable = zs.depthEnabled;
	desc.DepthFunc = ConvertGSDepthTest(zs.depthFunc);
	desc.DepthWriteMask = zs.depthWriteEnabled ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
	desc.StencilEnable = zs.stencilEnabled;
	desc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	desc.StencilWriteMask = zs.stencilWriteEnabled ? D3D12_DEFAULT_STENCIL_WRITE_MASK : 0;
	ConvertStencilSide(desc.FrontFace, zs.stencilFront);
	ConvertStencilSide(desc.BackFace, zs.stencilBack);
	return desc;
}

D3D12_RASTERIZER_DESC gs_device::ConvertRasterState(const RasterState &rs)
{
	D3D12_RASTERIZER_DESC desc;
	memset(&desc, 0, sizeof(desc));

	/* use CCW to convert to a right-handed coordinate system */
	desc.FrontCounterClockwise = true;
	desc.FillMode = D3D12_FILL_MODE_SOLID;
	desc.CullMode = ConvertGSCullMode(rs.cullMode);
	return desc;
}

D3D12_BLEND_DESC gs_device::ConvertBlendState(const BlendState &bs)
{
	D3D12_BLEND_DESC desc;
	memset(&desc, 0, sizeof(desc));

	for (int i = 0; i < 8; i++) {
		desc.RenderTarget[i].LogicOpEnable = FALSE;
		desc.RenderTarget[i].LogicOp = D3D12_LOGIC_OP_NOOP;
		desc.RenderTarget[i].BlendEnable = bs.blendEnabled;
		desc.RenderTarget[i].BlendOp = ConvertGSBlendOpType(bs.op);
		desc.RenderTarget[i].BlendOpAlpha = ConvertGSBlendOpType(bs.op);
		desc.RenderTarget[i].SrcBlend = ConvertGSBlendType(bs.srcFactorC);
		desc.RenderTarget[i].DestBlend = ConvertGSBlendType(bs.destFactorC);
		desc.RenderTarget[i].SrcBlendAlpha = ConvertGSBlendType(bs.srcFactorA);
		desc.RenderTarget[i].DestBlendAlpha = ConvertGSBlendType(bs.destFactorA);
		desc.RenderTarget[i].RenderTargetWriteMask = (bs.redEnabled ? D3D12_COLOR_WRITE_ENABLE_RED : 0) |
							     (bs.greenEnabled ? D3D12_COLOR_WRITE_ENABLE_GREEN : 0) |
							     (bs.blueEnabled ? D3D12_COLOR_WRITE_ENABLE_BLUE : 0) |
							     (bs.alphaEnabled ? D3D12_COLOR_WRITE_ENABLE_ALPHA : 0);
	}

	return desc;
}

void gs_device::LoadVertexBufferData()
{
	if (curVertexBuffer == lastVertexBuffer && curVertexShader == lastVertexShader)
		return;

	D3D12Graphics::GpuBuffer *buffers[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT];

	memset(buffers, 0, sizeof(buffers));

	uint32_t strides[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {0};
	uint32_t offsets[D3D12_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT] = {0};
	UINT numBuffers = 0;

	if (curVertexBuffer && curVertexShader) {
		numBuffers = curVertexBuffer->MakeBufferList(curVertexShader, buffers, strides);
	} else {
		numBuffers = curVertexShader ? curVertexShader->NumBuffersExpected() : 0;
	}

	curVertexBufferViews.resize(numBuffers);

	for (uint32_t i = 0; i < numBuffers && curVertexBuffer; ++i) {
		curVertexBufferViews[i] = buffers[i]->VertexBufferView();
	}

	lastVertexBuffer = curVertexBuffer;
	lastVertexShader = curVertexShader;
}

void gs_device::LoadRootSignature(std::unique_ptr<D3D12Graphics::RootSignature> &rootSignature)
{
	int32_t numParameters = 0;
	if (curVertexShader->hasDynamicUniformConstantBuffer) {
		numParameters += 1;
	}

	if (curPixelShader->samplerCount > 0) {
		numParameters += 1;
	}

	if (curPixelShader->textureCount > 0) {
		numParameters += 1;
	}

	if (curPixelShader->hasDynamicUniformConstantBuffer) {
		numParameters += 1;
	}

	rootSignature->Reset(numParameters, 0);
	int32_t parameterIndex = 0;

	D3D12Graphics::RootSignature &curRootSignatureTemp = *rootSignature;
	if (curVertexShader->hasDynamicUniformConstantBuffer) {
		curRootSignatureTemp[parameterIndex].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_VERTEX);
		curVertexShader->dynamicUniformConstantBufferRootParameterIndex = parameterIndex;
		parameterIndex += 1;
	}

	if (curPixelShader->samplerCount > 0) {
		curRootSignatureTemp[parameterIndex].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 0,
									   (UINT)(curPixelShader->samplerCount),
									   D3D12_SHADER_VISIBILITY_PIXEL);
		curPixelShader->samplerRootParameterIndex = parameterIndex;
		parameterIndex += 1;
	}

	if (curPixelShader->textureCount > 0) {
		curRootSignatureTemp[parameterIndex].InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0,
									   (UINT)(curPixelShader->textureCount),
									   D3D12_SHADER_VISIBILITY_PIXEL);
		curPixelShader->textureRootParameterIndex = parameterIndex;
		parameterIndex += 1;
	}

	if (curPixelShader->hasDynamicUniformConstantBuffer) {
		curRootSignatureTemp[parameterIndex].InitAsConstantBuffer(0, D3D12_SHADER_VISIBILITY_PIXEL);
		curPixelShader->dynamicUniformConstantBufferRootParameterIndex = parameterIndex;
		parameterIndex += 1;
	}

	rootSignature->Finalize(L"", D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
					     D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
					     D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
					     D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS);
}

void gs_device::LoadCurrentGraphicsPSO(std::unique_ptr<D3D12Graphics::GraphicsPSO> &PipelineState,
				       std::unique_ptr<D3D12Graphics::RootSignature> &rootSignature)
{
	DXGI_FORMAT zsForamt = curZStencilBuffer ? curZStencilBuffer->GetDSVFormat() : DXGI_FORMAT_UNKNOWN;

	PipelineState->SetRootSignature(*rootSignature);
	PipelineState->SetRasterizerState(ConvertRasterState(curRasterState));
	PipelineState->SetBlendState(ConvertBlendState(curBlendState));
	PipelineState->SetDepthStencilState(ConvertZStencilState(curZstencilState));
	PipelineState->SetSampleMask(0xFFFFFFFF);
	PipelineState->SetInputLayout((UINT)(curVertexShader->layoutData.size()), curVertexShader->layoutData.data());
	PipelineState->SetPrimitiveTopologyType(ConvertD3D12Topology(curToplogy));

	PipelineState->SetVertexShader(curVertexShader->data.data(), curVertexShader->data.size());
	PipelineState->SetPixelShader(curPixelShader->data.data(), curPixelShader->data.size());

	DXGI_FORMAT rtvFormat = curFramebufferSrgb ? curRenderTarget->dxgiFormatViewLinear
						   : curRenderTarget->dxgiFormatView;
	PipelineState->SetRenderTargetFormats(1, &rtvFormat, zsForamt);
	PipelineState->Finalize();
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
		gs_shader_set_matrix4(curVertexShader->viewProj, &curViewProjMatrix);
}
void gs_device::FlushOutputViews()
{
	if (curFramebufferInvalidate) {
		D3D12Graphics::GpuResource *rtv = nullptr;
		D3D12_CPU_DESCRIPTOR_HANDLE pRenderTargetDescriptors;
		if (curRenderTarget) {
			const int i = curRenderSide;
			rtv = curRenderTarget;

			pRenderTargetDescriptors = curFramebufferSrgb ? curRenderTarget->renderTargetLinearRTV[i]
								      : curRenderTarget->renderTargetRTV[i];
			if (!rtv) {
				blog(LOG_ERROR, "device_draw (D3D12): texture is not a render target");
				return;
			}
		}
		const D3D12_CPU_DESCRIPTOR_HANDLE *dsv = nullptr;
		if (curZStencilBuffer) {
			dsv = &curZStencilBuffer->GetDSV();
		}

		context->TransitionResource(*rtv, D3D12_RESOURCE_STATE_RENDER_TARGET);
		context->SetRenderTargets(1, &pRenderTargetDescriptors, false, dsv);

		curFramebufferInvalidate = false;
	}
}

gs_monitor_color_info gs_device::GetMonitorColorInfo(HMONITOR hMonitor)
{
	D3D12Graphics::MonitorColorInfo info = D3D12Graphics::D3D12DeviceInstance::GetMonitorColorInfo(hMonitor);
	return gs_monitor_color_info(info.m_HDR, info.m_BitsPerColor, info.m_SDRWhiteNits);
}

gs_device::gs_device(uint32_t adapterIdx)
{
	matrix4_identity(&curProjMatrix);
	matrix4_identity(&curViewMatrix);
	matrix4_identity(&curViewProjMatrix);

	memset(&viewport, 0, sizeof(viewport));

	for (size_t i = 0; i < GS_MAX_TEXTURES; i++) {
		curTextures[i] = NULL;
		curSamplers[i] = NULL;
	}
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

bool device_enum_adapters(gs_device_t *device, bool (*callback)(void *param, const char *name, uint32_t id),
			  void *param)
{
	UNUSED_PARAMETER(device);

	try {
		D3D12Graphics::D3D12DeviceInstance::EnumD3DAdapters(callback, param);
		return true;

	} catch (const HRError &error) {
		blog(LOG_WARNING, "Failed enumerating devices: %s (%08lX)", error.str, error.hr);
		return false;
	}
}

int device_create(gs_device_t **p_device, uint32_t adapter)
{
	gs_device *device = NULL;
	int errorcode = GS_SUCCESS;

	try {
		blog(LOG_INFO, "---------------------------------");
		blog(LOG_INFO, "Initializing D3D12...");
		D3D12Graphics::D3D12DeviceInstance::LogD3DAdapters();
		device = new gs_device(adapter);

	} catch (const UnsupportedHWError &error) {
		blog(LOG_ERROR, "device_create (D3D12): %s (%08lX)", error.str, error.hr);
		errorcode = GS_ERROR_NOT_SUPPORTED;

	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_create (D3D12): %s (%08lX)", error.str, error.hr);
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
	return (void *)device->d3d12Instance->GetDevice();
}

gs_swapchain_t *device_swapchain_create(gs_device_t *device, const struct gs_init_data *data)
{
	gs_swap_chain *swap = NULL;

	try {
		swap = new gs_swap_chain(device, data);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_swapchain_create (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return swap;
}

static void device_resize_internal(gs_device_t *device, uint32_t cx, uint32_t cy, gs_color_space space)
{
	try {
		const gs_color_format format = get_swap_format_from_space(space, device->curSwapChain->initData.format);
		device->context->Flush(true);
		device->context->SetNullRenderTarget();
		device->curSwapChain->Resize(cx, cy, format);
		device->curRenderTarget = &device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex];
		device->curSwapChain->space = space;
		device->curFramebufferInvalidate = true;
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_resize_internal (D3D11): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}
}

void device_resize(gs_device_t *device, uint32_t cx, uint32_t cy)
{
	if (!device->curSwapChain) {
		blog(LOG_WARNING, "device_resize (D3D12): No active swap");
		return;
	}

	const enum gs_color_space next_space =
		get_next_space(device, device->curSwapChain->hwnd, device->curSwapChain->swapDesc.SwapEffect);
	device_resize_internal(device, cx, cy, next_space);
}

enum gs_color_space device_get_color_space(gs_device_t *device)
{
	return device->curColorSpace;
}

void device_update_color_space(gs_device_t *device)
{
	if (device->curSwapChain) {
		const enum gs_color_space next_space =
			get_next_space(device, device->curSwapChain->hwnd, device->curSwapChain->swapDesc.SwapEffect);
		if (device->curSwapChain->space != next_space)
			device_resize_internal(device, 0, 0, next_space);
	} else {
		blog(LOG_WARNING, "device_update_color_space (D3D12): No active swap");
	}
}

void device_get_size(const gs_device_t *device, uint32_t *cx, uint32_t *cy)
{
	if (device->curSwapChain) {
		*cx = device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex].width;
		*cy = device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex].height;
	} else {
		blog(LOG_ERROR, "device_get_size (D3D12): no active swap");
		*cx = 0;
		*cy = 0;
	}
}

uint32_t device_get_width(const gs_device_t *device)
{
	if (device->curSwapChain) {
		return device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex].width;
	} else {
		blog(LOG_ERROR, "device_get_size (D3D12): no active swap");
		return 0;
	}
}

uint32_t device_get_height(const gs_device_t *device)
{
	if (device->curSwapChain) {
		return device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex].height;
	} else {
		blog(LOG_ERROR, "device_get_size (D3D12): no active swap");
		return 0;
	}
}

gs_texture_t *device_texture_create(gs_device_t *device, uint32_t width, uint32_t height,
				    enum gs_color_format color_format, uint32_t levels, const uint8_t **data,
				    uint32_t flags)
{
	gs_texture *texture = NULL;
	try {
		texture = new gs_texture_2d(device, width, height, color_format, levels, data, flags, GS_TEXTURE_2D,
					    false);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_texture_create (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_texture_create (D3D12): %s", error);
	}

	return texture;
}

gs_texture_t *device_cubetexture_create(gs_device_t *device, uint32_t size, enum gs_color_format color_format,
					uint32_t levels, const uint8_t **data, uint32_t flags)
{
	gs_texture *texture = NULL;
	try {
		texture = new gs_texture_2d(device, size, size, color_format, levels, data, flags, GS_TEXTURE_CUBE,
					    false);
	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_cubetexture_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_cubetexture_create (D3D12): %s", error);
	}

	return texture;
}

gs_texture_t *device_voltexture_create(gs_device_t *device, uint32_t width, uint32_t height, uint32_t depth,
				       enum gs_color_format color_format, uint32_t levels, const uint8_t *const *data,
				       uint32_t flags)
{
	gs_texture *texture = NULL;
	try {
		texture = new gs_texture_3d(device, width, height, depth, color_format, levels, data, flags);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_voltexture_create (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_voltexture_create (D3D12): %s", error);
	}

	return texture;
}

gs_zstencil_t *device_zstencil_create(gs_device_t *device, uint32_t width, uint32_t height,
				      enum gs_zstencil_format format)
{
	gs_zstencil_buffer *zstencil = NULL;
	try {
		zstencil = new gs_zstencil_buffer(device, width, height, format);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_zstencil_create (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return zstencil;
}
gs_stagesurf_t *device_stagesurface_create(gs_device_t *device, uint32_t width, uint32_t height,
					   enum gs_color_format color_format)
{
	gs_stage_surface *surf = NULL;
	try {
		surf = new gs_stage_surface(device, width, height, color_format);
	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_stagesurface_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return surf;
}

gs_samplerstate_t *device_samplerstate_create(gs_device_t *device, const struct gs_sampler_info *info)
{
	gs_sampler_state *ss = NULL;
	try {
		ss = new gs_sampler_state(device, info);
	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_samplerstate_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return ss;
}

gs_shader_t *device_vertexshader_create(gs_device_t *device, const char *shader_string, const char *file,
					char **error_string)
{
	gs_vertex_shader *shader = NULL;
	try {
		shader = new gs_vertex_shader(device, file, shader_string);

	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_vertexshader_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);

	} catch (const ShaderError &error) {
		const char *buf = (const char *)error.errors->GetBufferPointer();
		if (error_string)
			*error_string = bstrdup(buf);
		blog(LOG_ERROR,
		     "device_vertexshader_create (D3D12): "
		     "Compile warnings/errors for %s:\n%s",
		     file, buf);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexshader_create (D3D12): %s", error);
	}

	return shader;
}

gs_shader_t *device_pixelshader_create(gs_device_t *device, const char *shader_string, const char *file,
				       char **error_string)
{
	gs_pixel_shader *shader = NULL;
	try {
		shader = new gs_pixel_shader(device, file, shader_string);

	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_pixelshader_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);

	} catch (const ShaderError &error) {
		const char *buf = (const char *)error.errors->GetBufferPointer();
		if (error_string)
			*error_string = bstrdup(buf);
		blog(LOG_ERROR,
		     "device_pixelshader_create (D3D12): "
		     "Compiler warnings/errors for %s:\n%s",
		     file, buf);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_pixelshader_create (D3D12): %s", error);
	}

	return shader;
}

gs_vertbuffer_t *device_vertexbuffer_create(gs_device_t *device, struct gs_vb_data *data, uint32_t flags)
{
	gs_vertex_buffer *buffer = NULL;
	try {
		buffer = new gs_vertex_buffer(device, data, flags);
	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_vertexbuffer_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_vertexbuffer_create (D3D12): %s", error);
	}

	return buffer;
}

gs_indexbuffer_t *device_indexbuffer_create(gs_device_t *device, enum gs_index_type type, void *indices, size_t num,
					    uint32_t flags)
{
	gs_index_buffer *buffer = NULL;
	try {
		buffer = new gs_index_buffer(device, type, indices, num, flags);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_indexbuffer_create (D3D11): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return buffer;
}

gs_timer_t *device_timer_create(gs_device_t *device)
{
	gs_timer *timer = NULL;
	try {
		timer = new gs_timer(device);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_timer_create (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return timer;
}

gs_timer_range_t *device_timer_range_create(gs_device_t *device)
{
	gs_timer_range *range = NULL;
	try {
		range = new gs_timer_range(device);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_timer_range_create (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return range;
}

enum gs_texture_type device_get_texture_type(const gs_texture_t *texture)
{
	return texture->type;
}

void device_load_vertexbuffer(gs_device_t *device, gs_vertbuffer_t *vertbuffer)
{
	if (device->curVertexBuffer == vertbuffer)
		return;

	device->curVertexBuffer = vertbuffer;
}

void device_load_indexbuffer(gs_device_t *device, gs_indexbuffer_t *indexbuffer)
{
	if (device->curIndexBuffer == indexbuffer)
		return;

	device->curIndexBuffer = indexbuffer;
}

static void device_load_texture_internal(gs_device_t *device, gs_texture_t *tex, int unit,
					 const D3D12_CPU_DESCRIPTOR_HANDLE *handle)
{
	if (device->curTextures[unit] == tex)
		return;

	device->curTextures[unit] = tex;
	device->context->SetDynamicDescriptor(device->curPixelShader->textureRootParameterIndex, unit, *handle);
}

void device_load_texture(gs_device_t *device, gs_texture_t *tex, int unit)
{
	if (device->curTextures[unit] == tex)
		return;

	const D3D12_CPU_DESCRIPTOR_HANDLE *handle = nullptr;
	if (tex) {
		handle = &tex->shaderSRV;
		device->context->TransitionResource(*tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	} else {
		handle = nullptr;
	}
	device_load_texture_internal(device, tex, unit, handle);
}

void device_load_texture_srgb(gs_device_t *device, gs_texture_t *tex, int unit)
{
	if (device->curTextures[unit] == tex)
		return;
	const D3D12_CPU_DESCRIPTOR_HANDLE *handle = nullptr;
	if (tex) {
		handle = &tex->shaderLinearSRV;
		device->context->TransitionResource(*tex, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	} else {
		handle = nullptr;
	}
	device_load_texture_internal(device, tex, unit, handle);
}

void device_load_samplerstate(gs_device_t *device, gs_samplerstate_t *samplerstate, int unit)
{
	const D3D12_CPU_DESCRIPTOR_HANDLE *handle = nullptr;
	if (device->curSamplers[unit] == samplerstate)
		return;

	if (samplerstate) {
		handle = &samplerstate->sampler;
	}

	device->curSamplers[unit] = samplerstate;
	device->context->SetDynamicSampler(device->curPixelShader->samplerRootParameterIndex, unit, *handle);
}

void device_load_vertexshader(gs_device_t *device, gs_shader_t *vertshader)
{
	if (device->curVertexShader == vertshader)
		return;

	gs_vertex_shader *vs = static_cast<gs_vertex_shader *>(vertshader);

	if (vertshader) {
		if (vertshader->type != GS_SHADER_VERTEX) {
			blog(LOG_ERROR, "device_load_vertexshader (D3D12): "
					"Specified shader is not a vertex "
					"shader");
			return;
		}
	}

	device->curVertexShader = vs;
}

void device_load_pixelshader(gs_device_t *device, gs_shader_t *pixelshader)
{
	gs_samplerstate_t *states[GS_MAX_TEXTURES] = {0};

	if (device->curPixelShader == pixelshader)
		return;

	gs_pixel_shader *ps = static_cast<gs_pixel_shader *>(pixelshader);

	if (pixelshader) {
		if (pixelshader->type != GS_SHADER_PIXEL) {
			blog(LOG_ERROR, "device_load_pixelshader (D3D11): "
					"Specified shader is not a pixel "
					"shader");
			return;
		}

	} else {
		memset(states, 0, sizeof(states));
	}

	memset(device->curTextures, 0, sizeof(device->curTextures));

	device->curPixelShader = ps;
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
	return device->curRenderTarget;
}

gs_zstencil_t *device_get_zstencil_target(const gs_device_t *device)
{
	return device->curZStencilBuffer;
}

static void device_set_render_target_internal(gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil,
					      enum gs_color_space space)
{
	if (device->curSwapChain) {
		if (!tex)
			tex = &device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex];
		if (!zstencil)
			zstencil = device->curSwapChain->zs.get();
	}

	if (device->curRenderTarget == tex && device->curZStencilBuffer == zstencil) {
		device->curColorSpace = space;
	}

	if (tex && tex->type != GS_TEXTURE_2D) {
		blog(LOG_ERROR, "device_set_render_target_internal (D3D12): texture is not a 2D texture");
		return;
	}

	gs_texture_2d *const tex2d = static_cast<gs_texture_2d *>(tex);
	if (device->curRenderTarget != tex2d || device->curRenderSide != 0 || device->curZStencilBuffer != zstencil) {
		device->curRenderTarget = tex2d;
		device->curZStencilBuffer = zstencil;
		device->curRenderSide = 0;
		device->curColorSpace = space;
		device->curFramebufferInvalidate = true;
	}
}

void device_set_render_target(gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil)
{
	device_set_render_target_internal(device, tex, zstencil, GS_CS_SRGB);
}

void device_set_render_target_with_color_space(gs_device_t *device, gs_texture_t *tex, gs_zstencil_t *zstencil,
					       enum gs_color_space space)
{
	device_set_render_target_internal(device, tex, zstencil, space);
}

void device_set_cube_render_target(gs_device_t *device, gs_texture_t *tex, int side, gs_zstencil_t *zstencil)
{
	if (device->curSwapChain) {
		if (!tex) {
			tex = &device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex];
			side = 0;
		}

		if (!zstencil)
			zstencil = device->curSwapChain->zs.get();
	}

	if (device->curRenderTarget == tex && device->curRenderSide == side && device->curZStencilBuffer == zstencil)
		return;

	if (tex->type != GS_TEXTURE_CUBE) {
		blog(LOG_ERROR, "device_set_cube_render_target (D3D11): "
				"texture is not a cube texture");
		return;
	}

	gs_texture_2d *const tex2d = static_cast<gs_texture_2d *>(tex);
	if (device->curRenderTarget != tex2d || device->curRenderSide != side ||
	    device->curZStencilBuffer != zstencil) {
		device->curRenderTarget = tex2d;
		device->curZStencilBuffer = zstencil;
		device->curRenderSide = side;
		device->curColorSpace = GS_CS_SRGB;
		device->curFramebufferInvalidate = true;
	}
}

void device_enable_framebuffer_srgb(gs_device_t *device, bool enable)
{
	if (device->curFramebufferSrgb != enable) {
		device->curFramebufferSrgb = enable;
		device->curFramebufferInvalidate = true;
	}
}

bool device_framebuffer_srgb_enabled(gs_device_t *device)
{
	return device->curFramebufferSrgb;
}

static DXGI_FORMAT get_copy_compare_format(gs_color_format format)
{
	switch (format) {
	case GS_RGBA_UNORM:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;
	case GS_BGRX_UNORM:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;
	case GS_BGRA_UNORM:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;
	default:
		return ConvertGSTextureFormatResource(format);
	}
}

void device_copy_texture_region(gs_device_t *device, gs_texture_t *dst, uint32_t dst_x, uint32_t dst_y,
				gs_texture_t *src, uint32_t src_x, uint32_t src_y, uint32_t src_w, uint32_t src_h)
{
	try {
		gs_texture_2d *src2d = static_cast<gs_texture_2d *>(src);
		gs_texture_2d *dst2d = static_cast<gs_texture_2d *>(dst);

		if (!src)
			throw "Source texture is NULL";
		if (!dst)
			throw "Destination texture is NULL";
		if (src->type != GS_TEXTURE_2D || dst->type != GS_TEXTURE_2D)
			throw "Source and destination textures must be a 2D "
			      "textures";
		if (get_copy_compare_format(dst->format) != get_copy_compare_format(src->format))
			throw "Source and destination formats do not match";

		uint32_t copyWidth = (uint32_t)src_w ? (uint32_t)src_w : (src2d->width - src_x);
		uint32_t copyHeight = (uint32_t)src_h ? (uint32_t)src_h : (src2d->height - src_y);

		uint32_t dstWidth = dst2d->width - dst_x;
		uint32_t dstHeight = dst2d->height - dst_y;

		if (dstWidth < copyWidth || dstHeight < copyHeight)
			throw "Destination texture region is not big "
			      "enough to hold the source region";

		if (dst_x == 0 && dst_y == 0 && src_x == 0 && src_y == 0 && src_w == 0 && src_h == 0) {
			copyWidth = 0;
			copyHeight = 0;
		}

		if (dst_x == 0 && dst_y == 0 && src_x == 0 && src_y == 0 && src_w == 0 && src_h == 0) {
			device->context->CopyBuffer(*dst2d, *src2d);
		}

		RECT RectRegion;
		RectRegion.left = src_x;
		if (src_w > 0)
			RectRegion.right = src_x + src_w;
		else
			RectRegion.right = src2d->width - 1;

		RectRegion.top = src_y;
		if (src_h > 0)
			RectRegion.bottom = src_y + src_h;
		else
			RectRegion.bottom = src2d->height - 1;
		device->context->CopyTextureRegion(*dst, dst_x, dst_y, 0, *src, RectRegion);

	} catch (const char *error) {
		blog(LOG_ERROR, "device_copy_texture (D3D12): %s", error);
	}
}

void device_copy_texture(gs_device_t *device, gs_texture_t *dst, gs_texture_t *src)
{
	device_copy_texture_region(device, dst, 0, 0, src, 0, 0, 0, 0);
}

void device_stage_texture(gs_device_t *device, gs_stagesurf_t *dst, gs_texture_t *src)
{
	try {
		gs_texture_2d *src2d = static_cast<gs_texture_2d *>(src);

		if (!src)
			throw "Source texture is NULL";
		if (src->type != GS_TEXTURE_2D)
			throw "Source texture must be a 2D texture";
		if (!dst)
			throw "Destination surface is NULL";
		if (dst->format != GS_UNKNOWN && dst->format != src->format)
			throw "Source and destination formats do not match";
		if (dst->width != src2d->width || dst->height != src2d->height)
			throw "Source and destination must have the same "
			      "dimensions";

		device->context->ReadbackTexture(*dst, *src);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_copy_texture (D3D12): %s", error);
	}
}

extern "C" void reset_duplicators(void);
void device_begin_frame(gs_device_t *device)
{
	reset_duplicators();
	if (!device->context) {
		device->context = device->d3d12Instance->GetNewGraphicsContext();
	}
}

void device_end_frame(gs_device_t *device)
{
	device->context->Finish();
	device->context = device->d3d12Instance->GetNewGraphicsContext();
}

void device_begin_scene(gs_device_t *device)
{
	UNUSED_PARAMETER(device);
}

void device_draw(gs_device_t *device, enum gs_draw_mode draw_mode, uint32_t start_vert, uint32_t num_verts)
{
	try {
		if (!device->curVertexShader)
			throw "No vertex shader specified";

		if (!device->curPixelShader)
			throw "No pixel shader specified";

		if (!device->curVertexBuffer && (num_verts == 0))
			throw "No vertex buffer specified";

		if (!device->curSwapChain && !device->curRenderTarget)
			throw "No render target or swap chain to render to";

		device->FlushOutputViews();
		gs_effect_t *effect = gs_get_effect();
		if (effect)
			gs_effect_update_params(effect);

		device->UpdateViewProjMatrix();

		device->curToplogy = ConvertGSTopology(draw_mode);

		device->LoadVertexBufferData();

		device->context->SetVertexBuffers(0, (UINT)device->curVertexBufferViews.size(),
						  device->curVertexBufferViews.data());

		std::unique_ptr<D3D12Graphics::RootSignature> rootSig =
			std::make_unique<D3D12Graphics::RootSignature>(device->d3d12Instance);
		device->LoadRootSignature(rootSig);

		std::unique_ptr<D3D12Graphics::GraphicsPSO> pso =
			std::make_unique<D3D12Graphics::GraphicsPSO>(device->d3d12Instance);
		device->LoadCurrentGraphicsPSO(pso, rootSig);

		device->context->SetPipelineState(*(pso));
		device->context->SetRootSignature(*(rootSig));

		gs_samplerstate_t *states[GS_MAX_TEXTURES] = {0};
		device->curPixelShader->GetSamplerStates(states);

		for (int32_t i = 0; i < GS_MAX_TEXTURES; ++i) {
			if (states[i] != nullptr) {
				device->context->SetDynamicSampler(device->curPixelShader->samplerRootParameterIndex, i,
								   states[i]->sampler);
			}
		}

		for (int32_t i = 0; i < GS_MAX_TEXTURES; ++i) {
			if (device->curSamplers[i] && device->curSamplers[i]->sampler.ptr != states[i]->sampler.ptr) {
				device->curSamplers[i] = nullptr;
			}
		}

		device->curVertexShader->UploadParams();
		device->curPixelShader->UploadParams();

		D3D12Graphics::Color blendFactor = {1.0f, 1.0f, 1.0f, 1.0f};

		device->context->SetBlendFactor(blendFactor);

		device->context->SetStencilRef(0);
		D3D12_PRIMITIVE_TOPOLOGY newToplogy = ConvertGSTopology(draw_mode);
		device->context->SetPrimitiveTopology(newToplogy);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_draw (D3D12): %s", error);
		return;

	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_draw (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
		return;
	}

	if (device->curIndexBuffer) {
		if (num_verts == 0)
			num_verts = (uint32_t)device->curIndexBuffer->num;
		device->context->DrawIndexedInstanced(num_verts, 1, start_vert, 0, 0);
	} else {
		if (num_verts == 0)
			num_verts = (uint32_t)device->curVertexBuffer->numVerts;
		device->context->DrawInstanced(num_verts, 1, start_vert, 0);
	}
}

void device_end_scene(gs_device_t *device) {}

void device_load_swapchain(gs_device_t *device, gs_swapchain_t *swapchain)
{
	gs_texture_t *target = device->curRenderTarget;
	gs_zstencil_t *zs = device->curZStencilBuffer;
	bool is_cube = device->curRenderTarget ? (device->curRenderTarget->type == GS_TEXTURE_CUBE) : false;

	if (device->curSwapChain) {
		if (target == &device->curSwapChain->target[device->curSwapChain->currentBackBufferIndex])
			target = NULL;
		if (zs == device->curSwapChain->zs.get())
			zs = NULL;
	}

	device->curSwapChain = swapchain;

	if (is_cube) {
		device_set_cube_render_target(device, target, device->curRenderSide, zs);
	} else {
		const enum gs_color_space space = swapchain ? swapchain->space : GS_CS_SRGB;
		device_set_render_target_internal(device, target, zs, space);
	}
}

void device_clear(gs_device_t *device, uint32_t clear_flags, const struct vec4 *color, float depth, uint8_t stencil)
{
	if (clear_flags & GS_CLEAR_COLOR) {
		gs_texture_2d *const tex = device->curRenderTarget;
		if (tex) {
			const int side = device->curRenderSide;
			D3D12_CPU_DESCRIPTOR_HANDLE rtv = device->curFramebufferSrgb ? tex->renderTargetLinearRTV[side]
										     : tex->renderTargetRTV[side];
			device->context->TransitionResource(*tex, D3D12_RESOURCE_STATE_RENDER_TARGET);
			D3D12_RECT Rect;
			Rect.left = 0;
			Rect.top = 0;
			Rect.right = tex->width;
			Rect.bottom = tex->height;
			device->context->ClearColor(rtv, color->ptr, 1, &Rect);
		}
	}

	if (device->curZStencilBuffer) {
		bool clearDepth = false;
		bool clearStencil = false;
		if ((clear_flags & GS_CLEAR_DEPTH) != 0)
			clearDepth = true;
		if ((clear_flags & GS_CLEAR_STENCIL) != 0)
			clearStencil = true;

		if ((clearDepth || clearStencil) && device->curZStencilBuffer) {
			D3D12Graphics::DepthBuffer &depthBuffer =
				(D3D12Graphics::DepthBuffer &)(*device->curZStencilBuffer);
			device->context->ClearDepthAndStencil(depthBuffer);
		}
	}
}

bool device_is_present_ready(gs_device_t *device)
{
	gs_swap_chain *const curSwapChain = device->curSwapChain;
	bool ready = curSwapChain != nullptr;
	if (ready) {
		device->context->TransitionResource(*device->curRenderTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
	} else {
		blog(LOG_WARNING, "device_is_present_ready (D3D12): No active swap");
	}

	return ready;
}

void device_present(gs_device_t *device)
{
	gs_swap_chain *const curSwapChain = device->curSwapChain;
	if (curSwapChain) {
		device->curFramebufferInvalidate = true;
		device->context->TransitionResource(*device->curRenderTarget, D3D12_RESOURCE_STATE_PRESENT);
		device->context->Finish(true);
		device->context = device->d3d12Instance->GetNewGraphicsContext();
		const HRESULT hr = curSwapChain->swap->Present(0, 0);
		if (FAILED(hr)) {
			auto removeReason = device->d3d12Instance->GetDevice()->GetDeviceRemovedReason();
			__debugbreak();
			blog(LOG_ERROR, "device_present (D3D12): IDXGISwapChain::Present failed %08lX", removeReason);
		}

		curSwapChain->currentBackBufferIndex = curSwapChain->swap->GetCurrentBackBufferIndex();
		device->curRenderTarget = &curSwapChain->target[curSwapChain->currentBackBufferIndex];
	} else {
		blog(LOG_WARNING, "device_present (D3D12): No active swap");
	}
}

void device_flush(gs_device_t *device)
{
	if (device->context) {
		device->context->Flush();
	}
}

void device_set_cull_mode(gs_device_t *device, enum gs_cull_mode mode)
{
	if (mode == device->curRasterState.cullMode)
		return;

	device->curRasterState.cullMode = mode;
}

enum gs_cull_mode device_get_cull_mode(const gs_device_t *device)
{
	return device->curRasterState.cullMode;
}

void device_enable_blending(gs_device_t *device, bool enable)
{
	if (enable == device->curBlendState.blendEnabled)
		return;

	device->curBlendState.blendEnabled = enable;
}

void device_enable_depth_test(gs_device_t *device, bool enable)
{
	if (enable == device->curZstencilState.depthEnabled)
		return;

	device->curZstencilState.depthEnabled = enable;
}

void device_enable_stencil_test(gs_device_t *device, bool enable)
{
	if (enable == device->curZstencilState.stencilEnabled)
		return;

	device->curZstencilState.stencilEnabled = enable;
}

void device_enable_stencil_write(gs_device_t *device, bool enable)
{
	if (enable == device->curZstencilState.stencilWriteEnabled)
		return;

	device->curZstencilState.stencilWriteEnabled = enable;
}

void device_enable_color(gs_device_t *device, bool red, bool green, bool blue, bool alpha)
{
	if (device->curBlendState.redEnabled == red && device->curBlendState.greenEnabled == green &&
	    device->curBlendState.blueEnabled == blue && device->curBlendState.alphaEnabled == alpha)
		return;

	device->curBlendState.redEnabled = red;
	device->curBlendState.greenEnabled = green;
	device->curBlendState.blueEnabled = blue;
	device->curBlendState.alphaEnabled = alpha;
}

void device_blend_function(gs_device_t *device, enum gs_blend_type src, enum gs_blend_type dest)
{
	if (device->curBlendState.srcFactorC == src && device->curBlendState.destFactorC == dest &&
	    device->curBlendState.srcFactorA == src && device->curBlendState.destFactorA == dest)
		return;

	device->curBlendState.srcFactorC = src;
	device->curBlendState.destFactorC = dest;
	device->curBlendState.srcFactorA = src;
	device->curBlendState.destFactorA = dest;
}

void device_blend_function_separate(gs_device_t *device, enum gs_blend_type src_c, enum gs_blend_type dest_c,
				    enum gs_blend_type src_a, enum gs_blend_type dest_a)
{
	if (device->curBlendState.srcFactorC == src_c && device->curBlendState.destFactorC == dest_c &&
	    device->curBlendState.srcFactorA == src_a && device->curBlendState.destFactorA == dest_a)
		return;

	device->curBlendState.srcFactorC = src_c;
	device->curBlendState.destFactorC = dest_c;
	device->curBlendState.srcFactorA = src_a;
	device->curBlendState.destFactorA = dest_a;
}

void device_blend_op(gs_device_t *device, enum gs_blend_op_type op)
{
	if (device->curBlendState.op == op)
		return;

	device->curBlendState.op = op;
}

void device_depth_function(gs_device_t *device, enum gs_depth_test test)
{
	if (device->curZstencilState.depthFunc == test)
		return;

	device->curZstencilState.depthFunc = test;
}

static inline void update_stencilside_test(gs_device_t *device, StencilSide &side, gs_depth_test test)
{
	if (side.test == test)
		return;

	side.test = test;
}

void device_stencil_function(gs_device_t *device, enum gs_stencil_side side, enum gs_depth_test test)
{
	int sideVal = (int)side;

	if (sideVal & GS_STENCIL_FRONT)
		update_stencilside_test(device, device->curZstencilState.stencilFront, test);
	if (sideVal & GS_STENCIL_BACK)
		update_stencilside_test(device, device->curZstencilState.stencilBack, test);
}

static inline void update_stencilside_op(gs_device_t *device, StencilSide &side, enum gs_stencil_op_type fail,
					 enum gs_stencil_op_type zfail, enum gs_stencil_op_type zpass)
{
	if (side.fail == fail && side.zfail == zfail && side.zpass == zpass)
		return;

	side.fail = fail;
	side.zfail = zfail;
	side.zpass = zpass;
}

void device_stencil_op(gs_device_t *device, enum gs_stencil_side side, enum gs_stencil_op_type fail,
		       enum gs_stencil_op_type zfail, enum gs_stencil_op_type zpass)
{
	int sideVal = (int)side;

	if (sideVal & GS_STENCIL_FRONT)
		update_stencilside_op(device, device->curZstencilState.stencilFront, fail, zfail, zpass);
	if (sideVal & GS_STENCIL_BACK)
		update_stencilside_op(device, device->curZstencilState.stencilBack, fail, zfail, zpass);
}

void device_set_viewport(gs_device_t *device, int x, int y, int width, int height)
{
	device->context->SetViewportAndScissor(x, y, width, height);

	device->viewport.x = x;
	device->viewport.y = y;
	device->viewport.cx = width;
	device->viewport.cy = height;
}

void device_get_viewport(const gs_device_t *device, struct gs_rect *rect)
{
	memcpy(rect, &device->viewport, sizeof(gs_rect));
}

void device_set_scissor_rect(gs_device_t *device, const struct gs_rect *rect)
{
	D3D12_RECT d3drect;
	if (rect != NULL) {
		d3drect.left = rect->x;
		d3drect.top = rect->y;
		d3drect.right = rect->x + rect->cx;
		d3drect.bottom = rect->y + rect->cy;
		device->context->SetScissor(d3drect);
	}
}

void device_ortho(gs_device_t *device, float left, float right, float top, float bottom, float zNear, float zFar)
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

void device_frustum(gs_device_t *device, float left, float right, float top, float bottom, float zNear, float zFar)
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
	if (tex == nullptr) {
		return;
	}
	delete tex;
}

uint32_t gs_texture_get_width(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return 0;

	return static_cast<const gs_texture_2d *>(tex)->width;
}

uint32_t gs_texture_get_height(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return 0;

	return static_cast<const gs_texture_2d *>(tex)->height;
}

enum gs_color_format gs_texture_get_color_format(const gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return GS_UNKNOWN;

	return static_cast<const gs_texture_2d *>(tex)->format;
}

bool gs_texture_map(gs_texture_t *tex, uint8_t **ptr, uint32_t *linesize)
{
	if (tex->type != GS_TEXTURE_2D) {
		return false;
	}

	gs_texture_2d *texture = (gs_texture_2d *)(tex);

	if (!texture->isDynamic) {
		return false;
	}

	D3D12Graphics::UploadBuffer *upload = texture->uploadBuffer.get();
	*ptr = (uint8_t *)upload->Map();

	*linesize = texture->GetLineSize();

	return !!(*ptr);
}

void gs_texture_unmap(gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D) {
		return;
	}

	gs_texture_2d *texture = (gs_texture_2d *)(tex);

	if (!texture->isDynamic) {
		return;
	}

	D3D12Graphics::UploadBuffer *upload = texture->uploadBuffer.get();
	upload->Unmap();
	texture->device->context->UpdateTexture(*texture, *upload);
	texture->device->context->TransitionResource(*texture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void *gs_texture_get_obj(gs_texture_t *tex)
{
	if (tex->type != GS_TEXTURE_2D)
		return nullptr;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d *>(tex);
	return tex2d->GetResource();
}

void gs_cubetexture_destroy(gs_texture_t *cubetex)
{
	delete cubetex;
}

uint32_t gs_cubetexture_get_size(const gs_texture_t *cubetex)
{
	if (cubetex->type != GS_TEXTURE_CUBE)
		return 0;

	const gs_texture_2d *tex = static_cast<const gs_texture_2d *>(cubetex);
	return tex->width;
}

enum gs_color_format gs_cubetexture_get_color_format(const gs_texture_t *cubetex)
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

enum gs_color_format gs_stagesurface_get_color_format(const gs_stagesurf_t *stagesurf)
{
	return stagesurf->format;
}

bool gs_stagesurface_map(gs_stagesurf_t *stagesurf, uint8_t **data, uint32_t *linesize)
{
	*data = (uint8_t *)stagesurf->Map();
	*linesize = stagesurf->GetLineSize();
	return !!(*data);
}

void gs_stagesurface_unmap(gs_stagesurf_t *stagesurf)
{
	stagesurf->Unmap();
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
			if (samplerstate->device->curSamplers[i] == samplerstate)
				samplerstate->device->curSamplers[i] = nullptr;

	delete samplerstate;
}

void gs_vertexbuffer_destroy(gs_vertbuffer_t *vertbuffer)
{
	delete vertbuffer;
}

static inline void gs_vertexbuffer_flush_internal(gs_vertbuffer_t *vertbuffer, const gs_vb_data *data)
{
	size_t num_tex = data->num_tex < vertbuffer->uvBuffers.size() ? data->num_tex : vertbuffer->uvBuffers.size();

	if (!vertbuffer->dynamic) {
		blog(LOG_ERROR, "gs_vertexbuffer_flush: vertex buffer is "
				"not dynamic");
		return;
	}

	if (data->points)
		vertbuffer->FlushBuffer(vertbuffer->vertexBuffer, data->points, sizeof(vec3));

	if (vertbuffer->normalBuffer && data->normals)
		vertbuffer->FlushBuffer(vertbuffer->normalBuffer, data->normals, sizeof(vec3));

	if (vertbuffer->tangentBuffer && data->tangents)
		vertbuffer->FlushBuffer(vertbuffer->tangentBuffer, data->tangents, sizeof(vec3));

	if (vertbuffer->colorBuffer && data->colors)
		vertbuffer->FlushBuffer(vertbuffer->colorBuffer, data->colors, sizeof(uint32_t));

	for (size_t i = 0; i < num_tex; i++) {
		gs_tvertarray &tv = data->tvarray[i];
		vertbuffer->FlushBuffer(vertbuffer->uvBuffers[i], tv.array, tv.width * sizeof(float));
	}
}

void gs_vertexbuffer_flush(gs_vertbuffer_t *vertbuffer)
{
	gs_vertexbuffer_flush_internal(vertbuffer, vertbuffer->vbd.data);
}

void gs_vertexbuffer_flush_direct(gs_vertbuffer_t *vertbuffer, const gs_vb_data *data)
{
	gs_vertexbuffer_flush_internal(vertbuffer, data);
}

struct gs_vb_data *gs_vertexbuffer_get_data(const gs_vertbuffer_t *vertbuffer)
{
	return vertbuffer->vbd.data;
}

void gs_indexbuffer_destroy(gs_indexbuffer_t *indexbuffer)
{
	delete indexbuffer;
}

static inline void gs_indexbuffer_flush_internal(gs_indexbuffer_t *indexbuffer, const void *data)
{
	if (!indexbuffer->dynamic)
		return;

	indexbuffer->device->context->WriteBuffer(*indexbuffer->indexBuffer, 0, data,
						  indexbuffer->num * indexbuffer->indexSize);
}

void gs_indexbuffer_flush(gs_indexbuffer_t *indexbuffer)
{
	gs_indexbuffer_flush_internal(indexbuffer, indexbuffer->indices.data);
}

void gs_indexbuffer_flush_direct(gs_indexbuffer_t *indexbuffer, const void *data)
{
	gs_indexbuffer_flush_internal(indexbuffer, data);
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

void gs_timer_destroy(gs_timer_t *timer)
{
	delete timer;
}

void gs_timer_begin(gs_timer_t *timer)
{
	// timer->device->context->End(timer->query_begin);
}

void gs_timer_end(gs_timer_t *timer)
{
	// timer->device->context->End(timer->query_end);
}

bool gs_timer_get_data(gs_timer_t *timer, uint64_t *ticks)
{
	/*uint64_t begin, end;
	HRESULT hr_begin, hr_end;
	do {
		hr_begin = timer->device->context->GetData(timer->query_begin, &begin, sizeof(begin), 0);
	} while (hr_begin == S_FALSE);
	do {
		hr_end = timer->device->context->GetData(timer->query_end, &end, sizeof(end), 0);
	} while (hr_end == S_FALSE);

	const bool succeeded = SUCCEEDED(hr_begin) && SUCCEEDED(hr_end);
	if (succeeded)
		*ticks = end - begin;

	return succeeded;*/
	return false;
}

void gs_timer_range_destroy(gs_timer_range_t *range)
{
	delete range;
}

void gs_timer_range_begin(gs_timer_range_t *range)
{
	// range->device->context->Begin(range->query_disjoint);
}

void gs_timer_range_end(gs_timer_range_t *range)
{
	// range->device->context->End(range->query_disjoint);
}

bool gs_timer_range_get_data(gs_timer_range_t *range, bool *disjoint, uint64_t *frequency)
{
	/*D3D11_QUERY_DATA_TIMESTAMP_DISJOINT timestamp_disjoint;
	HRESULT hr;
	do {
		hr = range->device->context->GetData(range->query_disjoint, &timestamp_disjoint,
						     sizeof(timestamp_disjoint), 0);
	} while (hr == S_FALSE);

	const bool succeeded = SUCCEEDED(hr);
	if (succeeded) {
		*disjoint = timestamp_disjoint.Disjoint;
		*frequency = timestamp_disjoint.Frequency;
	}

	return succeeded;*/
	return false;
}

gs_timer::gs_timer(gs_device_t *device) : gs_obj(device, gs_type::gs_timer)
{
	//Rebuild(device->device);
}

gs_timer_range::gs_timer_range(gs_device_t *device) : gs_obj(device, gs_type::gs_timer_range)
{
	//Rebuild(device->device);
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
	const HMONITOR hMonitor = static_cast<HMONITOR>(monitor);
	return device->GetMonitorColorInfo(hMonitor).hdr;
}

extern "C" EXPORT void device_debug_marker_begin(gs_device_t *, const char *markername, const float color[4]) {}

extern "C" EXPORT void device_debug_marker_end(gs_device_t *) {}

extern "C" EXPORT gs_texture_t *device_texture_create_gdi(gs_device_t *device, uint32_t width, uint32_t height)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, width, height, GS_BGRA_UNORM, 1, nullptr, GS_RENDER_TARGET,
					    GS_TEXTURE_2D, true);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "device_texture_create_gdi (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "device_texture_create_gdi (D3D12): %s", error);
	}

	return texture;
}

static inline bool TextureGDICompatible(gs_texture_2d *tex2d, const char *func)
{
	if (!tex2d->isGDICompatible) {
		blog(LOG_ERROR, "%s (D3D11): Texture is not GDI compatible", func);
		return false;
	}

	return true;
}

extern "C" EXPORT void *gs_texture_get_dc(gs_texture_t *tex)
{
	HDC hDC = nullptr;

	if (tex->type != GS_TEXTURE_2D)
		return nullptr;

	gs_texture_2d *tex2d = static_cast<gs_texture_2d *>(tex);
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

	gs_texture_2d *tex2d = static_cast<gs_texture_2d *>(tex);
	if (!TextureGDICompatible(tex2d, "gs_texture_release_dc"))
		return;

	tex2d->gdiSurface->ReleaseDC(nullptr);
}

extern "C" EXPORT gs_texture_t *device_texture_open_shared(gs_device_t *device, uint32_t handle)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, handle);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "gs_texture_open_shared (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "gs_texture_open_shared (D3D12): %s", error);
	}

	return texture;
}

extern "C" EXPORT gs_texture_t *device_texture_open_nt_shared(gs_device_t *device, uint32_t handle)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, handle, true);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "gs_texture_open_nt_shared (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "gs_texture_open_nt_shared (D3D12): %s", error);
	}

	return texture;
}

extern "C" EXPORT uint32_t device_texture_get_shared_handle(gs_texture_t *tex)
{
	gs_texture_2d *tex2d = reinterpret_cast<gs_texture_2d *>(tex);
	if (tex->type != GS_TEXTURE_2D)
		return GS_INVALID_HANDLE;

	return tex2d->isShared ? tex2d->sharedHandle : GS_INVALID_HANDLE;
}

extern "C" EXPORT gs_texture_t *device_texture_wrap_obj(gs_device_t *device, void *obj)
{
	gs_texture *texture = nullptr;
	try {
		texture = new gs_texture_2d(device, (ID3D12Resource *)obj);
	} catch (const HRError &error) {
		blog(LOG_ERROR, "gs_texture_wrap_obj (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	} catch (const char *error) {
		blog(LOG_ERROR, "gs_texture_wrap_obj (D3D12): %s", error);
	}

	return texture;
}

int device_texture_acquire_sync(gs_texture_t *tex, uint64_t key, uint32_t ms)
{
	gs_texture_2d *tex2d = reinterpret_cast<gs_texture_2d *>(tex);
	if (tex->type != GS_TEXTURE_2D)
		return -1;
	ComQIPtr<IDXGIKeyedMutex> keyedMutex(tex2d->GetResource());
	return 0;
}

extern "C" EXPORT int device_texture_release_sync(gs_texture_t *tex, uint64_t key)
{
	gs_texture_2d *tex2d = reinterpret_cast<gs_texture_2d *>(tex);
	if (tex->type != GS_TEXTURE_2D)
		return -1;

	ComQIPtr<IDXGIKeyedMutex> keyedMutex(tex2d->GetResource());
	return 0;
}

extern "C" EXPORT bool device_texture_create_nv12(gs_device_t *device, gs_texture_t **p_tex_y, gs_texture_t **p_tex_uv,
						  uint32_t width, uint32_t height, uint32_t flags)
{
	if (!device->nv12Supported)
		return false;

	*p_tex_y = nullptr;
	*p_tex_uv = nullptr;

	gs_texture_2d *tex_y;
	gs_texture_2d *tex_uv;

	try {
		tex_y = new gs_texture_2d(device, width, height, GS_R8, 1, nullptr, flags, GS_TEXTURE_2D, false, true);
		tex_uv = new gs_texture_2d(device, tex_y->m_pResource, flags);

	} catch (const HRError &error) {
		blog(LOG_ERROR, "gs_texture_create_nv12 (D3D11): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
		return false;

	} catch (const char *error) {
		blog(LOG_ERROR, "gs_texture_create_nv12 (D3D11): %s", error);
		return false;
	}

	tex_y->pairedTexture = tex_uv;
	tex_uv->pairedTexture = tex_y;

	*p_tex_y = tex_y;
	*p_tex_uv = tex_uv;
	return true;
}

extern "C" EXPORT bool device_texture_create_p010(gs_device_t *device, gs_texture_t **p_tex_y, gs_texture_t **p_tex_uv,
						  uint32_t width, uint32_t height, uint32_t flags)
{
	if (!device->p010Supported)
		return false;

	*p_tex_y = nullptr;
	*p_tex_uv = nullptr;

	gs_texture_2d *tex_y;
	gs_texture_2d *tex_uv;

	try {
		tex_y = new gs_texture_2d(device, width, height, GS_R16, 1, nullptr, flags, GS_TEXTURE_2D, false, true);
		tex_uv = new gs_texture_2d(device, tex_y->m_pResource, flags);

	} catch (const HRError &error) {
		blog(LOG_ERROR, "gs_texture_create_p010 (D3D12): %s (%08lX)", error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
		return false;

	} catch (const char *error) {
		blog(LOG_ERROR, "gs_texture_create_p010 (D3D12): %s", error);
		return false;
	}

	tex_y->pairedTexture = tex_uv;
	tex_uv->pairedTexture = tex_y;

	*p_tex_y = tex_y;
	*p_tex_uv = tex_uv;
	return true;
}

extern "C" EXPORT gs_stagesurf_t *device_stagesurface_create_nv12(gs_device_t *device, uint32_t width, uint32_t height)
{
	gs_stage_surface *surf = NULL;
	try {
		surf = new gs_stage_surface(device, width, height, false);
	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_stagesurface_create (D3D12): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return surf;
}

extern "C" EXPORT gs_stagesurf_t *device_stagesurface_create_p010(gs_device_t *device, uint32_t width, uint32_t height)
{
	gs_stage_surface *surf = NULL;
	try {
		surf = new gs_stage_surface(device, width, height, true);
	} catch (const HRError &error) {
		blog(LOG_ERROR,
		     "device_stagesurface_create (D3D11): %s "
		     "(%08lX)",
		     error.str, error.hr);
		LogD3D12ErrorDetails(error, device);
	}

	return surf;
}

extern "C" EXPORT void device_register_loss_callbacks(gs_device_t *device, const gs_device_loss *callbacks)
{
	device->loss_callbacks.emplace_back(*callbacks);
}

extern "C" EXPORT void device_unregister_loss_callbacks(gs_device_t *device, void *data)
{
	for (auto iter = device->loss_callbacks.begin(); iter != device->loss_callbacks.end(); ++iter) {
		if (iter->data == data) {
			device->loss_callbacks.erase(iter);
			break;
		}
	}
}

uint32_t gs_get_adapter_count(void)
{
	uint32_t count = 0;

	ComPtr<IDXGIFactory1> factory;
	HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (SUCCEEDED(hr)) {
		ComPtr<IDXGIAdapter1> adapter;
		for (UINT i = 0; factory->EnumAdapters1(i, adapter.Assign()) == S_OK; ++i) {
			DXGI_ADAPTER_DESC desc;
			if (SUCCEEDED(adapter->GetDesc(&desc))) {
				/* ignore Microsoft's 'basic' renderer' */
				if (desc.VendorId != 0x1414 && desc.DeviceId != 0x8c) {
					++count;
				}
			}
		}
	}

	return count;
}

extern "C" EXPORT bool device_can_adapter_fast_clear(gs_device_t *device)
{
	return device->fastClearSupported;
}
