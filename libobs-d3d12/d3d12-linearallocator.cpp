#include <d3d12-deviceinstance.hpp>

namespace D3D12Graphics {

LinearAllocatorPageManager::LinearAllocatorPageManager(D3D12DeviceInstance *DeviceInstance, LinearAllocatorType Type)
	: m_DeviceInstance(DeviceInstance),
	  m_AllocationType(Type)
{
}

LinearAllocationPage *LinearAllocatorPageManager::RequestPage(void)
{
	while (!m_RetiredPages.empty() &&
	       m_DeviceInstance->GetCommandManager().IsFenceComplete(m_RetiredPages.front().first)) {
		m_AvailablePages.push(m_RetiredPages.front().second);
		m_RetiredPages.pop();
	}

	LinearAllocationPage *PagePtr = nullptr;

	if (!m_AvailablePages.empty()) {
		PagePtr = m_AvailablePages.front();
		m_AvailablePages.pop();
	} else {
		PagePtr = CreateNewPage();
		m_PagePool.emplace_back(PagePtr);
	}

	return PagePtr;
}

LinearAllocationPage *LinearAllocatorPageManager::CreateNewPage(size_t PageSize)
{
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC ResourceDesc;
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Alignment = 0;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.MipLevels = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	D3D12_RESOURCE_STATES DefaultUsage;

	if (m_AllocationType == kGpuExclusive) {
		HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
		ResourceDesc.Width = PageSize == 0 ? kGpuAllocatorPageSize : PageSize;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		DefaultUsage = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	} else {
		HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
		ResourceDesc.Width = PageSize == 0 ? kCpuAllocatorPageSize : PageSize;
		ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		DefaultUsage = D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	ID3D12Resource *pBuffer;
	HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommittedResource(
		&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, DefaultUsage, nullptr, IID_PPV_ARGS(&pBuffer));
	if (FAILED(hr)) {
		throw HRError("LinearAllocatorPageManager: Failed to create a new allocation page.");
	}
	pBuffer->SetName(L"LinearAllocator Page");

	return new LinearAllocationPage(pBuffer, DefaultUsage);
}

void LinearAllocatorPageManager::DiscardPages(uint64_t FenceValue, const std::vector<LinearAllocationPage *> &UsedPages)
{
	for (auto iter = UsedPages.begin(); iter != UsedPages.end(); ++iter)
		m_RetiredPages.push(std::make_pair(FenceValue, *iter));
}

void LinearAllocatorPageManager::FreeLargePages(uint64_t FenceValue,
						const std::vector<LinearAllocationPage *> &LargePages)
{
	while (!m_DeletionQueue.empty() &&
	       m_DeviceInstance->GetCommandManager().IsFenceComplete(m_DeletionQueue.front().first)) {
		delete m_DeletionQueue.front().second;
		m_DeletionQueue.pop();
	}

	for (auto iter = LargePages.begin(); iter != LargePages.end(); ++iter) {
		(*iter)->Unmap();
		m_DeletionQueue.push(std::make_pair(FenceValue, *iter));
	}
}

void LinearAllocatorPageManager::Destroy(void)
{
	m_PagePool.clear();
}

LinearAllocator::LinearAllocator(D3D12DeviceInstance *DeviceInstance, LinearAllocatorType Type)
	: m_DeviceInstance(DeviceInstance),
	  m_AllocationType(Type),
	  m_PageSize(0),
	  m_CurOffset(~(size_t)0),
	  m_CurPage(nullptr)
{
	m_PageSize = (Type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
}

DynAlloc LinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
{
	const size_t AlignmentMask = Alignment - 1;

	// Assert that it's a power of two.
	if ((AlignmentMask & Alignment) != 0) {
		throw HRError("LinearAllocator: Alignment must be a power of two.");
	}

	// Align the allocation
	const size_t AlignedSize = AlignUpWithMask(SizeInBytes, AlignmentMask);

	if (AlignedSize > m_PageSize)
		return AllocateLargePage(AlignedSize);

	m_CurOffset = AlignUp(m_CurOffset, Alignment);

	if (m_CurOffset + AlignedSize > m_PageSize) {
		if (m_CurPage == nullptr) {
			throw HRError("LinearAllocator: Current page is null when trying to allocate a new page.");
		}
		m_RetiredPages.push_back(m_CurPage);
		m_CurPage = nullptr;
	}

	if (m_CurPage == nullptr) {
		m_CurPage = m_DeviceInstance->GetPageManager(m_AllocationType)->RequestPage();
		m_CurOffset = 0;
	}

	DynAlloc ret(*m_CurPage, m_CurOffset, AlignedSize);
	ret.DataPtr = (uint8_t *)m_CurPage->m_CpuVirtualAddress + m_CurOffset;
	ret.GpuAddress = m_CurPage->m_GpuVirtualAddress + m_CurOffset;

	m_CurOffset += AlignedSize;

	return ret;
}

void LinearAllocator::CleanupUsedPages(uint64_t FenceID)
{
	if (m_CurPage != nullptr) {
		m_RetiredPages.push_back(m_CurPage);
		m_CurPage = nullptr;
		m_CurOffset = 0;
	}

	m_DeviceInstance->GetPageManager(m_AllocationType)->DiscardPages(FenceID, m_RetiredPages);
	m_RetiredPages.clear();

	m_DeviceInstance->GetPageManager(m_AllocationType)->FreeLargePages(FenceID, m_LargePageList);
	m_LargePageList.clear();
}

DynAlloc LinearAllocator::AllocateLargePage(size_t SizeInBytes)
{
	LinearAllocationPage *OneOff = m_DeviceInstance->GetPageManager(m_AllocationType)->CreateNewPage(SizeInBytes);
	m_LargePageList.push_back(OneOff);

	DynAlloc ret(*OneOff, 0, SizeInBytes);
	ret.DataPtr = OneOff->m_CpuVirtualAddress;
	ret.GpuAddress = OneOff->m_GpuVirtualAddress;

	return ret;
}

} // namespace D3D12Graphics
