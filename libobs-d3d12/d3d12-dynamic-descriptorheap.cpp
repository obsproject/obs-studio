#include <d3d12-deviceinstance.hpp>

namespace D3D12Graphics {

DescriptorHandle::DescriptorHandle()
{
	m_CpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_GpuHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

DescriptorHandle::DescriptorHandle(D3D12_CPU_DESCRIPTOR_HANDLE CpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE GpuHandle)
	: m_CpuHandle(CpuHandle),
	  m_GpuHandle(GpuHandle)
{
}

DescriptorHandleCache::DescriptorHandleCache(D3D12DeviceInstance *DeviceInstance) : m_DeviceInstance(DeviceInstance)
{
	ClearCache();
}

void DescriptorHandleCache::ClearCache()
{
	m_RootDescriptorTablesBitMap = 0;
	m_StaleRootParamsBitMap = 0;
	m_MaxCachedDescriptors = 0;
}

uint32_t DescriptorHandleCache::ComputeStagedSize()
{
	// Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
	uint32_t NeededSpace = 0;
	uint32_t RootIndex;
	uint32_t StaleParams = m_StaleRootParamsBitMap;
	while (_BitScanForward((unsigned long *)&RootIndex, StaleParams)) {
		StaleParams ^= (1 << RootIndex);

		uint32_t MaxSetHandle;
		if (TRUE != _BitScanReverse((unsigned long *)&MaxSetHandle,
					    m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap)) {
			throw HRError("Root entry marked as stale but has no stale descriptors");
		}

		NeededSpace += MaxSetHandle + 1;
	}
	return NeededSpace;
}

void DescriptorHandleCache::CopyAndBindStaleTables(
	D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t DescriptorSize, DescriptorHandle DestHandleStart,
	ID3D12GraphicsCommandList *CmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint32_t StaleParamCount = 0;
	uint32_t TableSize[kMaxNumDescriptorTables];
	uint32_t RootIndices[kMaxNumDescriptorTables];
	uint32_t NeededSpace = 0;
	uint32_t RootIndex;

	// Sum the maximum assigned offsets of stale descriptor tables to determine total needed space.
	uint32_t StaleParams = m_StaleRootParamsBitMap;
	while (_BitScanForward((unsigned long *)&RootIndex, StaleParams)) {
		RootIndices[StaleParamCount] = RootIndex;
		StaleParams ^= (1 << RootIndex);

		uint32_t MaxSetHandle;
		if (TRUE != _BitScanReverse((unsigned long *)&MaxSetHandle,
					    m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap)) {
			throw HRError("Root entry marked as stale but has no stale descriptors");
		}

		NeededSpace += MaxSetHandle + 1;
		TableSize[StaleParamCount] = MaxSetHandle + 1;

		++StaleParamCount;
	}
	if (StaleParamCount > kMaxNumDescriptorTables) {
		throw HRError("We're only equipped to handle so many descriptor tables");
	}

	m_StaleRootParamsBitMap = 0;

	UINT NumDestDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxNumDescriptorTables];
	UINT pDestDescriptorRangeSizes[kMaxNumDescriptorTables];

	UINT NumSrcDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxNumDescriptorTables];
	UINT pSrcDescriptorRangeSizes[kMaxNumDescriptorTables];

	for (uint32_t i = 0; i < StaleParamCount; ++i) {
		RootIndex = RootIndices[i];
		(CmdList->*SetFunc)(RootIndex, DestHandleStart);

		DescriptorTableCache &RootDescTable = m_RootDescriptorTable[RootIndex];

		D3D12_CPU_DESCRIPTOR_HANDLE *SrcHandles = RootDescTable.TableStart;
		uint64_t SetHandles = (uint64_t)RootDescTable.AssignedHandlesBitMap;
		D3D12_CPU_DESCRIPTOR_HANDLE CurDest = DestHandleStart;
		DestHandleStart += TableSize[i] * DescriptorSize;

		unsigned long SkipCount;
		while (_BitScanForward64(&SkipCount, SetHandles)) {
			// Skip over unset descriptor handles
			SetHandles >>= SkipCount;
			SrcHandles += SkipCount;
			CurDest.ptr += SkipCount * DescriptorSize;

			unsigned long DescriptorCount;
			_BitScanForward64(&DescriptorCount, ~SetHandles);
			SetHandles >>= DescriptorCount;

			// If we run out of temp room, copy what we've got so far
			if (NumSrcDescriptorRanges + DescriptorCount > kMaxNumDescriptorTables) {
				m_DeviceInstance->GetDevice()->CopyDescriptors(
					NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
					NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes,
					Type);

				NumSrcDescriptorRanges = 0;
				NumDestDescriptorRanges = 0;
			}

			// Setup destination range
			pDestDescriptorRangeStarts[NumDestDescriptorRanges] = CurDest;
			pDestDescriptorRangeSizes[NumDestDescriptorRanges] = DescriptorCount;
			++NumDestDescriptorRanges;

			// Setup source ranges (one descriptor each because we don't assume they are contiguous)
			for (uint32_t j = 0; j < DescriptorCount; ++j) {
				pSrcDescriptorRangeStarts[NumSrcDescriptorRanges] = SrcHandles[j];
				pSrcDescriptorRangeSizes[NumSrcDescriptorRanges] = 1;
				++NumSrcDescriptorRanges;
			}

			// Move the destination pointer forward by the number of descriptors we will copy
			SrcHandles += DescriptorCount;
			CurDest.ptr += DescriptorCount * DescriptorSize;
		}
	}

	m_DeviceInstance->GetDevice()->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts,
						       pDestDescriptorRangeSizes, NumSrcDescriptorRanges,
						       pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, Type);
}

void DescriptorHandleCache::UnbindAllValid()
{
	m_StaleRootParamsBitMap = 0;

	unsigned long TableParams = m_RootDescriptorTablesBitMap;
	unsigned long RootIndex;
	while (_BitScanForward(&RootIndex, TableParams)) {
		TableParams ^= (1 << RootIndex);
		if (m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap != 0)
			m_StaleRootParamsBitMap |= (1 << RootIndex);
	}
}

void DescriptorHandleCache::StageDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
						   const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	if (((1 << RootIndex) & m_RootDescriptorTablesBitMap) == 0) {
		throw HRError("Root parameter is not a CBV_SRV_UAV descriptor table");
	}

	if (Offset + NumHandles > m_RootDescriptorTable[RootIndex].TableSize) {
		throw HRError("Attempting to stage more descriptors than exist in the descriptor table");
	}

	DescriptorTableCache &TableCache = m_RootDescriptorTable[RootIndex];
	D3D12_CPU_DESCRIPTOR_HANDLE *CopyDest = TableCache.TableStart + Offset;
	for (UINT i = 0; i < NumHandles; ++i)
		CopyDest[i] = Handles[i];
	TableCache.AssignedHandlesBitMap |= ((1 << NumHandles) - 1) << Offset;
	m_StaleRootParamsBitMap |= (1 << RootIndex);
}

void DescriptorHandleCache::ParseRootSignature(D3D12_DESCRIPTOR_HEAP_TYPE Type, const RootSignature &RootSig)
{
	UINT CurrentOffset = 0;

	if (RootSig.m_NumParameters > kMaxNumDescriptorTables) {
		throw HRError("Maybe we need to support something greater");
	}

	m_StaleRootParamsBitMap = 0;
	m_RootDescriptorTablesBitMap = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? RootSig.m_SamplerTableBitMap
										   : RootSig.m_DescriptorTableBitMap);

	unsigned long TableParams = m_RootDescriptorTablesBitMap;
	unsigned long RootIndex;
	while (_BitScanForward(&RootIndex, TableParams)) {
		TableParams ^= (1 << RootIndex);

		UINT TableSize = RootSig.m_DescriptorTableSize[RootIndex];

		if (TableSize == 0) {
			throw HRError("Descriptor table has zero size");
		}

		DescriptorTableCache &RootDescriptorTable = m_RootDescriptorTable[RootIndex];
		RootDescriptorTable.AssignedHandlesBitMap = 0;
		RootDescriptorTable.TableStart = m_HandleCache + CurrentOffset;
		RootDescriptorTable.TableSize = TableSize;

		CurrentOffset += TableSize;
	}

	m_MaxCachedDescriptors = CurrentOffset;

	if (m_MaxCachedDescriptors > kMaxNumDescriptors) {
		throw HRError("Exceeded user-supplied maximum cache size");
	}
}

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12DeviceInstance *DeviceInstance, CommandContext &OwningContext,
					     D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
	: m_DeviceInstance(DeviceInstance),
	  m_OwningContext(OwningContext),
	  m_DescriptorType(HeapType),
	  m_GraphicsHandleCache(std::make_unique<DescriptorHandleCache>(DeviceInstance)),
	  m_ComputeHandleCache(std::make_unique<DescriptorHandleCache>(DeviceInstance))
{
	m_CurrentHeapPtr = nullptr;
	m_CurrentOffset = 0;
	m_DescriptorSize = m_DeviceInstance->GetDevice()->GetDescriptorHandleIncrementSize(HeapType);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap() {}

void DynamicDescriptorHeap::CleanupUsedHeaps(uint64_t fenceValue)
{
	RetireCurrentHeap();
	RetireUsedHeaps(fenceValue);
	m_GraphicsHandleCache->ClearCache();
	m_ComputeHandleCache->ClearCache();
}

void DynamicDescriptorHeap::SetGraphicsDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
							 const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_GraphicsHandleCache->StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
}

void DynamicDescriptorHeap::SetComputeDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
							const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_ComputeHandleCache->StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	if (!HasSpace(1)) {
		RetireCurrentHeap();
		UnbindAllValid();
	}

	m_OwningContext.SetDescriptorHeap(m_DescriptorType, GetHeapPointer());

	DescriptorHandle DestHandle = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
	m_CurrentOffset += 1;

	m_DeviceInstance->GetDevice()->CopyDescriptorsSimple(1, DestHandle, Handle, m_DescriptorType);

	return DestHandle;
}
void DynamicDescriptorHeap::ParseGraphicsRootSignature(const RootSignature &RootSig)
{
	m_GraphicsHandleCache->ParseRootSignature(m_DescriptorType, RootSig);
}

void DynamicDescriptorHeap::ParseComputeRootSignature(const RootSignature &RootSig)
{
	m_ComputeHandleCache->ParseRootSignature(m_DescriptorType, RootSig);
}

void DynamicDescriptorHeap::CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList *CmdList)
{
	if (m_GraphicsHandleCache->m_StaleRootParamsBitMap != 0)
		CopyAndBindStagedTables(*m_GraphicsHandleCache, CmdList,
					&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList *CmdList)
{
	if (m_ComputeHandleCache->m_StaleRootParamsBitMap != 0)
		CopyAndBindStagedTables(*m_ComputeHandleCache, CmdList,
					&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

bool DynamicDescriptorHeap::HasSpace(uint32_t Count)
{
	return (m_CurrentHeapPtr != nullptr && m_CurrentOffset + Count <= kMaxNumDescriptors);
}

void DynamicDescriptorHeap::RetireCurrentHeap(void)
{
	if (m_CurrentOffset == 0) {
		return;
	}

	if (m_CurrentHeapPtr == nullptr) {
		throw HRError("No current heap to retire");
	}
	m_RetiredHeaps.push_back(m_CurrentHeapPtr);
	m_CurrentHeapPtr = nullptr;
	m_CurrentOffset = 0;
}

void DynamicDescriptorHeap::RetireUsedHeaps(uint64_t fenceValue)
{
	m_DeviceInstance->DiscardDynamicDescriptorHeaps(m_DescriptorType, fenceValue, m_RetiredHeaps);
	m_RetiredHeaps.clear();
}

ID3D12DescriptorHeap *DynamicDescriptorHeap::GetHeapPointer()
{
	if (m_CurrentHeapPtr == nullptr) {
		m_CurrentHeapPtr = m_DeviceInstance->RequestDynamicDescriptorHeap(m_DescriptorType);
		m_FirstDescriptor = DescriptorHandle(m_CurrentHeapPtr->GetCPUDescriptorHandleForHeapStart(),
						     m_CurrentHeapPtr->GetGPUDescriptorHandleForHeapStart());
	}

	return m_CurrentHeapPtr;
}

DescriptorHandle DynamicDescriptorHeap::Allocate(UINT Count)
{
	DescriptorHandle ret = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
	m_CurrentOffset += Count;
	return ret;
}

void DynamicDescriptorHeap::CopyAndBindStagedTables(
	DescriptorHandleCache &HandleCache, ID3D12GraphicsCommandList *CmdList,
	void (STDMETHODCALLTYPE ID3D12GraphicsCommandList::*SetFunc)(UINT, D3D12_GPU_DESCRIPTOR_HANDLE))
{
	uint32_t NeededSize = HandleCache.ComputeStagedSize();
	if (!HasSpace(NeededSize)) {
		RetireCurrentHeap();
		UnbindAllValid();
		NeededSize = HandleCache.ComputeStagedSize();
	}

	// This can trigger the creation of a new heap
	m_OwningContext.SetDescriptorHeap(m_DescriptorType, GetHeapPointer());
	HandleCache.CopyAndBindStaleTables(m_DescriptorType, m_DescriptorSize, Allocate(NeededSize), CmdList, SetFunc);
}

void DynamicDescriptorHeap::UnbindAllValid(void)
{
	m_GraphicsHandleCache->UnbindAllValid();
	m_ComputeHandleCache->UnbindAllValid();
}
} // namespace D3D12Graphics
