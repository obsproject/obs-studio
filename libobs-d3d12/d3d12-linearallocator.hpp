#pragma once

#include <d3d12-gpubuffer.hpp>

namespace D3D12Graphics {

class D3D12DeviceInstance;

// Various types of allocations may contain NULL pointers.  Check before dereferencing if you are unsure.
struct DynAlloc {
	DynAlloc(GpuResource &BaseResource, size_t ThisOffset, size_t ThisSize)
		: Buffer(BaseResource),
		  Offset(ThisOffset),
		  Size(ThisSize)
	{
	}

	GpuResource &Buffer;                  // The D3D buffer associated with this memory.
	size_t Offset;                        // Offset from start of buffer resource
	size_t Size;                          // Reserved size of this allocation
	void *DataPtr;                        // The CPU-writeable address
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress; // The GPU-visible address
};

class LinearAllocationPage : public GpuResource {
public:
	LinearAllocationPage(ID3D12Resource *pResource, D3D12_RESOURCE_STATES Usage) : GpuResource()
	{
		m_pResource = pResource;
		m_UsageState = Usage;
		m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
		m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
	}

	~LinearAllocationPage() { Unmap(); }

	void Map(void)
	{
		if (m_CpuVirtualAddress == nullptr) {
			m_pResource->Map(0, nullptr, &m_CpuVirtualAddress);
		}
	}

	void Unmap(void)
	{
		if (m_CpuVirtualAddress != nullptr) {
			m_pResource->Unmap(0, nullptr);
			m_CpuVirtualAddress = nullptr;
		}
	}

	void *m_CpuVirtualAddress;
	D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;
};

class LinearAllocatorPageManager {
public:
	LinearAllocatorPageManager(D3D12DeviceInstance *DeviceInstance, LinearAllocatorType Type);
	LinearAllocationPage *RequestPage(void);
	LinearAllocationPage *CreateNewPage(size_t PageSize = 0);

	// Discarded pages will get recycled.  This is for fixed size pages.
	void DiscardPages(uint64_t FenceID, const std::vector<LinearAllocationPage *> &Pages);

	// Freed pages will be destroyed once their fence has passed.  This is for single-use,
	// "large" pages.
	void FreeLargePages(uint64_t FenceID, const std::vector<LinearAllocationPage *> &Pages);

	void Destroy(void);

private:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	LinearAllocatorType m_AllocationType;
	std::vector<std::unique_ptr<LinearAllocationPage>> m_PagePool;
	std::queue<std::pair<uint64_t, LinearAllocationPage *>> m_RetiredPages;
	std::queue<std::pair<uint64_t, LinearAllocationPage *>> m_DeletionQueue;
	std::queue<LinearAllocationPage *> m_AvailablePages;
};

class LinearAllocator {
public:
	LinearAllocator(D3D12DeviceInstance *DeviceInstance, LinearAllocatorType Type);
	DynAlloc Allocate(size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN);
	void CleanupUsedPages(uint64_t FenceID);
	DynAlloc AllocateLargePage(size_t SizeInBytes);

private:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	LinearAllocatorType m_AllocationType;
	size_t m_PageSize;
	size_t m_CurOffset;
	LinearAllocationPage *m_CurPage;
	std::vector<LinearAllocationPage *> m_RetiredPages;
	std::vector<LinearAllocationPage *> m_LargePageList;
};

} // namespace D3D12Graphics
