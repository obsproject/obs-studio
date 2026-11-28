#pragma once

#include <d3d12-common.hpp>

namespace D3D12Graphics {

class D3D12DeviceInstance;

typedef struct DescriptorHandleNode {
	UINT64 index = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	void *next = (void *)D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
} DescriptorHandleNode;

typedef struct D3D12_CPU_DESCRIPTOR_HANDLE_NODE {
	UINT64 index;
	D3D12_CPU_DESCRIPTOR_HANDLE handle = {D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN};
} D3D12_CPU_DESCRIPTOR_HANDLE_NODE;

class DescriptorAllocator {
public:
	DescriptorAllocator(D3D12DeviceInstance *DeviceInstance, D3D12_DESCRIPTOR_HEAP_TYPE Type);
	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);

protected:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	ID3D12DescriptorHeap *m_CurrentHeap = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentHandle;
	uint32_t m_DescriptorSize = 0;
	uint32_t m_RemainingFreeHandles;

	// TODO
	UINT64 GetAvailableIndex();
	void FreeIndex(UINT64 index);
	DescriptorHandleNode *m_DescriptorPoolHead = nullptr;
	DescriptorHandleNode m_DescriptorPoolNodes[kMaxNumDescriptors];
};

} // namespace D3D12Graphics
