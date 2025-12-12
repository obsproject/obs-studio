#include <util/base.h>
#include "d3d12-subsystem.hpp"

void gs_texture_2d::InitSRD(std::vector<D3D12_SUBRESOURCE_DATA> &srd)
{
	uint32_t rowSizeBytes = width * gs_get_format_bpp(format);
	uint32_t texSizeBytes = height * rowSizeBytes / 8;
	size_t textures = type == GS_TEXTURE_2D ? 1 : 6;
	uint32_t actual_levels = levels;
	size_t curTex = 0;

	if (!actual_levels)
		actual_levels = gs_get_total_levels(width, height, 1);

	rowSizeBytes /= 8;

	for (size_t i = 0; i < textures; i++) {
		uint32_t newRowSize = rowSizeBytes;
		uint32_t newTexSize = texSizeBytes;

		for (uint32_t j = 0; j < actual_levels; j++) {
			D3D12_SUBRESOURCE_DATA newSRD;
			newSRD.pData = data[curTex++].data();
			newSRD.RowPitch = newRowSize;
			newSRD.SlicePitch = newTexSize;
			srd.push_back(newSRD);

			newRowSize /= 2;
			newTexSize /= 4;
		}
	}
}

void gs_texture_2d::BackupTexture(const uint8_t *const *data)
{
	uint32_t textures = type == GS_TEXTURE_CUBE ? 6 : 1;
	uint32_t bbp = gs_get_format_bpp(format);

	this->data.resize(levels * textures);

	for (uint32_t t = 0; t < textures; t++) {
		uint32_t w = width;
		uint32_t h = height;

		for (uint32_t lv = 0; lv < levels; lv++) {
			uint32_t i = levels * t + lv;
			if (!data[i])
				break;

			uint32_t texSize = bbp * w * h / 8;

			std::vector<uint8_t> &subData = this->data[i];
			subData.resize(texSize);
			memcpy(&subData[0], data[i], texSize);

			if (w > 1)
				w /= 2;
			if (h > 1)
				h /= 2;
		}
	}
}

void gs_texture_2d::GetSharedHandle(IDXGIResource *dxgi_res)
{
	HANDLE handle;
	HRESULT hr;

	hr = dxgi_res->GetSharedHandle(&handle);
	if (FAILED(hr)) {
		blog(LOG_WARNING,
		     "GetSharedHandle: Failed to "
		     "get shared handle: %08lX",
		     hr);
	} else {
		sharedHandle = (uint32_t)(uintptr_t)handle;
	}
}

void gs_texture_2d::InitTexture(const uint8_t *const *data)
{
	Destroy();

	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = type == GS_TEXTURE_CUBE ? 6 : 1;

	texDesc.MipLevels = genMipmaps ? 0 : levels;

	texDesc.Format = twoPlane ? ((format == GS_R16) ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12) : dxgiFormatResource;

	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;
	texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	if (isShared) {
		texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
		texDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER;
	}

	if (isRenderTarget || isGDICompatible) {
		m_UsageState = D3D12_RESOURCE_STATE_COMMON;
		texDesc.Flags = D3D12_RESOURCE_FLAG_NONE | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS |
				D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	}

	if (data) {
		BackupTexture(data);
		InitSRD(srd);
	}

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = dxgiFormatView;
	ClearValue.Color[1] = 0;
	ClearValue.Color[2] = 0;
	ClearValue.Color[3] = 0;

	if (texDesc.Format == DXGI_FORMAT_NV12 || texDesc.Format == DXGI_FORMAT_P010) {
		texDesc.Width = (texDesc.Width + 1) & ~1;
		texDesc.Height = (texDesc.Height + 1) & ~1;
	}

	HRESULT hr = device->d3d12Instance->GetDevice()->CreateCommittedResource(
		&HeapProps, isShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE, &texDesc, m_UsageState, nullptr,
		IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		hr = device->d3d12Instance->GetDevice()->GetDeviceRemovedReason();
		throw HRError("Failed to create 2D texture resource", hr);
	}

	if (data) {
		device->d3d12Instance->InitializeTexture(*this, (UINT)srd.size(), srd.data());
	}
	if (isDynamic) {
		uploadBuffer = std::make_unique<D3D12Graphics::UploadBuffer>(device->d3d12Instance);
		uploadBuffer->Create(L"Texture2D Upload Buffer",
				     D3D12Graphics::GetRequiredIntermediateSize(GetResource(), 0,
										data ? (UINT)srd.size() : 1));
	}

	if (isShared) {
		hr = device->d3d12Instance->GetDevice()->CreateSharedHandle(
			m_pResource.Get(), nullptr, GENERIC_ALL, nullptr, (HANDLE *)(uintptr_t)&sharedHandle);
		if (FAILED(hr)) {
			throw HRError("Create Shared Handle Failed", hr);
		}
	}
}

void gs_texture_2d::InitResourceView(int32_t PlaneSliceCount)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = dxgiFormatView;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	if (type == GS_TEXTURE_CUBE) {
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		SRVDesc.TextureCube.MipLevels = genMipmaps || !levels ? -1 : levels;
		SRVDesc.TextureCube.MostDetailedMip = 0;
	} else {
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = genMipmaps || !levels ? -1 : levels;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		SRVDesc.Texture2D.PlaneSlice = PlaneSliceCount;
	}

	if (shaderSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		shaderSRV = device->d3d12Instance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	ID3D12Resource *Resource = m_pResource.Get();
	device->d3d12Instance->GetDevice()->CreateShaderResourceView(Resource, &SRVDesc, shaderSRV);

	SRVDesc.Format = dxgiFormatViewLinear;

	if (shaderLinearSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		shaderLinearSRV = device->d3d12Instance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	device->d3d12Instance->GetDevice()->CreateShaderResourceView(Resource, &SRVDesc, shaderLinearSRV);
}

void gs_texture_2d::InitRenderTargets(int32_t PlaneSliceCount)
{
	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	if (type == GS_TEXTURE_2D) {
		RTVDesc.Format = dxgiFormatView;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;
		RTVDesc.Texture2D.PlaneSlice = PlaneSliceCount;

		renderTargetRTV[0] = device->d3d12Instance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		device->d3d12Instance->GetDevice()->CreateRenderTargetView(GetResource(), &RTVDesc, renderTargetRTV[0]);

		RTVDesc.Format = dxgiFormatViewLinear;
		renderTargetLinearRTV[0] = device->d3d12Instance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		device->d3d12Instance->GetDevice()->CreateRenderTargetView(GetResource(), &RTVDesc,
									   renderTargetLinearRTV[0]);
	} else {
		RTVDesc.Format = dxgiFormatView;
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.MipSlice = 0;
		RTVDesc.Texture2DArray.ArraySize = 1;

		for (UINT i = 0; i < 6; i++) {
			RTVDesc.Texture2DArray.FirstArraySlice = i;
			renderTargetRTV[0] = device->d3d12Instance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			device->d3d12Instance->GetDevice()->CreateRenderTargetView(GetResource(), &RTVDesc,
										   renderTargetRTV[0]);

			RTVDesc.Format = dxgiFormatViewLinear;
			renderTargetLinearRTV[i] =
				device->d3d12Instance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			device->d3d12Instance->GetDevice()->CreateRenderTargetView(GetResource(), &RTVDesc,
										   renderTargetLinearRTV[i]);
		}
	}
}

void gs_texture_2d::InitUAV() {}

UINT gs_texture_2d::GetLineSize()
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTextureDesc;

	auto textureDesc = m_pResource->GetDesc();
	UINT NumRows;
	UINT64 RowLength;
	UINT64 TotalBytes;
	device->d3d12Instance->GetDevice()->GetCopyableFootprints(&textureDesc, 0, 1, 0, &placedTextureDesc, &NumRows,
								  &RowLength, &TotalBytes);
	return placedTextureDesc.Footprint.RowPitch;
}

void gs_texture_2d::CreateTargetFromSwapChain(gs_device_t *device_, IDXGISwapChain *swap, int32_t bufferIndex,
					      enum gs_color_format resFormat, const gs_init_data &initData)
{
	device = device_;
	isRenderTarget = true;
	format = initData.format;
	dxgiFormatResource = ConvertGSTextureFormatResource(resFormat);
	dxgiFormatView = ConvertGSTextureFormatView(resFormat);
	dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(resFormat);

	width = initData.cx;
	height = initData.cy;

	HRESULT hr = swap->GetBuffer(bufferIndex, IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("Failed to get swap chain buffer", hr);
	}

	std::wstring bufferName = std::to_wstring(bufferIndex) + L" - Swap Chain Back Buffer";
	m_pResource->SetName(bufferName.c_str());
	InitRenderTargets();
}

#define SHARED_FLAGS (GS_SHARED_TEX | GS_SHARED_KM_TEX)

gs_texture_2d::gs_texture_2d(gs_device_t *device, uint32_t width, uint32_t height, gs_color_format colorFormat,
			     uint32_t levels, const uint8_t *const *data, uint32_t flags_, gs_texture_type type,
			     bool gdiCompatible, bool twoPlane_)
	: gs_texture(device, gs_type::gs_texture_2d, type, levels, colorFormat),
	  width(width),
	  height(height),
	  flags(flags_),
	  dxgiFormatResource(ConvertGSTextureFormatResource(format)),
	  dxgiFormatView(ConvertGSTextureFormatView(format)),
	  dxgiFormatViewLinear(ConvertGSTextureFormatViewLinear(format)),
	  isRenderTarget((flags_ & GS_RENDER_TARGET) != 0),
	  isGDICompatible(gdiCompatible),
	  isDynamic((flags_ & GS_DYNAMIC) != 0),
	  isShared((flags_ & SHARED_FLAGS) != 0),
	  genMipmaps((flags_ & GS_BUILD_MIPMAPS) != 0),
	  sharedHandle(GS_INVALID_HANDLE),
	  twoPlane(twoPlane_)
{
	InitTexture(data);
	InitResourceView();

	if (isRenderTarget) {
		InitRenderTargets();
		InitUAV();
	}
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, ID3D12Resource *nv12tex, uint32_t flags_)
	: gs_texture(device, gs_type::gs_texture_2d, GS_TEXTURE_2D),
	  isRenderTarget((flags_ & GS_RENDER_TARGET) != 0),
	  isDynamic((flags_ & GS_DYNAMIC) != 0),
	  isShared((flags_ & SHARED_FLAGS) != 0),
	  genMipmaps((flags_ & GS_BUILD_MIPMAPS) != 0),
	  twoPlane(true)
{
	m_pResource = nv12tex;
	m_UsageState = D3D12_RESOURCE_STATE_COMMON;
	texDesc = nv12tex->GetDesc();

	const bool p010 = texDesc.Format == DXGI_FORMAT_P010;
	const DXGI_FORMAT dxgi_format = p010 ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R8G8_UNORM;

	this->type = GS_TEXTURE_2D;
	this->format = p010 ? GS_RG16 : GS_R8G8;
	this->flags = flags_;
	this->levels = 1;
	this->device = device;
	this->chroma = true;
	this->width = (uint32_t)texDesc.Width / 2;
	this->height = (uint32_t)texDesc.Height / 2;
	this->dxgiFormatResource = dxgi_format;
	this->dxgiFormatView = dxgi_format;
	this->dxgiFormatViewLinear = dxgi_format;

	InitResourceView(1);
	if (isRenderTarget) {
		InitRenderTargets(1);
		InitUAV();
	}
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, uint32_t handle, bool ntHandle)
	: gs_texture(device, gs_type::gs_texture_2d, GS_TEXTURE_2D),
	  isShared(true),
	  sharedHandle(handle)
{
	(void)ntHandle;

	HRESULT hr = device->d3d12Instance->GetDevice()->OpenSharedHandle((HANDLE)(uintptr_t)handle,
									  IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr))
		throw HRError("Failed to open shared 2D texture", hr);

	texDesc = m_pResource->GetDesc();

	const gs_color_format format = ConvertDXGITextureFormat(texDesc.Format);

	this->type = GS_TEXTURE_2D;
	this->format = format;
	this->levels = 1;
	this->device = device;

	this->width = (uint32_t)texDesc.Width;
	this->height = (uint32_t)texDesc.Height;
	this->dxgiFormatResource = ConvertGSTextureFormatResource(format);
	this->dxgiFormatView = ConvertGSTextureFormatView(format);
	this->dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(format);

	InitResourceView();
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, ID3D12Resource *obj)
	: gs_texture(device, gs_type::gs_texture_2d, GS_TEXTURE_2D)
{
	m_pResource = obj;

	texDesc = m_pResource->GetDesc();

	const gs_color_format format = ConvertDXGITextureFormat(texDesc.Format);

	this->type = GS_TEXTURE_2D;
	this->format = format;
	this->levels = 1;
	this->device = device;

	this->width = (uint32_t)texDesc.Width;
	this->height = (uint32_t)texDesc.Height;
	this->dxgiFormatResource = ConvertGSTextureFormatResource(format);
	this->dxgiFormatView = ConvertGSTextureFormatView(format);
	this->dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(format);

	InitResourceView();
}

gs_texture_2d::~gs_texture_2d()
{
	Destroy();
}

void gs_texture_2d::Destroy()
{
	if (device) {
		device->d3d12Instance->GetCommandManager().IdleGPU();
	}

	uploadBuffer.reset();
	if (shaderSRV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		shaderSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (shaderLinearSRV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		shaderLinearSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	for (size_t i = 0; i < 6; ++i) {
		renderTargetRTV[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		renderTargetLinearRTV[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}
	GpuResource::Destroy();
}
