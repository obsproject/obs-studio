#pragma once

#include <d3d12-common.hpp>

namespace D3D12Graphics {

class D3D12DeviceInstance;

class GpuResource {
public:
	GpuResource()
		: m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
		  m_UsageState(D3D12_RESOURCE_STATE_COMMON),
		  m_TransitioningState((D3D12_RESOURCE_STATES)-1)
	{
	}

	GpuResource(ID3D12Resource *pResource, D3D12_RESOURCE_STATES CurrentState)
		: m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
		  m_pResource(pResource),
		  m_UsageState(CurrentState),
		  m_TransitioningState((D3D12_RESOURCE_STATES)-1)
	{
	}

	~GpuResource() { Destroy(); }

	virtual void Destroy()
	{
		m_pResource = nullptr;
		m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
		++m_VersionID;
	}

	ID3D12Resource *operator->() { return m_pResource.Get(); }
	const ID3D12Resource *operator->() const { return m_pResource.Get(); }

	ID3D12Resource *GetResource() { return m_pResource.Get(); }
	const ID3D12Resource *GetResource() const { return m_pResource.Get(); }

	ID3D12Resource **GetAddressOf() { return &m_pResource; }

	D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

	uint32_t GetVersionID() const { return m_VersionID; }

	ComPtr<ID3D12Resource> m_pResource;
	D3D12_RESOURCE_STATES m_UsageState;
	D3D12_RESOURCE_STATES m_TransitioningState;
	D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;

	// Used to identify when a resource changes so descriptors can be copied etc.
	uint32_t m_VersionID = 0;
};

class UploadBuffer : public GpuResource {
public:
	UploadBuffer(D3D12DeviceInstance *DeviceInstance);
	virtual ~UploadBuffer();

	void Create(const std::wstring &name, size_t BufferSize);

	void *Map(void);
	void Unmap(size_t begin = 0, size_t end = -1);

	size_t GetBufferSize() const;

protected:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	size_t m_BufferSize;
};

class GpuBuffer : public GpuResource {
public:
	GpuBuffer(D3D12DeviceInstance *DeviceInstance);
	virtual ~GpuBuffer();

	virtual void Destroy() override;

	// Create a buffer.  If initial data is provided, it will be copied into the buffer using the default command context.
	void Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize,
		    const void *initialData = nullptr);

	void Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize, const UploadBuffer &srcData,
		    uint32_t srcOffset = 0);

	// Sub-Allocate a buffer out of a pre-allocated heap.  If initial data is provided, it will be copied into the buffer using the default command context.
	void CreatePlaced(const std::wstring &name, ID3D12Heap *pBackingHeap, uint32_t HeapOffset, uint32_t NumElements,
			  uint32_t ElementSize, const void *initialData = nullptr);

	const D3D12_CPU_DESCRIPTOR_HANDLE &GetUAV(void) const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetSRV(void) const;

	D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView(void) const;

	D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView(uint32_t Offset, uint32_t Size) const;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const;
	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t BaseVertexIndex = 0) const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const;
	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t StartIndex = 0) const;

	size_t GetBufferSize() const;
	uint32_t GetElementCount() const;
	uint32_t GetElementSize() const;

	D3D12_RESOURCE_DESC DescribeBuffer(void);
	virtual void CreateDerivedViews(void) = 0;

protected:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;

	size_t m_BufferSize;
	uint32_t m_ElementCount;
	uint32_t m_ElementSize;
	D3D12_RESOURCE_FLAGS m_ResourceFlags;
};

class ByteAddressBuffer : public GpuBuffer {
public:
	ByteAddressBuffer(D3D12DeviceInstance *DeviceInstance);
	virtual void CreateDerivedViews(void) override;
};

class ReadbackBuffer : public GpuBuffer {
public:
	ReadbackBuffer(D3D12DeviceInstance *DeviceInstance);
	virtual ~ReadbackBuffer();

	void Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize);

	void *Map(void);
	void Unmap(void);

protected:
	void CreateDerivedViews(void);
};

class PixelBuffer : public GpuResource {
public:
	PixelBuffer();

	uint32_t GetWidth(void) const;
	uint32_t GetHeight(void) const;
	uint32_t GetDepth(void) const;
	const DXGI_FORMAT &GetFormat(void) const;

	void SetBankRotation(uint32_t RotationAmount);

	D3D12_RESOURCE_DESC DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips,
					  DXGI_FORMAT Format, UINT Flags);

	void AssociateWithResource(ID3D12Device *Device, const std::wstring &Name, ID3D12Resource *Resource,
				   D3D12_RESOURCE_STATES CurrentState);

	void CreateTextureResource(ID3D12Device *Device, const std::wstring &Name,
				   const D3D12_RESOURCE_DESC &ResourceDesc, D3D12_CLEAR_VALUE ClearValue,
				   D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	DXGI_FORMAT GetBaseFormat();
	DXGI_FORMAT GetUAVFormat();
	DXGI_FORMAT GetDSVFormat();
	DXGI_FORMAT GetDepthFormat();
	DXGI_FORMAT GetStencilFormat();
	static size_t BytesPerPixel(DXGI_FORMAT Format);

protected:
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_ArraySize;
	DXGI_FORMAT m_Format;
	uint32_t m_BankRotation;
};

class DepthBuffer : public PixelBuffer {
public:
	DepthBuffer(D3D12DeviceInstance *DeviceInstance, float ClearDepth = 0.0f, uint8_t ClearStencil = 0);
	virtual ~DepthBuffer();
	// Create a depth buffer.  If an address is supplied, memory will not be allocated.
	// The vmem address allows you to alias buffers (which can be especially useful for
	// reusing ESRAM across a frame.)
	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
		    D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
		    D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
	virtual void Destroy() override;

	// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV_DepthReadOnly() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV_StencilReadOnly() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV_ReadOnly() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDepthSRV() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetStencilSRV() const;

	float GetClearDepth() const;
	uint8_t GetClearStencil() const;

	void CreateDerivedViews(ID3D12Device *Device);

protected:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	float m_ClearDepth;
	uint8_t m_ClearStencil;
	D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4];
	D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
};

} // namespace D3D12Graphics
