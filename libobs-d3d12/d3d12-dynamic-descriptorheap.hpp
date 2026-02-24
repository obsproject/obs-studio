#pragma once

#include <d3d12-common.hpp>

namespace D3D12Graphics {

class D3D12DeviceInstance;
class CommandContext;
class RootSignature;

class DescriptorHandle {
public:
	DescriptorHandle();
	DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle);

	DescriptorHandle operator+(INT OffsetScaledByDescriptorSize) const
	{
		DescriptorHandle ret = *this;
		ret += OffsetScaledByDescriptorSize;
		return ret;
	}

	void operator+=(INT OffsetScaledByDescriptorSize)
	{
		if (m_CpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_CpuHandle.ptr += OffsetScaledByDescriptorSize;
		if (m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_GpuHandle.ptr += OffsetScaledByDescriptorSize;
	}

	const D3D12_CPU_DESCRIPTOR_HANDLE *operator&() const { return &m_CpuHandle; }
	operator D3D12_CPU_DESCRIPTOR_HANDLE() const { return m_CpuHandle; }
	operator D3D12_GPU_DESCRIPTOR_HANDLE() const { return m_GpuHandle; }

	size_t GetCpuPtr() const { return m_CpuHandle.ptr; }
	uint64_t GetGpuPtr() const { return m_GpuHandle.ptr; }
	bool IsNull() const { return m_CpuHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }
	bool IsShaderVisible() const { return m_GpuHandle.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN; }

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_CpuHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GpuHandle;
};

struct DescriptorTableCache {
	DescriptorTableCache() : AssignedHandlesBitMap(0), TableStart(nullptr), TableSize(0) {}
	uint32_t AssignedHandlesBitMap = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE *TableStart = nullptr;
	uint32_t TableSize = 0;
};

class DescriptorHandleCache {
public:
	DescriptorHandleCache(D3D12DeviceInstance *DeviceInstance);
	void ClearCache();

	uint32_t ComputeStagedSize();
	void CopyAndBindStaleTables(
		D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, DescriptorHandle DestHandleStart,
		ID3D12GraphicsCommandList *CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

	void UnbindAllValid();
	void StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
				    const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
	void ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RootSignature &RootSig);

	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	DescriptorTableCache m_RootDescriptorTable[kMaxNumDescriptorTables];
	D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[kMaxNumDescriptors];
	uint32_t m_RootDescriptorTablesBitMap;
	uint32_t m_StaleRootParamsBitMap;
	uint32_t m_MaxCachedDescriptors;
};

class DynamicDescriptorHeap {
public:
	DynamicDescriptorHeap(D3D12DeviceInstance *DeviceInstance, CommandContext &OwningContext,
			      D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
	~DynamicDescriptorHeap();

	void CleanupUsedHeaps(uint64_t fenceValue);

	// Copy multiple handles into the cache area reserved for the specified root parameter.
	void SetGraphicsDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
					  const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

	void SetComputeDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
					 const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

	// Bypass the cache and upload directly to the shader-visible heap
	D3D12_GPU_DESCRIPTOR_HANDLE UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handles);

	// Deduce cache layout needed to support the descriptor tables needed by the root signature.
	void ParseGraphicsRootSignature(const RootSignature &RootSig);

	void ParseComputeRootSignature(const RootSignature &RootSig);

	// Upload any new descriptors in the cache to the shader-visible heap.
	void CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList *CmdList);

	void CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList *CmdList);

	bool HasSpace(uint32_t Count);
	void RetireCurrentHeap(void);
	void RetireUsedHeaps(uint64_t fenceValue);
	ID3D12DescriptorHeap *GetHeapPointer();

	DescriptorHandle Allocate(UINT Count);

	void CopyAndBindStagedTables(
		DescriptorHandleCache &HandleCache, ID3D12GraphicsCommandList *CmdList,
		void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE));

	// Mark all descriptors in the cache as stale and in need of re-uploading.
	void UnbindAllValid(void);

	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	CommandContext &m_OwningContext;
	ID3D12DescriptorHeap *m_CurrentHeapPtr;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
	uint32_t m_DescriptorSize;
	uint32_t m_CurrentOffset;
	DescriptorHandle m_FirstDescriptor;
	std::vector<ID3D12DescriptorHeap *> m_RetiredHeaps;

	std::unique_ptr<DescriptorHandleCache> m_GraphicsHandleCache;
	std::unique_ptr<DescriptorHandleCache> m_ComputeHandleCache;
};

} // namespace D3D12Graphics
