#include <d3d12-deviceinstance.hpp>

namespace D3D12Graphics {

DescriptorAllocator::DescriptorAllocator(D3D12DeviceInstance *DeviceInstance, D3D12_DESCRIPTOR_HEAP_TYPE Type)
	: m_DeviceInstance(DeviceInstance),
	  m_Type(Type),
	  m_CurrentHeap(nullptr),
	  m_DescriptorSize(0),
	  m_RemainingFreeHandles(0),
	  m_DescriptorPoolHead(nullptr)
{
	m_CurrentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate(uint32_t Count)
{
	if (m_CurrentHeap == nullptr || m_RemainingFreeHandles < Count) {
		m_CurrentHeap = m_DeviceInstance->RequestCommonHeap(m_Type);
		m_CurrentHandle = m_CurrentHeap->GetCPUDescriptorHandleForHeapStart();
		m_RemainingFreeHandles = kMaxNumDescriptors;

		if (m_DescriptorSize == 0)
			m_DescriptorSize = m_DeviceInstance->GetDevice()->GetDescriptorHandleIncrementSize(m_Type);

		for (int32_t i = 0; i < kMaxNumDescriptors; ++i) {
			m_DescriptorPoolNodes[i].index = i;
			if (i != kMaxNumDescriptors - 1) {
				m_DescriptorPoolNodes[i].next = &m_DescriptorPoolNodes[i + 1];
			}
		}

		m_DescriptorPoolHead = &m_DescriptorPoolNodes[0];
	}

	if (m_RemainingFreeHandles <= 0) {
		throw HRError("DescriptorAllocator: No remaining free handles");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ret = m_CurrentHandle;
	m_CurrentHandle.ptr += Count * m_DescriptorSize;
	m_RemainingFreeHandles -= Count;
	return ret;
}

UINT64 DescriptorAllocator::GetAvailableIndex()
{
	if (m_DescriptorPoolHead) {
		SIZE_T index = m_DescriptorPoolHead->index;
		m_DescriptorPoolHead = (DescriptorHandleNode *)(m_DescriptorPoolHead->next);
		return index;
	} else {
		return D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}
}

void DescriptorAllocator::FreeIndex(UINT64 index)
{
	m_DescriptorPoolNodes[index].next = m_DescriptorPoolHead;
	m_DescriptorPoolHead = &m_DescriptorPoolNodes[index];
}

} // namespace D3D12Graphics
