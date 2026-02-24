#include <d3d12-deviceinstance.hpp>

namespace D3D12Graphics {

UploadBuffer::UploadBuffer(D3D12DeviceInstance *DeviceInstance) : m_DeviceInstance(DeviceInstance), m_BufferSize(0) {}

UploadBuffer::~UploadBuffer()
{
	Destroy();
}

void UploadBuffer::Create(const std::wstring &name, size_t BufferSize)
{
	Destroy();

	m_BufferSize = BufferSize;

	// Create an upload buffer.  This is CPU-visible, but it's write combined memory, so
	// avoid reading back from it.
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	// Upload buffers must be 1-dimensional
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = m_BufferSize;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
									    &ResourceDesc,
									    D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
									    IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("UploadBuffer: CreateCommittedResource failed", hr);
	}

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

#ifdef RELEASE
	(name);
#else
	m_pResource->SetName(name.c_str());
#endif
}

void *UploadBuffer::Map(void)
{
	void *Memory;
	CD3DX12_RANGE temp = CD3DX12_RANGE(0, m_BufferSize);
	m_pResource->Map(0, &temp, &Memory);
	return Memory;
}

void UploadBuffer::Unmap(size_t begin, size_t end)
{
	CD3DX12_RANGE temp = CD3DX12_RANGE(begin, std::min(end, m_BufferSize));
	m_pResource->Unmap(0, &temp);
}

size_t UploadBuffer::GetBufferSize() const
{
	return m_BufferSize;
}

GpuBuffer::GpuBuffer(D3D12DeviceInstance *DeviceInstance)
	: m_DeviceInstance(DeviceInstance),
	  m_BufferSize(0),
	  m_ElementCount(0),
	  m_ElementSize(0)
{
	m_ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

GpuBuffer::~GpuBuffer()
{
	Destroy();
}

void GpuBuffer::Destroy()
{
	if (m_UAV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (m_SRV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	GpuResource::Destroy();
}

void GpuBuffer::Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize, const void *initialData)
{
	Destroy();

	m_ElementCount = NumElements;
	m_ElementSize = ElementSize;
	m_BufferSize = NumElements * ElementSize;

	D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

	m_UsageState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommittedResource(
		&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("GpuBuffer: CreateCommittedResource failed", hr);
	}

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	if (initialData)
		m_DeviceInstance->InitializeBuffer(*this, initialData, m_BufferSize);

#ifdef RELEASE
	(name);
#else
	m_pResource->SetName(name.c_str());
#endif

	CreateDerivedViews();
}

void GpuBuffer::Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize,
		       const UploadBuffer &srcData, uint32_t srcOffset)
{
	Destroy();

	m_ElementCount = NumElements;
	m_ElementSize = ElementSize;
	m_BufferSize = NumElements * ElementSize;

	D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

	m_UsageState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommittedResource(
		&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("GpuBuffer: CreateCommittedResource failed", hr);
	}

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	m_DeviceInstance->InitializeBuffer(*this, srcData, srcOffset);

#ifdef RELEASE
	(name);
#else
	m_pResource->SetName(name.c_str());
#endif

	CreateDerivedViews();
}

// Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
void GpuBuffer::CreatePlaced(const std::wstring &name, ID3D12Heap *pBackingHeap, uint32_t HeapOffset,
			     uint32_t NumElements, uint32_t ElementSize, const void *initialData)
{
	m_ElementCount = NumElements;
	m_ElementSize = ElementSize;
	m_BufferSize = NumElements * ElementSize;

	D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

	m_UsageState = D3D12_RESOURCE_STATE_COMMON;

	HRESULT hr = m_DeviceInstance->GetDevice()->CreatePlacedResource(
		pBackingHeap, HeapOffset, &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("GpuBuffer: CreatePlacedResource failed", hr);
	}

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	if (initialData)
		m_DeviceInstance->InitializeBuffer(*this, initialData, m_BufferSize);

#ifdef RELEASE
	(name);
#else
	m_pResource->SetName(name.c_str());
#endif

	CreateDerivedViews();
}

const D3D12_CPU_DESCRIPTOR_HANDLE &GpuBuffer::GetUAV(void) const
{
	return m_UAV;
}

const D3D12_CPU_DESCRIPTOR_HANDLE &GpuBuffer::GetSRV(void) const
{
	return m_SRV;
}

D3D12_GPU_VIRTUAL_ADDRESS GpuBuffer::RootConstantBufferView(void) const
{
	return m_GpuVirtualAddress;
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuBuffer::CreateConstantBufferView(uint32_t Offset, uint32_t Size) const
{
	if (Offset + Size > m_BufferSize) {
		throw HRError("GpuBuffer: CreateConstantBufferView out of bounds");
	}

	Size = AlignUp(Size, 16);

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = m_GpuVirtualAddress + (size_t)Offset;
	CBVDesc.SizeInBytes = Size;

	D3D12_CPU_DESCRIPTOR_HANDLE hCBV = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_DeviceInstance->GetDevice()->CreateConstantBufferView(&CBVDesc, hCBV);
	return hCBV;
}

D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
{
	D3D12_VERTEX_BUFFER_VIEW VBView;
	VBView.BufferLocation = m_GpuVirtualAddress + Offset;
	VBView.SizeInBytes = Size;
	VBView.StrideInBytes = Stride;
	return VBView;
}

D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t BaseVertexIndex) const
{
	size_t Offset = BaseVertexIndex * m_ElementSize;
	return VertexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize);
}

D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
{
	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = m_GpuVirtualAddress + Offset;
	IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
	IBView.SizeInBytes = Size;
	return IBView;
}

D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t StartIndex) const
{
	size_t Offset = StartIndex * m_ElementSize;
	return IndexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize == 4);
}

size_t GpuBuffer::GetBufferSize() const
{
	return m_BufferSize;
}

uint32_t GpuBuffer::GetElementCount() const
{
	return m_ElementCount;
}

uint32_t GpuBuffer::GetElementSize() const
{
	return m_ElementSize;
}

D3D12_RESOURCE_DESC GpuBuffer::DescribeBuffer(void)
{
	D3D12_RESOURCE_DESC Desc = {};
	Desc.Alignment = 0;
	Desc.DepthOrArraySize = 1;
	Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	Desc.Flags = m_ResourceFlags;
	Desc.Format = DXGI_FORMAT_UNKNOWN;
	Desc.Height = 1;
	Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	Desc.MipLevels = 1;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Width = (UINT64)m_BufferSize;
	return Desc;
}

void GpuBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_SRV = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_DeviceInstance->GetDevice()->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_UAV = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_DeviceInstance->GetDevice()->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
}

ByteAddressBuffer::ByteAddressBuffer(D3D12DeviceInstance *DeviceInstance) : GpuBuffer(DeviceInstance) {}

void ByteAddressBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_SRV = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_DeviceInstance->GetDevice()->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_UAV = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	m_DeviceInstance->GetDevice()->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
}

ReadbackBuffer::ReadbackBuffer(D3D12DeviceInstance *DeviceInstance) : GpuBuffer(DeviceInstance) {}

ReadbackBuffer::~ReadbackBuffer()
{
	Destroy();
}

void ReadbackBuffer::Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize)
{
	Destroy();

	m_ElementCount = NumElements;
	m_ElementSize = ElementSize;
	m_BufferSize = NumElements * ElementSize;
	m_UsageState = D3D12_RESOURCE_STATE_COPY_DEST;

	// Create a readback buffer large enough to hold all texel data
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	// Readback buffers must be 1-dimensional, i.e. "buffer" not "texture2d"
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = m_BufferSize;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE,
									    &ResourceDesc,
									    D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
									    IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		auto remoteReason = m_DeviceInstance->GetDevice()->GetDeviceRemovedReason();
		throw HRError("ReadbackBuffer: CreateCommittedResource failed", remoteReason);
	}

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

#ifdef RELEASE
	(name);
#else
	m_pResource->SetName(name.c_str());
#endif
}

void *ReadbackBuffer::Map(void)
{
	void *Memory;
	CD3DX12_RANGE temp = CD3DX12_RANGE(0, m_BufferSize);
	m_pResource->Map(0, &temp, &Memory);
	return Memory;
}

void ReadbackBuffer::Unmap(void)
{
	CD3DX12_RANGE temp = CD3DX12_RANGE(0, 0);
	m_pResource->Unmap(0, &temp);
}

void ReadbackBuffer::CreateDerivedViews(void) {}

PixelBuffer::PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN), m_BankRotation(0)
{
}

uint32_t PixelBuffer::GetWidth(void) const
{
	return m_Width;
}

uint32_t PixelBuffer::GetHeight(void) const
{
	return m_Height;
}

uint32_t PixelBuffer::GetDepth(void) const
{
	return m_ArraySize;
}

const DXGI_FORMAT &PixelBuffer::GetFormat(void) const
{
	return m_Format;
}

void PixelBuffer::SetBankRotation(uint32_t RotationAmount)
{
	(RotationAmount);
}

D3D12_RESOURCE_DESC PixelBuffer::DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize,
					       uint32_t NumMips, DXGI_FORMAT Format, UINT Flags)
{
	m_Width = Width;
	m_Height = Height;
	m_ArraySize = DepthOrArraySize;
	m_Format = Format;

	D3D12_RESOURCE_DESC Desc = {};
	Desc.Alignment = 0;
	Desc.DepthOrArraySize = (UINT16)DepthOrArraySize;
	Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	Desc.Flags = (D3D12_RESOURCE_FLAGS)Flags;
	Desc.Format = GetBaseFormat();
	Desc.Height = (UINT)Height;
	Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	Desc.MipLevels = (UINT16)NumMips;
	Desc.SampleDesc.Count = 1;
	Desc.SampleDesc.Quality = 0;
	Desc.Width = (UINT64)Width;
	return Desc;
}

void PixelBuffer::AssociateWithResource(ID3D12Device *Device, const std::wstring &Name, ID3D12Resource *Resource,
					D3D12_RESOURCE_STATES CurrentState)
{
	(Device); // Unused until we support multiple adapters
	D3D12_RESOURCE_DESC ResourceDesc = Resource->GetDesc();

	m_pResource.Set(Resource);
	m_UsageState = CurrentState;

	m_Width = (uint32_t)ResourceDesc.Width; // We don't care about large virtual textures yet
	m_Height = ResourceDesc.Height;
	m_ArraySize = ResourceDesc.DepthOrArraySize;
	m_Format = ResourceDesc.Format;

#ifndef RELEASE
	m_pResource->SetName(Name.c_str());
#else
	(Name);
#endif
}

void PixelBuffer::CreateTextureResource(ID3D12Device *Device, const std::wstring &Name,
					const D3D12_RESOURCE_DESC &ResourceDesc, D3D12_CLEAR_VALUE ClearValue,
					D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	Destroy();

	(void)VidMemPtr;

	CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
	HRESULT hr = Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
						     D3D12_RESOURCE_STATE_COMMON, &ClearValue,
						     IID_PPV_ARGS(&m_pResource));
	if (FAILED(hr)) {
		throw HRError("PixelBuffer: CreateCommittedResource failed", hr);
	}

	m_UsageState = D3D12_RESOURCE_STATE_COMMON;
	m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

#ifndef RELEASE
	m_pResource->SetName(Name.c_str());
#else
	(Name);
#endif
}

DXGI_FORMAT PixelBuffer::GetBaseFormat()
{
	switch (m_Format) {
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_TYPELESS;

	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_TYPELESS;

	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8X8_TYPELESS;

	// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_R32G8X24_TYPELESS;

	// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_TYPELESS;

	// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_R24G8_TYPELESS;

	// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_TYPELESS;

	default:
		return m_Format;
	}
}

DXGI_FORMAT PixelBuffer::GetUAVFormat()
{
	switch (m_Format) {
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
		return DXGI_FORMAT_R8G8B8A8_UNORM;

	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8A8_UNORM;

	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return DXGI_FORMAT_B8G8R8X8_UNORM;

	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;

	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_D16_UNORM:
		throw HRError("PixelBuffer: GetUAVFormat called with a depth stencil format.");

	default:
		return m_Format;
	}
}

DXGI_FORMAT PixelBuffer::GetDSVFormat()
{
	switch (m_Format) {
	// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

	// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_D32_FLOAT;

	// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_D24_UNORM_S8_UINT;

	// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_D16_UNORM;

	default:
		return m_Format;
	}
}

DXGI_FORMAT PixelBuffer::GetDepthFormat()
{
	switch (m_Format) {
	// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

	// No Stencil
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
		return DXGI_FORMAT_R32_FLOAT;

	// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

	// 16-bit Z w/o Stencil
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
		return DXGI_FORMAT_R16_UNORM;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

DXGI_FORMAT PixelBuffer::GetStencilFormat()
{
	switch (m_Format) {
	// 32-bit Z w/ Stencil
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return DXGI_FORMAT_X32_TYPELESS_G8X24_UINT;

	// 24-bit Z
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
		return DXGI_FORMAT_X24_TYPELESS_G8_UINT;

	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

size_t PixelBuffer::BytesPerPixel(DXGI_FORMAT Format)
{
	switch (Format) {
	case DXGI_FORMAT_R32G32B32A32_TYPELESS:
	case DXGI_FORMAT_R32G32B32A32_FLOAT:
	case DXGI_FORMAT_R32G32B32A32_UINT:
	case DXGI_FORMAT_R32G32B32A32_SINT:
		return 16;

	case DXGI_FORMAT_R32G32B32_TYPELESS:
	case DXGI_FORMAT_R32G32B32_FLOAT:
	case DXGI_FORMAT_R32G32B32_UINT:
	case DXGI_FORMAT_R32G32B32_SINT:
		return 12;

	case DXGI_FORMAT_R16G16B16A16_TYPELESS:
	case DXGI_FORMAT_R16G16B16A16_FLOAT:
	case DXGI_FORMAT_R16G16B16A16_UNORM:
	case DXGI_FORMAT_R16G16B16A16_UINT:
	case DXGI_FORMAT_R16G16B16A16_SNORM:
	case DXGI_FORMAT_R16G16B16A16_SINT:
	case DXGI_FORMAT_R32G32_TYPELESS:
	case DXGI_FORMAT_R32G32_FLOAT:
	case DXGI_FORMAT_R32G32_UINT:
	case DXGI_FORMAT_R32G32_SINT:
	case DXGI_FORMAT_R32G8X24_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
	case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
	case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
		return 8;

	case DXGI_FORMAT_R10G10B10A2_TYPELESS:
	case DXGI_FORMAT_R10G10B10A2_UNORM:
	case DXGI_FORMAT_R10G10B10A2_UINT:
	case DXGI_FORMAT_R11G11B10_FLOAT:
	case DXGI_FORMAT_R8G8B8A8_TYPELESS:
	case DXGI_FORMAT_R8G8B8A8_UNORM:
	case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
	case DXGI_FORMAT_R8G8B8A8_UINT:
	case DXGI_FORMAT_R8G8B8A8_SNORM:
	case DXGI_FORMAT_R8G8B8A8_SINT:
	case DXGI_FORMAT_R16G16_TYPELESS:
	case DXGI_FORMAT_R16G16_FLOAT:
	case DXGI_FORMAT_R16G16_UNORM:
	case DXGI_FORMAT_R16G16_UINT:
	case DXGI_FORMAT_R16G16_SNORM:
	case DXGI_FORMAT_R16G16_SINT:
	case DXGI_FORMAT_R32_TYPELESS:
	case DXGI_FORMAT_D32_FLOAT:
	case DXGI_FORMAT_R32_FLOAT:
	case DXGI_FORMAT_R32_UINT:
	case DXGI_FORMAT_R32_SINT:
	case DXGI_FORMAT_R24G8_TYPELESS:
	case DXGI_FORMAT_D24_UNORM_S8_UINT:
	case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
	case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
	case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
	case DXGI_FORMAT_R8G8_B8G8_UNORM:
	case DXGI_FORMAT_G8R8_G8B8_UNORM:
	case DXGI_FORMAT_B8G8R8A8_UNORM:
	case DXGI_FORMAT_B8G8R8X8_UNORM:
	case DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM:
	case DXGI_FORMAT_B8G8R8A8_TYPELESS:
	case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
	case DXGI_FORMAT_B8G8R8X8_TYPELESS:
	case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
		return 4;

	case DXGI_FORMAT_R8G8_TYPELESS:
	case DXGI_FORMAT_R8G8_UNORM:
	case DXGI_FORMAT_R8G8_UINT:
	case DXGI_FORMAT_R8G8_SNORM:
	case DXGI_FORMAT_R8G8_SINT:
	case DXGI_FORMAT_R16_TYPELESS:
	case DXGI_FORMAT_R16_FLOAT:
	case DXGI_FORMAT_D16_UNORM:
	case DXGI_FORMAT_R16_UNORM:
	case DXGI_FORMAT_R16_UINT:
	case DXGI_FORMAT_R16_SNORM:
	case DXGI_FORMAT_R16_SINT:
	case DXGI_FORMAT_B5G6R5_UNORM:
	case DXGI_FORMAT_B5G5R5A1_UNORM:
	case DXGI_FORMAT_A8P8:
	case DXGI_FORMAT_B4G4R4A4_UNORM:
		return 2;

	case DXGI_FORMAT_R8_TYPELESS:
	case DXGI_FORMAT_R8_UNORM:
	case DXGI_FORMAT_R8_UINT:
	case DXGI_FORMAT_R8_SNORM:
	case DXGI_FORMAT_R8_SINT:
	case DXGI_FORMAT_A8_UNORM:
	case DXGI_FORMAT_P8:
		return 1;

	default:
		return 0;
	}
}

DepthBuffer::DepthBuffer(D3D12DeviceInstance *DeviceInstance, float ClearDepth, uint8_t ClearStencil)
	: m_DeviceInstance(DeviceInstance),
	  m_ClearDepth(ClearDepth),
	  m_ClearStencil(ClearStencil)
{
	m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

DepthBuffer::~DepthBuffer()
{
	Destroy();
}

void DepthBuffer::Create(const std::wstring &Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
			 D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	Create(Name, Width, Height, 1, Format, VidMemPtr);
}

void DepthBuffer::Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t Samples,
			 DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	D3D12_RESOURCE_DESC ResourceDesc =
		DescribeTex2D(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ResourceDesc.SampleDesc.Count = Samples;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	CreateTextureResource(m_DeviceInstance->GetDevice(), Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(m_DeviceInstance->GetDevice());
}

void DepthBuffer::Destroy()
{
	if (m_hDSV[0].ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (m_hDSV[1].ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (m_hDSV[2].ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (m_hDSV[3].ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (m_hDepthSRV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	if (m_hStencilSRV.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	GpuResource::Destroy();
}

const D3D12_CPU_DESCRIPTOR_HANDLE &DepthBuffer::GetDSV() const
{
	return m_hDSV[0];
}
const D3D12_CPU_DESCRIPTOR_HANDLE &DepthBuffer::GetDSV_DepthReadOnly() const
{
	return m_hDSV[1];
}
const D3D12_CPU_DESCRIPTOR_HANDLE &DepthBuffer::GetDSV_StencilReadOnly() const
{
	return m_hDSV[2];
}
const D3D12_CPU_DESCRIPTOR_HANDLE &DepthBuffer::GetDSV_ReadOnly() const
{
	return m_hDSV[3];
}
const D3D12_CPU_DESCRIPTOR_HANDLE &DepthBuffer::GetDepthSRV() const
{
	return m_hDepthSRV;
}
const D3D12_CPU_DESCRIPTOR_HANDLE &DepthBuffer::GetStencilSRV() const
{
	return m_hStencilSRV;
}

float DepthBuffer::GetClearDepth() const
{
	return m_ClearDepth;
}
uint8_t DepthBuffer::GetClearStencil() const
{
	return m_ClearStencil;
}

void DepthBuffer::CreateDerivedViews(ID3D12Device *Device)
{
	ID3D12Resource *Resource = m_pResource.Get();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat();
	if (Resource->GetDesc().SampleDesc.Count == 1) {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	} else {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	if (m_hDSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hDSV[0] = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_hDSV[1] = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[1]);

	DXGI_FORMAT stencilReadFormat = GetStencilFormat();
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN) {
		if (m_hDSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			m_hDSV[2] = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			m_hDSV[3] = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		}

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[3]);
	} else {
		m_hDSV[2] = m_hDSV[0];
		m_hDSV[3] = m_hDSV[1];
	}

	if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_hDepthSRV = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = GetDepthFormat();
	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D) {
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
	} else {
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Device->CreateShaderResourceView(Resource, &SRVDesc, m_hDepthSRV);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN) {
		if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hStencilSRV = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SRVDesc.Format = stencilReadFormat;
		SRVDesc.Texture2D.PlaneSlice = 1;

		Device->CreateShaderResourceView(Resource, &SRVDesc, m_hStencilSRV);
	}
}

} // namespace D3D12Graphics
