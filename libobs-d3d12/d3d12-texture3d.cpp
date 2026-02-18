#include <util/base.h>
#include "d3d12-subsystem.hpp"

void gs_texture_3d::InitSRD(std::vector<D3D12_SUBRESOURCE_DATA> &srd)
{
	uint32_t rowSizeBits = width * gs_get_format_bpp(format);
	uint32_t sliceSizeBytes = height * rowSizeBits / 8;
	uint32_t actual_levels = levels;

	if (!actual_levels)
		actual_levels = gs_get_total_levels(width, height, depth);

	uint32_t newRowSize = rowSizeBits / 8;
	uint32_t newSlizeSize = sliceSizeBytes;

	for (uint32_t level = 0; level < actual_levels; ++level) {
		D3D12_SUBRESOURCE_DATA newSRD;
		newSRD.pData = data[level].data();
		newSRD.RowPitch = newRowSize;
		newSRD.SlicePitch = newSlizeSize;
		srd.push_back(newSRD);

		newRowSize /= 2;
		newSlizeSize /= 4;
	}
}

void gs_texture_3d::BackupTexture(const uint8_t *const *data)
{
	this->data.resize(levels);

	uint32_t w = width;
	uint32_t h = height;
	uint32_t d = depth;
	const uint32_t bbp = gs_get_format_bpp(format);

	for (uint32_t i = 0; i < levels; i++) {
		if (!data[i])
			break;

		const uint32_t texSize = bbp * w * h * d / 8;
		this->data[i].resize(texSize);

		std::vector<uint8_t> &subData = this->data[i];
		memcpy(&subData[0], data[i], texSize);

		if (w > 1)
			w /= 2;
		if (h > 1)
			h /= 2;
		if (d > 1)
			d /= 2;
	}
}

void gs_texture_3d::GetSharedHandle(IDXGIResource *dxgi_res)
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

void gs_texture_3d::InitTexture(const uint8_t *const *data)
{
	Destroy();

	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
	texDesc.Width = width;
	texDesc.Height = height;
	texDesc.DepthOrArraySize = depth;

	texDesc.MipLevels = genMipmaps ? 0 : levels;

	texDesc.Format = dxgiFormatResource;

	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

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
	}

	if (data) {
		BackupTexture(data);
		InitSRD(srd);
	}

	HRESULT hr = device->d3d12Instance->GetDevice()->CreateCommittedResource(
		&HeapProps, isShared ? D3D12_HEAP_FLAG_SHARED : D3D12_HEAP_FLAG_NONE, &texDesc, m_UsageState, nullptr,
		IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		auto removeReason = device->d3d12Instance->GetDevice()->GetDeviceRemovedReason();
		throw HRError("Failed to create 2D texture resource", removeReason);
	}

	if (data) {
		device->d3d12Instance->InitializeTexture(*this, (UINT)srd.size(), srd.data());
	}

	if (isShared) {
		hr = device->d3d12Instance->GetDevice()->CreateSharedHandle(m_pResource.Get(), nullptr, GENERIC_ALL,
									    nullptr, (HANDLE *)(&sharedHandle));
		if (FAILED(hr)) {
			throw HRError("Create Shared Handle Failed", hr);
		}

		acquired = true;
	}
}

void gs_texture_3d::InitResourceView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = dxgiFormatView;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	SRVDesc.Texture3D.MipLevels = genMipmaps || !levels ? -1 : levels;
	SRVDesc.Texture3D.MostDetailedMip = 0;

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

#define SHARED_FLAGS (GS_SHARED_TEX | GS_SHARED_KM_TEX)

gs_texture_3d::gs_texture_3d(gs_device_t *device, uint32_t width, uint32_t height, uint32_t depth,
			     gs_color_format colorFormat, uint32_t levels, const uint8_t *const *data, uint32_t flags_)
	: gs_texture(device, gs_type::gs_texture_3d, GS_TEXTURE_3D, levels, colorFormat),
	  width(width),
	  height(height),
	  depth(depth),
	  flags(flags_),
	  dxgiFormatResource(ConvertGSTextureFormatResource(format)),
	  dxgiFormatView(ConvertGSTextureFormatView(format)),
	  dxgiFormatViewLinear(ConvertGSTextureFormatViewLinear(format)),
	  isDynamic((flags_ & GS_DYNAMIC) != 0),
	  isShared((flags_ & SHARED_FLAGS) != 0),
	  genMipmaps((flags_ & GS_BUILD_MIPMAPS) != 0),
	  sharedHandle(NULL)
{
	InitTexture(data);
	InitResourceView();
}

gs_texture_3d::gs_texture_3d(gs_device_t *device, uint32_t handle)
	: gs_texture(device, gs_type::gs_texture_3d, GS_TEXTURE_3D),
	  isShared(true),
	  sharedHandle(handle)
{
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

gs_texture_3d::~gs_texture_3d()
{
	Destroy();
}

void gs_texture_3d::Destroy()
{
	if (device) {
		device->d3d12Instance->GetCommandManager().IdleGPU();
	}

	if (shaderSRV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		shaderSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (shaderLinearSRV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		shaderLinearSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	GpuResource::Destroy();
}
