#include "d3d12-subsystem.hpp"

void gs_vertex_buffer::Rebuild()
{
	uvBuffers.clear();
	uvSizes.clear();
	BuildBuffers();
}

void gs_index_buffer::Rebuild(ID3D12Device *dev)
{
	Release();
	indexBuffer = new D3D12Graphics::ByteAddressBuffer(device->d3d12Instance);
	indexBuffer->Create(L"Index Buffer", (uint32_t)num, (uint32_t)indexSize, indices.data);

	if (indexBuffer->GetResource() == nullptr) {
		throw HRError("Failed to create buffer", E_FAIL);
	}
}

void gs_texture_2d::RebuildSharedTextureFallback()
{
	static const gs_color_format format = GS_BGRA;
	static const DXGI_FORMAT dxgi_format_resource = ConvertGSTextureFormatResource(format);
	static const DXGI_FORMAT dxgi_format_view = ConvertGSTextureFormatView(format);
	static const DXGI_FORMAT dxgi_format_view_linear = ConvertGSTextureFormatViewLinear(format);

	texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = 2;
	texDesc.Height = 2;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = dxgi_format_resource;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	width = (uint32_t)texDesc.Width;
	height = (uint32_t)texDesc.Height;
	dxgiFormatResource = dxgi_format_resource;
	dxgiFormatView = dxgi_format_view;
	dxgiFormatViewLinear = dxgi_format_view_linear;
	levels = 1;

	isShared = false;
}

void gs_texture_2d::RebuildResource(ID3D12Device *dev)
{
	HRESULT hr;

	if (isShared) {
		hr = dev->OpenSharedHandle((HANDLE)(uintptr_t)sharedHandle, IID_PPV_ARGS(&m_pResource));
		if (FAILED(hr)) {
			blog(LOG_WARNING, "Failed to rebuild shared texture: 0x%08lX", hr);
			RebuildSharedTextureFallback();
		}
	}

	if (!isShared) {
		D3D12_HEAP_PROPERTIES HeapProps;
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		hr = dev->CreateCommittedResource(&HeapProps, isShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE,
						  &texDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource));
		if (FAILED(hr)) {
			auto removeReason = device->d3d12Instance->GetDevice()->GetDeviceRemovedReason();
			throw HRError("Failed to create 2D texture resource", removeReason);
		}

		if (srd.data()) {
			device->d3d12Instance->InitializeTexture(*this, (UINT)srd.size(), srd.data());
		}
	}

	if (isDynamic) {
		uploadBuffer = std::make_unique<D3D12Graphics::UploadBuffer>(device->d3d12Instance);
		uploadBuffer->Create(L"Texture2D Upload Buffer",
				     D3D12Graphics::GetRequiredIntermediateSize(GetResource(), 0,
										srd.data() ? (UINT)srd.size() : 1));
	}
}

void gs_texture_2d::Rebuild(ID3D12Device *dev)
{
	RebuildResource(dev);
	InitResourceView();

	if (isRenderTarget) {
		InitRenderTargets();
	}

	acquired = false;
}

void gs_texture_2d::RebuildPaired_Y(ID3D12Device *dev)
{
	gs_texture_2d *tex_uv = pairedTexture;
	RebuildResource(dev);
	InitResourceView();

	if (isRenderTarget) {
		InitRenderTargets();
	}

	if (tex_uv) {
		tex_uv->RebuildPaired_UV(dev);
	}

	acquired = false;
}

void gs_texture_2d::RebuildPaired_UV(ID3D12Device *dev)
{
	gs_texture_2d *tex_y = pairedTexture;

	m_pResource = tex_y->m_pResource;
	InitResourceView(1);

	if (isRenderTarget) {
		InitRenderTargets(1);
	}

	acquired = false;
}

void gs_texture_3d::RebuildSharedTextureFallback()
{
	static const gs_color_format format = GS_BGRA;
	static const DXGI_FORMAT dxgi_format_resource = ConvertGSTextureFormatResource(format);
	static const DXGI_FORMAT dxgi_format_view = ConvertGSTextureFormatView(format);
	static const DXGI_FORMAT dxgi_format_view_linear = ConvertGSTextureFormatViewLinear(format);

	texDesc = {};
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	texDesc.Width = 2;
	texDesc.Height = 2;
	texDesc.DepthOrArraySize = 2;
	texDesc.MipLevels = 1;
	texDesc.Format = dxgi_format_resource;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	width = (uint32_t)texDesc.Width;
	height = (uint32_t)texDesc.Height;
	depth = (uint32_t)texDesc.DepthOrArraySize;
	dxgiFormatResource = dxgi_format_resource;
	dxgiFormatView = dxgi_format_view;
	dxgiFormatViewLinear = dxgi_format_view_linear;
	levels = 1;

	isShared = false;
}

void gs_texture_3d::Rebuild(ID3D12Device *dev)
{
	HRESULT hr;

	if (isShared) {
		hr = dev->OpenSharedHandle((HANDLE)(uintptr_t)sharedHandle, IID_PPV_ARGS(&m_pResource));
		if (FAILED(hr)) {
			blog(LOG_WARNING, "Failed to rebuild shared 3D texture: 0x%08lX", hr);
			RebuildSharedTextureFallback();
		}
	}

	if (!isShared) {
		D3D12_HEAP_PROPERTIES HeapProps;
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		HeapProps.CreationNodeMask = 1;
		HeapProps.VisibleNodeMask = 1;

		hr = dev->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
						  D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource));
		if (FAILED(hr)) {
			throw HRError("Failed to create 3D texture resource", hr);
		}

		if (data.size() > 0 && srd.size() > 0) {
			device->d3d12Instance->InitializeTexture(*this, (UINT)srd.size(), srd.data());
		}
	}

	InitResourceView();
	acquired = false;
}

void gs_texture_3d::RebuildNV12_Y(ID3D12Device *dev)
{
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	HRESULT hr = dev->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &texDesc,
						  D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("Failed to create NV12 Y texture", hr);
	}

	InitResourceView();
	acquired = false;
}

void gs_texture_3d::RebuildNV12_UV(ID3D12Device *dev)
{
	// UV component shares the same resource as Y
	// This is handled via shared handle or paired reference
	InitResourceView();
}

void gs_zstencil_buffer::Rebuild(ID3D12Device *dev)
{
	D3D12_RESOURCE_DESC desc = {};
	desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	desc.Width = width;
	desc.Height = height;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = ConvertGSZStencilFormat(format);
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	HRESULT hr = dev->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &desc,
						  D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr,
						  IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("Failed to create depth stencil texture", hr);
	}

	// Create DSV descriptor
	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = ConvertGSZStencilFormat(format);
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
		device->d3d12Instance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	dev->CreateDepthStencilView(m_pResource.Get(), &dsvDesc, dsvHandle);
}

void gs_stage_surface::Rebuild(ID3D12Device *dev)
{
	D3D12_RESOURCE_DESC desc = GetTextureDesc();

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	HRESULT hr = dev->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &desc,
						  D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("Failed to create readback buffer", hr);
	}
}

void gs_sampler_state::Rebuild(ID3D12Device *dev)
{
	// D3D12 sampler descriptors are stateless and created once
	// This method exists for API compatibility
}

void gs_vertex_shader::Rebuild(ID3D12Device *dev)
{
	// D3D12 shaders are immutable bytecode
	// Reset param states for consistency
	for (gs_shader_param &param : params) {
		param.nextSampler = nullptr;
		param.curValue.clear();
		gs_shader_set_default(&param);
	}
}

void gs_pixel_shader::Rebuild(ID3D12Device *dev)
{
	// D3D12 shaders are immutable bytecode
	// Reset param states for consistency
	for (gs_shader_param &param : params) {
		param.nextSampler = nullptr;
		param.curValue.clear();
		gs_shader_set_default(&param);
	}
}

void gs_swap_chain::Rebuild(ID3D12Device *dev)
{
	// Swap chain is typically not recreated on device reset
	// Just reconfigure the back buffers
	Resize(initData.cx, initData.cy, initData.format);
}

void gs_device::RebuildDevice()
try {
	ID3D12Device *dev = nullptr;

	blog(LOG_WARNING, "Device Remove/Reset!  Rebuilding all assets...");

	/* ----------------------------------------------------------------- */

	for (gs_device_loss &callback : loss_callbacks)
		callback.device_loss_release(callback.data);

	gs_obj *obj = first_obj;

	while (obj) {
		switch (obj->obj_type) {
		case gs_type::gs_vertex_buffer:
			((gs_vertex_buffer *)obj)->Release();
			break;
		case gs_type::gs_index_buffer:
			((gs_index_buffer *)obj)->Release();
			break;
		case gs_type::gs_texture_2d:
			((gs_texture_2d *)obj)->Release();
			break;
		case gs_type::gs_zstencil_buffer:
			((gs_zstencil_buffer *)obj)->Release();
			break;
		case gs_type::gs_stage_surface:
			((gs_stage_surface *)obj)->Release();
			break;
		case gs_type::gs_sampler_state:
			((gs_sampler_state *)obj)->Release();
			break;
		case gs_type::gs_vertex_shader:
			((gs_vertex_shader *)obj)->Release();
			break;
		case gs_type::gs_pixel_shader:
			((gs_pixel_shader *)obj)->Release();
			break;
		case gs_type::gs_duplicator:
			((gs_duplicator *)obj)->Release();
			break;
		case gs_type::gs_swap_chain:
			((gs_swap_chain *)obj)->Release();
			break;
		case gs_type::gs_timer:
			((gs_timer *)obj)->Release();
			break;
		case gs_type::gs_timer_range:
			((gs_timer_range *)obj)->Release();
			break;
		case gs_type::gs_texture_3d:
			((gs_texture_3d *)obj)->Release();
			break;
		}

		obj = obj->next;
	}

	// Flush GPU commands
	if (curContext) {
		curContext->Flush(true);
		curContext->SetNullRenderTarget();
	}

	/* ----------------------------------------------------------------- */

	// Get device - D3D12 device is managed by DeviceInstance
	dev = d3d12Instance->GetDevice();

	/* ----------------------------------------------------------------- */

	obj = first_obj;
	while (obj) {
		switch (obj->obj_type) {
		case gs_type::gs_vertex_buffer:
			((gs_vertex_buffer *)obj)->Rebuild();
			break;
		case gs_type::gs_index_buffer:
			((gs_index_buffer *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_texture_2d: {
			gs_texture_2d *tex = (gs_texture_2d *)obj;
			if (!tex->pairedTexture) {
				tex->Rebuild(dev);
			} else if (!tex->chroma) {
				tex->RebuildPaired_Y(dev);
			}
		} break;
		case gs_type::gs_zstencil_buffer:
			((gs_zstencil_buffer *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_stage_surface:
			((gs_stage_surface *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_sampler_state:
			((gs_sampler_state *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_vertex_shader:
			((gs_vertex_shader *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_pixel_shader:
			((gs_pixel_shader *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_duplicator:
			try {
				((gs_duplicator *)obj)->Start();
			} catch (...) {
				((gs_duplicator *)obj)->Release();
			}
			break;
		case gs_type::gs_swap_chain:
			((gs_swap_chain *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_timer:
			((gs_timer *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_timer_range:
			((gs_timer_range *)obj)->Rebuild(dev);
			break;
		case gs_type::gs_texture_3d:
			((gs_texture_3d *)obj)->Rebuild(dev);
			break;
		}

		obj = obj->next;
	}

	// Reset device state
	curRenderTarget = nullptr;
	curZStencilBuffer = nullptr;
	curRenderSide = 0;
	memset(&curTextures, 0, sizeof(curTextures));
	memset(&curSamplers, 0, sizeof(curSamplers));
	curVertexBuffer = nullptr;
	curIndexBuffer = nullptr;
	curVertexShader = nullptr;
	curPixelShader = nullptr;
	curSwapChain = nullptr;
	zstencilStateChanged = true;
	rasterStateChanged = true;
	blendStateChanged = true;
	curFramebufferInvalidate = true;
	curToplogy = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	for (gs_device_loss &callback : loss_callbacks)
		callback.device_loss_rebuild(dev, callback.data);

	blog(LOG_WARNING, "Device rebuild complete!");

} catch (const char *error) {
	bcrash("Failed to recreate D3D12: %s", error);

} catch (const HRError &error) {
	bcrash("Failed to recreate D3D12: %s (%08lX)", error.str, error.hr);
}
