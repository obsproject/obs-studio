#include "d3d12-command-context.hpp"

#include <util/base.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/util.hpp>

#include <map>
#include <thread>
#include <cinttypes>

#include <shellscalingapi.h>
#include <d3dkmthk.h>

static const uint32_t vendorID_Nvidia = 0x10DE;
static const uint32_t vendorID_AMD = 0x1002;
static const uint32_t vendorID_Intel = 0x8086;

namespace D3D12Graphics {

void RootSignature::InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC &NonStaticSamplerDesc,
				      D3D12_SHADER_VISIBILITY Visibility)
{
	if (!(m_NumInitializedStaticSamplers < m_NumSamplers)) {
		throw HRError("RootSignature: m_NumInitializedStaticSamplers < m_NumSamplers");
	}
	D3D12_STATIC_SAMPLER_DESC &StaticSamplerDesc = m_SamplerArray[m_NumInitializedStaticSamplers++];

	StaticSamplerDesc.Filter = NonStaticSamplerDesc.Filter;
	StaticSamplerDesc.AddressU = NonStaticSamplerDesc.AddressU;
	StaticSamplerDesc.AddressV = NonStaticSamplerDesc.AddressV;
	StaticSamplerDesc.AddressW = NonStaticSamplerDesc.AddressW;
	StaticSamplerDesc.MipLODBias = NonStaticSamplerDesc.MipLODBias;
	StaticSamplerDesc.MaxAnisotropy = NonStaticSamplerDesc.MaxAnisotropy;
	StaticSamplerDesc.ComparisonFunc = NonStaticSamplerDesc.ComparisonFunc;
	StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	StaticSamplerDesc.MinLOD = NonStaticSamplerDesc.MinLOD;
	StaticSamplerDesc.MaxLOD = NonStaticSamplerDesc.MaxLOD;
	StaticSamplerDesc.ShaderRegister = Register;
	StaticSamplerDesc.RegisterSpace = 0;
	StaticSamplerDesc.ShaderVisibility = Visibility;

	if (StaticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
	    StaticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
	    StaticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER) {
		bool checkedStaticSample =
			(NonStaticSamplerDesc.BorderColor[0] == 0.0f && NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
				 NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
				 NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
			 NonStaticSamplerDesc.BorderColor[0] == 0.0f && NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
				 NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
				 NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
			 NonStaticSamplerDesc.BorderColor[0] == 1.0f && NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
				 NonStaticSamplerDesc.BorderColor[2] == 1.0f &&
				 NonStaticSamplerDesc.BorderColor[3] == 1.0f);
		if (!checkedStaticSample) {
			throw HRError("Sampler border color does not match static sampler limitations");
		}

		if (NonStaticSamplerDesc.BorderColor[3] == 1.0f) {
			if (NonStaticSamplerDesc.BorderColor[0] == 1.0f)
				StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			else
				StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		} else
			StaticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	}
}

void RootSignature::Finalize(const std::wstring &name, D3D12_ROOT_SIGNATURE_FLAGS Flags)
{
	if (m_Finalized)
		return;

	if (m_NumInitializedStaticSamplers != m_NumSamplers) {
		throw HRError("RootSignature: m_NumInitializedStaticSamplers != m_NumSamplers");
	}

	D3D12_ROOT_SIGNATURE_DESC RootDesc;
	RootDesc.NumParameters = m_NumParameters;
	RootDesc.pParameters = (const D3D12_ROOT_PARAMETER *)m_ParamArray.get();
	RootDesc.NumStaticSamplers = m_NumSamplers;
	RootDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC *)m_SamplerArray.get();
	RootDesc.Flags = Flags;

	m_DescriptorTableBitMap = 0;
	m_SamplerTableBitMap = 0;

	size_t HashCode = Utility::HashState(&RootDesc.Flags);
	HashCode = Utility::HashState(RootDesc.pStaticSamplers, m_NumSamplers, HashCode);

	for (UINT Param = 0; Param < m_NumParameters; ++Param) {
		const D3D12_ROOT_PARAMETER &RootParam = RootDesc.pParameters[Param];
		m_DescriptorTableSize[Param] = 0;

		if (RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			if (RootParam.DescriptorTable.pDescriptorRanges == nullptr) {
				throw HRError("RootSignature: DescriptorTable with null pDescriptorRanges");
			}

			HashCode = Utility::HashState(RootParam.DescriptorTable.pDescriptorRanges,
						      RootParam.DescriptorTable.NumDescriptorRanges, HashCode);

			// We keep track of sampler descriptor tables separately from CBV_SRV_UAV descriptor tables
			if (RootParam.DescriptorTable.pDescriptorRanges->RangeType ==
			    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
				m_SamplerTableBitMap |= (1 << Param);
			else
				m_DescriptorTableBitMap |= (1 << Param);

			for (UINT TableRange = 0; TableRange < RootParam.DescriptorTable.NumDescriptorRanges;
			     ++TableRange)
				m_DescriptorTableSize[Param] +=
					RootParam.DescriptorTable.pDescriptorRanges[TableRange].NumDescriptors;
		} else
			HashCode = Utility::HashState(&RootParam, 1, HashCode);
	}

	ComPtr<ID3D12RootSignature> RSRef = nullptr;
	bool firstCompile = false;
	{
		auto iter = m_DeviceInstance->GetRootSignatureHashMap().find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_DeviceInstance->GetRootSignatureHashMap().end()) {
			RSRef = m_DeviceInstance->GetRootSignatureHashMap()[HashCode];
			firstCompile = true;
		} else
			RSRef = iter->second;
	}
	if (firstCompile) {
		ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

		HRESULT hr =
			(D3D12SerializeRootSignature(&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob));
		if (FAILED(hr)) {
			throw HRError("D3D12SerializeRootSignature failed", hr);
		}
		hr = (m_DeviceInstance->GetDevice()->CreateRootSignature(
			1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_Signature)));

		if (FAILED(hr)) {
			auto DeviceRemoveReason = m_DeviceInstance->GetDevice()->GetDeviceRemovedReason();
			throw HRError("CreateRootSignature failed", hr);
		}
		m_Signature->SetName(name.c_str());

		m_DeviceInstance->GetRootSignatureHashMap()[HashCode].Set(m_Signature);
	} else {
		while (RSRef == nullptr)
			std::this_thread::yield();
		m_Signature = RSRef;
	}

	m_Finalized = TRUE;
}

ID3D12RootSignature *RootSignature::GetSignature() const
{
	return m_Signature;
}

ID3D12CommandSignature *CommandSignature::GetSignature() const
{
	return m_Signature.Get();
}

void CommandSignature::Finalize(const RootSignature *RootSignature)
{
	if (m_Finalized)
		return;

	UINT ByteStride = 0;
	bool RequiresRootSignature = false;

	for (UINT i = 0; i < m_NumParameters; ++i) {
		switch (m_ParamArray[i].GetDesc().Type) {
		case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW:
			ByteStride += sizeof(D3D12_DRAW_ARGUMENTS);
			break;
		case D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED:
			ByteStride += sizeof(D3D12_DRAW_INDEXED_ARGUMENTS);
			break;
		case D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH:
			ByteStride += sizeof(D3D12_DISPATCH_ARGUMENTS);
			break;
		case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
			ByteStride += m_ParamArray[i].GetDesc().Constant.Num32BitValuesToSet * 4;
			RequiresRootSignature = true;
			break;
		case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
			ByteStride += sizeof(D3D12_VERTEX_BUFFER_VIEW);
			break;
		case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
			ByteStride += sizeof(D3D12_INDEX_BUFFER_VIEW);
			break;
		case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
		case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
		case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW:
			ByteStride += 8;
			RequiresRootSignature = true;
			break;
		}
	}

	D3D12_COMMAND_SIGNATURE_DESC CommandSignatureDesc;
	CommandSignatureDesc.ByteStride = ByteStride;
	CommandSignatureDesc.NumArgumentDescs = m_NumParameters;
	CommandSignatureDesc.pArgumentDescs = (const D3D12_INDIRECT_ARGUMENT_DESC *)m_ParamArray.get();
	CommandSignatureDesc.NodeMask = 1;

	ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

	ID3D12RootSignature *pRootSig = RootSignature ? RootSignature->GetSignature() : nullptr;
	if (!RequiresRootSignature) {
		pRootSig = nullptr;
	}

	HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommandSignature(&CommandSignatureDesc, pRootSig,
									   IID_PPV_ARGS(&m_Signature));
	if (FAILED(hr)) {
		throw HRError("CreateCommandSignature failed", hr);
	}
	m_Signature->SetName(L"CommandSignature");

	m_Finalized = TRUE;
}

DescriptorAllocator::DescriptorAllocator(D3D12DeviceInstance *DeviceInstance, D3D12_DESCRIPTOR_HEAP_TYPE Type)
	: m_DeviceInstance(DeviceInstance),
	  m_Type(Type),
	  m_CurrentHeap(nullptr),
	  m_DescriptorSize(0),
	  m_RemainingFreeHandles(0)
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

ContextManager::ContextManager(D3D12DeviceInstance *DeviceInstance) : m_DeviceInstance(DeviceInstance) {}

CommandContext *ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE Type)
{
	auto &AvailableContexts = m_AvailableContexts[Type];

	CommandContext *ret = nullptr;
	if (AvailableContexts.empty()) {
		ret = new CommandContext(m_DeviceInstance, Type);
		m_ContextPool[Type].emplace_back(ret);
		ret->Initialize();
	} else {
		ret = AvailableContexts.front();
		AvailableContexts.pop();
		ret->Reset();
	}

	return ret;
}

void ContextManager::FreeContext(CommandContext *UsedContext)
{
	if (UsedContext == nullptr) {
		throw HRError("ContextManager: FreeContext called with null UsedContext");
	}
	m_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

void ContextManager::DestroyAllContexts()
{
	for (uint32_t i = 0; i < 4; ++i)
		m_ContextPool[i].clear();
}

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

	Size = Math::AlignUp(Size, 16);

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
		hr = m_DeviceInstance->GetDevice()->GetDeviceRemovedReason();
		throw HRError("ReadbackBuffer: CreateCommittedResource failed", hr);
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
	const size_t AlignedSize = Math::AlignUpWithMask(SizeInBytes, AlignmentMask);

	if (AlignedSize > m_PageSize)
		return AllocateLargePage(AlignedSize);

	m_CurOffset = Math::AlignUp(m_CurOffset, Alignment);

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

GraphicsPSO::GraphicsPSO(D3D12DeviceInstance *DeviceInstance, const wchar_t *Name) : PSO(DeviceInstance, Name)
{
	ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
	m_PSODesc.NodeMask = 1;
	m_PSODesc.SampleMask = 0xFFFFFFFFu;
	m_PSODesc.SampleDesc.Count = 1;
	m_PSODesc.InputLayout.NumElements = 0;
}

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC &BlendDesc)
{
	m_PSODesc.BlendState = BlendDesc;
}

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC &RasterizerDesc)
{
	m_PSODesc.RasterizerState = RasterizerDesc;
}

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC &DepthStencilDesc)
{
	m_PSODesc.DepthStencilState = DepthStencilDesc;
}

void GraphicsPSO::SetSampleMask(UINT SampleMask)
{
	m_PSODesc.SampleMask = SampleMask;
}

void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
	if (TopologyType == D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED) {
		throw HRError("GraphicsPSO: Can't draw with undefined topology.");
	}
	m_PSODesc.PrimitiveTopologyType = TopologyType;
}

void GraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
	SetRenderTargetFormats(0, nullptr, DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
	SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT *RTVFormats, DXGI_FORMAT DSVFormat,
					 UINT MsaaCount, UINT MsaaQuality)
{
	if (!(NumRTVs == 0 || RTVFormats != nullptr)) {
		throw HRError("GraphicsPSO: RTVFormats pointer is null with non-zero NumRTVs.");
	}
	for (UINT i = 0; i < NumRTVs; ++i) {
		if (RTVFormats[i] == DXGI_FORMAT_UNKNOWN) {
			throw HRError("GraphicsPSO: RTVFormats contains an UNKNOWN format.");
		}
		m_PSODesc.RTVFormats[i] = RTVFormats[i];
	}
	for (UINT i = NumRTVs; i < m_PSODesc.NumRenderTargets; ++i)
		m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
	m_PSODesc.NumRenderTargets = NumRTVs;
	m_PSODesc.DSVFormat = DSVFormat;
	m_PSODesc.SampleDesc.Count = MsaaCount;
	m_PSODesc.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs)
{
	m_PSODesc.InputLayout.NumElements = NumElements;

	if (NumElements > 0) {
		D3D12_INPUT_ELEMENT_DESC *NewElements =
			(D3D12_INPUT_ELEMENT_DESC *)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
		memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
		m_InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC *)NewElements);
	} else
		m_InputLayouts = nullptr;
}

void GraphicsPSO::SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps)
{
	m_PSODesc.IBStripCutValue = IBProps;
}

void GraphicsPSO::SetVertexShader(const void *Binary, size_t Size)
{
	m_PSODesc.VS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetPixelShader(const void *Binary, size_t Size)
{
	m_PSODesc.PS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetGeometryShader(const void *Binary, size_t Size)
{
	m_PSODesc.GS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetHullShader(const void *Binary, size_t Size)
{
	m_PSODesc.HS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetDomainShader(const void *Binary, size_t Size)
{
	m_PSODesc.DS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void GraphicsPSO::SetVertexShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.VS = Binary;
}

void GraphicsPSO::SetPixelShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.PS = Binary;
}

void GraphicsPSO::SetGeometryShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.GS = Binary;
}

void GraphicsPSO::SetHullShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.HS = Binary;
}

void GraphicsPSO::SetDomainShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.DS = Binary;
}

void GraphicsPSO::Finalize()
{
	m_PSODesc.pRootSignature = m_RootSignature->GetSignature();

	m_PSODesc.InputLayout.pInputElementDescs = nullptr;
	size_t HashCode = Utility::HashState(&m_PSODesc);
	HashCode = Utility::HashState(m_InputLayouts.get(), m_PSODesc.InputLayout.NumElements, HashCode);
	m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.get();

	ComPtr<ID3D12PipelineState> PSORef = nullptr;
	bool firstCompile = false;
	{
		auto iter = m_DeviceInstance->GetGraphicsPSOHashMap().find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_DeviceInstance->GetGraphicsPSOHashMap().end()) {
			firstCompile = true;
			PSORef = m_DeviceInstance->GetGraphicsPSOHashMap()[HashCode];
		} else
			PSORef = iter->second;
	}
	if (firstCompile) {
		HRESULT hr =
			m_DeviceInstance->GetDevice()->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO));
		if (FAILED(hr)) {
			throw HRError("GraphicsPSO: Failed to create graphics pipeline state object.");
		}
		m_DeviceInstance->GetGraphicsPSOHashMap()[HashCode].Set(m_PSO);
		m_PSO->SetName(m_Name);
	} else {
		while (PSORef == nullptr)
			std::this_thread::yield();
		m_PSO = PSORef;
	}
}

ComputePSO::ComputePSO(D3D12DeviceInstance *DeviceInstance, const wchar_t *Name) : PSO(DeviceInstance, Name)
{
	ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
	m_PSODesc.NodeMask = 1;
}

void ComputePSO::SetComputeShader(const void *Binary, size_t Size)
{
	m_PSODesc.CS = CD3DX12_SHADER_BYTECODE(const_cast<void *>(Binary), Size);
}

void ComputePSO::SetComputeShader(const D3D12_SHADER_BYTECODE &Binary)
{
	m_PSODesc.CS = Binary;
}

void ComputePSO::Finalize()
{
	m_PSODesc.pRootSignature = m_RootSignature->GetSignature();
	size_t HashCode = Utility::HashState(&m_PSODesc);

	ComPtr<ID3D12PipelineState> PSORef = nullptr;
	bool firstCompile = false;
	{
		auto iter = m_DeviceInstance->GetComputePSOHashMap().find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == m_DeviceInstance->GetComputePSOHashMap().end()) {
			firstCompile = true;
			PSORef = m_DeviceInstance->GetComputePSOHashMap()[HashCode];
		} else
			PSORef = iter->second;
	}

	if (firstCompile) {
		HRESULT hr =
			m_DeviceInstance->GetDevice()->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO));
		if (FAILED(hr)) {
			throw HRError("ComputePSO: Failed to create compute pipeline state object.");
		}
		m_DeviceInstance->GetComputePSOHashMap()[HashCode].Set(m_PSO);
		m_PSO->SetName(m_Name);
	} else {
		while (PSORef == nullptr)
			std::this_thread::yield();
		m_PSO = PSORef;
	}
}

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type)
	: m_cCommandListType(Type),
	  m_DeviceInstance(nullptr)
{
}

CommandAllocatorPool::~CommandAllocatorPool()
{
	Shutdown();
}

void CommandAllocatorPool::Create(D3D12DeviceInstance *DeviceInstance)
{
	m_DeviceInstance = DeviceInstance;
}

void CommandAllocatorPool::Shutdown()
{
	for (size_t i = 0; i < m_AllocatorPool.size(); ++i)
		m_AllocatorPool[i]->Release();

	m_AllocatorPool.clear();
}

ID3D12CommandAllocator *CommandAllocatorPool::RequestAllocator(uint64_t CompletedFenceValue)
{
	ID3D12CommandAllocator *pAllocator = nullptr;

	if (!m_ReadyAllocators.empty()) {
		std::pair<uint64_t, ID3D12CommandAllocator *> &AllocatorPair = m_ReadyAllocators.front();

		if (AllocatorPair.first <= CompletedFenceValue) {
			pAllocator = AllocatorPair.second;
			HRESULT hr = pAllocator->Reset();
			if (FAILED(hr)) {
				throw HRError("CommandAllocatorPool: Failed to reset command allocator for reuse.", hr);
			}
			m_ReadyAllocators.pop();
		}
	}

	// If no allocator's were ready to be reused, create a new one
	if (pAllocator == nullptr) {
		HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommandAllocator(m_cCommandListType,
										   IID_PPV_ARGS(&pAllocator));
		if (FAILED(hr)) {
			throw HRError("CommandAllocatorPool: Failed to create a new command allocator.", hr);
		}

		wchar_t AllocatorName[32];
		swprintf(AllocatorName, 32, L"CommandAllocator %zu", m_AllocatorPool.size());
		pAllocator->SetName(AllocatorName);
		m_AllocatorPool.push_back(pAllocator);
	}

	return pAllocator;
}

void CommandAllocatorPool::DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator *Allocator)
{
	m_ReadyAllocators.push(std::make_pair(FenceValue, Allocator));
}

inline size_t CommandAllocatorPool::Size()
{
	return m_AllocatorPool.size();
}

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE Type)
	: m_Type(Type),
	  m_CommandQueue(nullptr),
	  m_pFence(nullptr),
	  m_NextFenceValue((uint64_t)Type << 56 | 1),
	  m_LastCompletedFenceValue((uint64_t)Type << 56),
	  m_AllocatorPool(std::make_unique<CommandAllocatorPool>(m_Type))
{
}

CommandQueue::~CommandQueue()
{
	Shutdown();
}

void CommandQueue::Create(D3D12DeviceInstance *DeviceInstance)
{
	m_DeviceInstance = DeviceInstance;
	ID3D12Device *pDevice = m_DeviceInstance->GetDevice();

	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = m_Type;
	QueueDesc.NodeMask = 1;
	HRESULT hr = pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_CommandQueue));
	if (FAILED(hr)) {
		throw HRError("CommandQueue: Failed to create command queue.", hr);
	}

	m_CommandQueue->SetName(L"CommandListManager::m_CommandQueue");

	hr = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence));
	if (FAILED(hr)) {
		throw HRError("CommandQueue: Failed to create fence for command queue.", hr);
	}

	m_pFence->SetName(L"CommandListManager::m_pFence");
	m_pFence->Signal((uint64_t)m_Type << 56);

	m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
	if (m_FenceEventHandle == NULL) {
		throw HRError("CommandQueue: Failed to create fence event handle.");
	}

	m_AllocatorPool->Create(m_DeviceInstance);
}

void CommandQueue::Shutdown()
{
	if (m_CommandQueue == nullptr)
		return;

	m_AllocatorPool->Shutdown();

	CloseHandle(m_FenceEventHandle);

	m_pFence->Release();
	m_pFence = nullptr;

	m_CommandQueue->Release();
	m_CommandQueue = nullptr;
}

inline bool CommandQueue::IsReady()
{
	return m_CommandQueue != nullptr;
}

uint64_t CommandQueue::IncrementFence(void)
{
	m_CommandQueue->Signal(m_pFence, m_NextFenceValue);
	return m_NextFenceValue++;
}

bool CommandQueue::IsFenceComplete(uint64_t FenceValue)
{ // Avoid querying the fence value by testing against the last one seen.
	// The max() is to protect against an unlikely race condition that could cause the last
	// completed fence value to regress.
	if (FenceValue > m_LastCompletedFenceValue)
		m_LastCompletedFenceValue = std::max(m_LastCompletedFenceValue, m_pFence->GetCompletedValue());

	return FenceValue <= m_LastCompletedFenceValue;
}

void CommandQueue::StallForFence(uint64_t FenceValue)
{
	CommandQueue &Producer =
		m_DeviceInstance->GetCommandManager().GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	m_CommandQueue->Wait(Producer.m_pFence, FenceValue);
}

void CommandQueue::StallForProducer(CommandQueue &Producer)
{
	m_CommandQueue->Wait(Producer.m_pFence, Producer.m_NextFenceValue - 1);
}

void CommandQueue::WaitForFence(uint64_t FenceValue)
{
	if (IsFenceComplete(FenceValue))
		return;

	// TODO:  Think about how this might affect a multi-threaded situation.  Suppose thread A
	// wants to wait for fence 100, then thread B comes along and wants to wait for 99.  If
	// the fence can only have one event set on completion, then thread B has to wait for
	// 100 before it knows 99 is ready.  Maybe insert sequential events?
	{
		m_pFence->SetEventOnCompletion(FenceValue, m_FenceEventHandle);
		WaitForSingleObject(m_FenceEventHandle, INFINITE);
		m_LastCompletedFenceValue = FenceValue;
	}
}

void CommandQueue::WaitForIdle(void)
{
	WaitForFence(IncrementFence());
}

ID3D12CommandQueue *CommandQueue::GetCommandQueue()
{
	return m_CommandQueue;
}

uint64_t CommandQueue::GetNextFenceValue()
{
	return m_NextFenceValue;
}

uint64_t CommandQueue::ExecuteCommandList(ID3D12CommandList *List)
{
	HRESULT hr = ((ID3D12GraphicsCommandList *)List)->Close();
	if (FAILED(hr)) {
		auto removeReason = m_DeviceInstance->GetDevice()->GetDeviceRemovedReason();
		throw HRError("CommandQueue: Failed to close command list before execution.", removeReason);
	}

	// Kickoff the command list
	m_CommandQueue->ExecuteCommandLists(1, &List);

	// Signal the next fence value (with the GPU)
	m_CommandQueue->Signal(m_pFence, m_NextFenceValue);

	// And increment the fence value.
	return m_NextFenceValue++;
}

ID3D12CommandAllocator *CommandQueue::RequestAllocator(void)
{
	uint64_t CompletedFence = m_pFence->GetCompletedValue();

	return m_AllocatorPool->RequestAllocator(CompletedFence);
}

void CommandQueue::DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator *Allocator)
{
	m_AllocatorPool->DiscardAllocator(FenceValueForReset, Allocator);
}

CommandListManager::CommandListManager() : m_DeviceInstance(nullptr) {}

CommandListManager::~CommandListManager()
{
	Shutdown();
}

void CommandListManager::Create(D3D12DeviceInstance *DeviceInstance)
{
	m_DeviceInstance = DeviceInstance;
	ID3D12Device *pDevice = m_DeviceInstance->GetDevice();
	m_GraphicsQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_GraphicsQueue->Create(m_DeviceInstance);

	m_ComputeQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_ComputeQueue->Create(m_DeviceInstance);

	m_CopyQueue = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE::D3D12_COMMAND_LIST_TYPE_COPY);
	m_CopyQueue->Create(m_DeviceInstance);
}

void CommandListManager::Shutdown()
{
	m_GraphicsQueue->Shutdown();
	m_ComputeQueue->Shutdown();
	m_CopyQueue->Shutdown();
}

CommandQueue &CommandListManager::GetGraphicsQueue(void)
{
	return *m_GraphicsQueue;
}

CommandQueue &CommandListManager::GetComputeQueue(void)
{
	return *m_ComputeQueue;
}

CommandQueue &CommandListManager::GetCopyQueue(void)
{
	return *m_CopyQueue;
}

CommandQueue &CommandListManager::GetQueue(D3D12_COMMAND_LIST_TYPE Type)
{
	switch (Type) {
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return *m_ComputeQueue;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return *m_CopyQueue;
	default:
		return *m_GraphicsQueue;
	}
}

ID3D12CommandQueue *CommandListManager::GetCommandQueue()
{
	return m_GraphicsQueue->GetCommandQueue();
}

void CommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList **List,
					      ID3D12CommandAllocator **Allocator)
{
	switch (Type) {
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		*Allocator = m_GraphicsQueue->RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		*Allocator = m_ComputeQueue->RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		*Allocator = m_CopyQueue->RequestAllocator();
		break;
	}

	HRESULT hr = m_DeviceInstance->GetDevice()->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List));
	if (FAILED(hr)) {
		auto removeReason = m_DeviceInstance->GetDevice()->GetDeviceRemovedReason();
		__debugbreak();
		throw HRError("CommandListManager: Failed to create a new command list.", hr);
	}

	(*List)->SetName(L"CommandList");
}

bool CommandListManager::IsFenceComplete(uint64_t FenceValue)
{
	return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
}

void CommandListManager::WaitForFence(uint64_t FenceValue)
{
	CommandQueue &Producer =
		m_DeviceInstance->GetCommandManager().GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	Producer.WaitForFence(FenceValue);
}

void CommandListManager::IdleGPU(void)
{
	m_GraphicsQueue->WaitForIdle();
	m_ComputeQueue->WaitForIdle();
	m_CopyQueue->WaitForIdle();
}

CommandContext::CommandContext(D3D12DeviceInstance *DeviceInstance, D3D12_COMMAND_LIST_TYPE Type)
	: m_DeviceInstance(DeviceInstance),
	  m_Type(Type),
	  m_DynamicViewDescriptorHeap(m_DeviceInstance, *this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	  m_DynamicSamplerDescriptorHeap(m_DeviceInstance, *this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
	  m_CpuLinearAllocator(std::make_unique<LinearAllocator>(DeviceInstance, kCpuWritable)),
	  m_GpuLinearAllocator(std::make_unique<LinearAllocator>(DeviceInstance, kGpuExclusive))
{
	m_CommandList = nullptr;
	m_CurrentAllocator = nullptr;
	ZeroMemory(m_CurrentDescriptorHeaps, sizeof(m_CurrentDescriptorHeaps));

	m_CurGraphicsRootSignature = nullptr;
	m_CurComputeRootSignature = nullptr;
	m_CurPipelineState = nullptr;
	m_NumBarriersToFlush = 0;
}

CommandContext::~CommandContext(void)
{
	if (m_CommandList != nullptr)
		m_CommandList->Release();
}

void CommandContext::Reset(void)
{
	if (m_CommandList == nullptr) {
		throw HRError("Trying to reset a command list that is null.");
	}
	if (m_CurrentAllocator != nullptr) {
		throw HRError("Trying to reset a command list with no allocator set.");
	}

	m_CurrentAllocator = m_DeviceInstance->GetCommandManager().GetQueue(m_Type).RequestAllocator();
	m_CommandList->Reset(m_CurrentAllocator, nullptr);

	m_CurGraphicsRootSignature = nullptr;
	m_CurComputeRootSignature = nullptr;
	m_CurPipelineState = nullptr;
	m_NumBarriersToFlush = 0;

	BindDescriptorHeaps();
}

uint64_t CommandContext::Flush(bool WaitForCompletion)
{
	FlushResourceBarriers();

	if (m_CurrentAllocator == nullptr) {
		throw HRError("Trying to flush a command list with no allocator set.");
	}

	uint64_t FenceValue = m_DeviceInstance->GetCommandManager().GetQueue(m_Type).ExecuteCommandList(m_CommandList);

	if (WaitForCompletion)
		m_DeviceInstance->GetCommandManager().WaitForFence(FenceValue);

	m_CommandList->Reset(m_CurrentAllocator, nullptr);

	if (m_CurGraphicsRootSignature) {
		m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature);
	}
	if (m_CurComputeRootSignature) {
		m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature);
	}
	if (m_CurPipelineState) {
		m_CommandList->SetPipelineState(m_CurPipelineState);
	}

	BindDescriptorHeaps();

	return FenceValue;
}

uint64_t CommandContext::Finish(bool WaitForCompletion)
{
	FlushResourceBarriers();

	if (m_CurrentAllocator == nullptr) {
		throw HRError("Trying to finish a command list with no allocator set.");
	}

	CommandQueue &Queue = m_DeviceInstance->GetCommandManager().GetQueue(m_Type);

	uint64_t FenceValue = Queue.ExecuteCommandList(m_CommandList);
	Queue.DiscardAllocator(FenceValue, m_CurrentAllocator);
	m_CurrentAllocator = nullptr;

	m_CpuLinearAllocator->CleanupUsedPages(FenceValue);
	m_GpuLinearAllocator->CleanupUsedPages(FenceValue);
	m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
	m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

	if (WaitForCompletion)
		m_DeviceInstance->GetCommandManager().WaitForFence(FenceValue);

	m_DeviceInstance->GetContextManager().FreeContext(this);

	return FenceValue;
}

void CommandContext::Initialize(void)
{
	m_DeviceInstance->GetCommandManager().CreateNewCommandList(m_Type, &m_CommandList, &m_CurrentAllocator);
}

ID3D12GraphicsCommandList *CommandContext::GetCommandList()
{
	return m_CommandList;
}

void CommandContext::CopyBuffer(GpuResource &Dest, GpuResource &Src)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();
	m_CommandList->CopyResource(Dest.GetResource(), Src.GetResource());
}

void CommandContext::CopyBufferRegion(GpuResource &Dest, size_t DestOffset, GpuResource &Src, size_t SrcOffset,
				      size_t NumBytes)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();
	m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetResource(), SrcOffset, NumBytes);
}

void CommandContext::CopySubresource(GpuResource &Dest, UINT DestSubIndex, GpuResource &Src, UINT SrcSubIndex)
{
	FlushResourceBarriers();

	D3D12_TEXTURE_COPY_LOCATION DestLocation = {Dest.GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
						    DestSubIndex};

	D3D12_TEXTURE_COPY_LOCATION SrcLocation = {Src.GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
						   SrcSubIndex};

	m_CommandList->CopyTextureRegion(&DestLocation, 0, 0, 0, &SrcLocation, nullptr);
}

void CommandContext::CopyTextureRegion(GpuResource &Dest, UINT x, UINT y, UINT z, GpuResource &Source, RECT &Rect)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();

	D3D12_TEXTURE_COPY_LOCATION destLoc = CD3DX12_TEXTURE_COPY_LOCATION(Dest.GetResource(), 0);
	D3D12_TEXTURE_COPY_LOCATION srcLoc = CD3DX12_TEXTURE_COPY_LOCATION(Source.GetResource(), 0);

	D3D12_BOX box = {};
	box.back = 1;
	box.left = Rect.left;
	box.right = Rect.right;
	box.top = Rect.top;
	box.bottom = Rect.bottom;

	m_CommandList->CopyTextureRegion(&destLoc, x, y, z, &srcLoc, &box);
}

void CommandContext::UpdateTexture(GpuResource &Dest, UploadBuffer &buffer)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	FlushResourceBarriers();

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTextureDesc;
	D3D12_TEXTURE_COPY_LOCATION srcLocation;
	D3D12_TEXTURE_COPY_LOCATION dstLocation;
	UINT NumRows, RowPitch;
	UINT64 RowLength;

	auto texDesc = Dest.GetResource()->GetDesc();
	UINT64 TotalBytes;

	m_DeviceInstance->GetDevice()->GetCopyableFootprints(&texDesc, 0, 1, 0, &placedTextureDesc, &NumRows,
							     &RowLength, &TotalBytes);
	RowPitch = placedTextureDesc.Footprint.RowPitch;

	dstLocation.pResource = Dest.GetResource();
	dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	dstLocation.SubresourceIndex = 0;

	srcLocation.pResource = buffer.GetResource();
	srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcLocation.PlacedFootprint = placedTextureDesc;

	m_CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, NULL);
}

uint32_t CommandContext::ReadbackTexture(ReadbackBuffer &DstBuffer, GpuResource &SrcBuffer)
{
	uint64_t CopySize = 0;

	// The footprint may depend on the device of the resource, but we assume there is only one device.
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;

	auto resourceDesc = SrcBuffer.GetResource()->GetDesc();
	m_DeviceInstance->GetDevice()->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr,
							     &CopySize);

	DstBuffer.Create(L"Readback", (uint32_t)CopySize, 1);

	TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

	auto DescLocation = CD3DX12_TEXTURE_COPY_LOCATION(DstBuffer.GetResource(), PlacedFootprint);
	auto SrcLocation = CD3DX12_TEXTURE_COPY_LOCATION(SrcBuffer.GetResource(), 0);
	m_CommandList->CopyTextureRegion(&DescLocation, 0, 0, 0, &SrcLocation, nullptr);

	return PlacedFootprint.Footprint.RowPitch;
}

DynAlloc CommandContext::ReserveUploadMemory(size_t SizeInBytes)
{
	return m_CpuLinearAllocator->Allocate(SizeInBytes);
}

void CommandContext::WriteBuffer(GpuResource &Dest, size_t DestOffset, const void *BufferData, size_t NumBytes)
{
	if (BufferData == nullptr) {
		return;
	}

	if (!Math::IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to WriteBuffer must be 16-byte aligned.");
	}
	DynAlloc TempSpace = m_CpuLinearAllocator->Allocate(NumBytes, 512);
	SIMDMemCopy(TempSpace.DataPtr, BufferData, Math::DivideByMultiple(NumBytes, 16));
	CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes);
}

void CommandContext::TransitionResource(GpuResource &Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
	D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

	if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
		if ((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) != OldState) {
			throw HRError("Invalid resource state for compute command queue.");
		}

		if ((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) != NewState) {
			throw HRError("Invalid resource state for compute command queue.");
		}
	}

	if (OldState != NewState) {
		if (m_NumBarriersToFlush >= kMaxNumDescriptorTables) {
			throw HRError("Exceeded arbitrary limit on buffered barriers");
		}
		D3D12_RESOURCE_BARRIER &BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Transition.pResource = Resource.GetResource();
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = OldState;
		BarrierDesc.Transition.StateAfter = NewState;

		// Check to see if we already started the transition
		if (NewState == Resource.m_TransitioningState) {
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_END_ONLY;
			Resource.m_TransitioningState = (D3D12_RESOURCE_STATES)-1;
		} else
			BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;

		Resource.m_UsageState = NewState;
	} else if (NewState == D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
		InsertUAVBarrier(Resource, FlushImmediate);

	if (FlushImmediate || m_NumBarriersToFlush == kMaxNumDescriptorTables)
		FlushResourceBarriers();
}

void CommandContext::BeginResourceTransition(GpuResource &Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
	// If it's already transitioning, finish that transition
	if (Resource.m_TransitioningState != (D3D12_RESOURCE_STATES)-1)
		TransitionResource(Resource, Resource.m_TransitioningState);

	D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

	if (OldState != NewState) {
		if (m_NumBarriersToFlush >= kMaxNumDescriptorTables) {
			throw HRError("Exceeded arbitrary limit on buffered barriers");
		}
		D3D12_RESOURCE_BARRIER &BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Transition.pResource = Resource.GetResource();
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = OldState;
		BarrierDesc.Transition.StateAfter = NewState;

		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

		Resource.m_TransitioningState = NewState;
	}

	if (FlushImmediate || m_NumBarriersToFlush == kMaxNumDescriptorTables)
		FlushResourceBarriers();
}

void CommandContext::InsertUAVBarrier(GpuResource &Resource, bool FlushImmediate)
{
	if (m_NumBarriersToFlush >= kMaxNumDescriptorTables) {
		throw HRError("Exceeded arbitrary limit on buffered barriers");
	}
	D3D12_RESOURCE_BARRIER &BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.UAV.pResource = Resource.GetResource();

	if (FlushImmediate)
		FlushResourceBarriers();
}

void CommandContext::InsertAliasBarrier(GpuResource &Before, GpuResource &After, bool FlushImmediate)
{
	if (m_NumBarriersToFlush >= kMaxNumDescriptorTables) {
		throw HRError("Exceeded arbitrary limit on buffered barriers");
	}
	D3D12_RESOURCE_BARRIER &BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_ALIASING;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.Aliasing.pResourceBefore = Before.GetResource();
	BarrierDesc.Aliasing.pResourceAfter = After.GetResource();

	if (FlushImmediate)
		FlushResourceBarriers();
}

inline void CommandContext::FlushResourceBarriers(void)
{
	if (m_NumBarriersToFlush > 0) {
		m_CommandList->ResourceBarrier(m_NumBarriersToFlush, m_ResourceBarrierBuffer);
		m_NumBarriersToFlush = 0;
	}
}

void CommandContext::InsertTimeStamp(ID3D12QueryHeap *pQueryHeap, uint32_t QueryIdx)
{
	m_CommandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
}

void CommandContext::ResolveTimeStamps(ID3D12Resource *pReadbackHeap, ID3D12QueryHeap *pQueryHeap, uint32_t NumQueries)
{
	m_CommandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
}

void CommandContext::PIXBeginEvent(const wchar_t *label) {}

void CommandContext::PIXEndEvent(void) {}

void CommandContext::PIXSetMarker(const wchar_t *label) {}

void CommandContext::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap *HeapPtr)
{
	if (m_CurrentDescriptorHeaps[Type] != HeapPtr) {
		m_CurrentDescriptorHeaps[Type] = HeapPtr;
		BindDescriptorHeaps();
	}
}

void CommandContext::SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[],
					ID3D12DescriptorHeap *HeapPtrs[])
{
	bool AnyChanged = false;

	for (UINT i = 0; i < HeapCount; ++i) {
		if (m_CurrentDescriptorHeaps[Type[i]] != HeapPtrs[i]) {
			m_CurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
			AnyChanged = true;
		}
	}

	if (AnyChanged)
		BindDescriptorHeaps();
}

void CommandContext::SetPipelineState(const PSO &PSO)
{
	ID3D12PipelineState *PipelineState = PSO.GetPipelineStateObject();
	if (PipelineState == m_CurPipelineState)
		return;

	m_CommandList->SetPipelineState(PipelineState);
	m_CurPipelineState = PipelineState;
}

void CommandContext::SetPredication(ID3D12Resource *Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
{
	m_CommandList->SetPredication(Buffer, BufferOffset, Op);
}

void CommandContext::BindDescriptorHeaps(void)
{
	UINT NonNullHeaps = 0;
	ID3D12DescriptorHeap *HeapsToBind[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	for (UINT i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
		ID3D12DescriptorHeap *HeapIter = m_CurrentDescriptorHeaps[i];
		if (HeapIter != nullptr)
			HeapsToBind[NonNullHeaps++] = HeapIter;
	}

	if (NonNullHeaps > 0)
		m_CommandList->SetDescriptorHeaps(NonNullHeaps, HeapsToBind);
}

void GraphicsContext::ClearUAV(GpuBuffer &Target)
{
	FlushResourceBarriers();

	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
	const UINT ClearColor[4] = {};
	m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor,
						    0, nullptr);
}

void GraphicsContext::ClearColor(D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const FLOAT ColorRGBA[4], UINT NumRects,
				 const D3D12_RECT *pRects)
{
	FlushResourceBarriers();
	m_CommandList->ClearRenderTargetView(RenderTargetView, ColorRGBA, (pRects == nullptr) ? 0 : 1, pRects);
}

void GraphicsContext::ClearDepth(DepthBuffer &Target)
{
	FlushResourceBarriers();
	m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH, Target.GetClearDepth(),
					     Target.GetClearStencil(), 0, nullptr);
}

void GraphicsContext::ClearStencil(DepthBuffer &Target)
{
	FlushResourceBarriers();
	m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_STENCIL, Target.GetClearDepth(),
					     Target.GetClearStencil(), 0, nullptr);
}

void GraphicsContext::ClearDepthAndStencil(DepthBuffer &Target)
{
	FlushResourceBarriers();
	m_CommandList->ClearDepthStencilView(Target.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
					     Target.GetClearDepth(), Target.GetClearStencil(), 0, nullptr);
}

void GraphicsContext::BeginQuery(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
	m_CommandList->BeginQuery(QueryHeap, Type, HeapIndex);
}

void GraphicsContext::EndQuery(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex)
{
	m_CommandList->EndQuery(QueryHeap, Type, HeapIndex);
}

void GraphicsContext::ResolveQueryData(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex,
				       UINT NumQueries, ID3D12Resource *DestinationBuffer,
				       UINT64 DestinationBufferOffset)
{
	m_CommandList->ResolveQueryData(QueryHeap, Type, StartIndex, NumQueries, DestinationBuffer,
					DestinationBufferOffset);
}

void GraphicsContext::SetRootSignature(const RootSignature &RootSig)
{
	if (RootSig.GetSignature() == m_CurGraphicsRootSignature)
		return;

	m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature = RootSig.GetSignature());

	m_DynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
	m_DynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
}

void GraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[])
{
	m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, nullptr);
}

void GraphicsContext::SetRenderTargets(UINT NumRenderTargetDescriptors,
				       const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
				       BOOL RTsSingleHandleToDescriptorRange,
				       const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor)
{
	m_CommandList->OMSetRenderTargets(NumRenderTargetDescriptors, pRenderTargetDescriptors,
					  RTsSingleHandleToDescriptorRange, pDepthStencilDescriptor);
}

void GraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[],
				       D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
}

void GraphicsContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV)
{
	SetRenderTargets(1, &RTV);
}

void GraphicsContext::SetNullRenderTarget()
{
	m_CommandList->OMSetRenderTargets(0, nullptr, FALSE, nullptr);
}

void GraphicsContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	SetRenderTargets(1, &RTV, DSV);
}

void GraphicsContext::SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	SetRenderTargets(0, nullptr, DSV);
}

void GraphicsContext::SetViewport(const D3D12_VIEWPORT &vp)
{
	m_CommandList->RSSetViewports(1, &vp);
}

void GraphicsContext::SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth, FLOAT maxDepth)
{
	D3D12_VIEWPORT vp;
	vp.Width = w;
	vp.Height = h;
	vp.MinDepth = minDepth;
	vp.MaxDepth = maxDepth;
	vp.TopLeftX = x;
	vp.TopLeftY = y;
	m_CommandList->RSSetViewports(1, &vp);
}

void GraphicsContext::SetScissor(const D3D12_RECT &rect)
{
	if (!(rect.left < rect.right && rect.top < rect.bottom)) {
		throw HRError("Invalid scissor rectangle passed to SetViewportAndScissor.");
	}
	m_CommandList->RSSetScissorRects(1, &rect);
}

void GraphicsContext::SetScissor(UINT left, UINT top, UINT right, UINT bottom)
{
	SetScissor(CD3DX12_RECT(left, top, right, bottom));
}

void GraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT &vp, const D3D12_RECT &rect)
{
	if (!(rect.left < rect.right && rect.top < rect.bottom)) {
		throw HRError("Invalid scissor rectangle passed to SetViewportAndScissor.");
	}
	m_CommandList->RSSetViewports(1, &vp);
	m_CommandList->RSSetScissorRects(1, &rect);
}

void GraphicsContext::SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h)
{
	SetViewport((float)x, (float)y, (float)w, (float)h);
	SetScissor(x, y, x + w, y + h);
}

void GraphicsContext::SetStencilRef(UINT StencilRef)
{
	m_CommandList->OMSetStencilRef(StencilRef);
}

void GraphicsContext::SetBlendFactor(Color BlendFactor)
{
	m_CommandList->OMSetBlendFactor(BlendFactor.ptr);
}

void GraphicsContext::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology)
{
	m_CommandList->IASetPrimitiveTopology(Topology);
}

void GraphicsContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void *pConstants)
{
	m_CommandList->SetGraphicsRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
}

void GraphicsContext::SetConstant(UINT RootIndex, UINT Offset, UINT Val)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Val, Offset);
}

void GraphicsContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
	m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
}

void GraphicsContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	if (!Math::IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicConstantBufferView must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	// memcpy(cb.DataPtr, BufferData, BufferSize);
	m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

void GraphicsContext::SetBufferSRV(UINT RootIndex, const GpuBuffer &SRV, UINT64 Offset)
{
	if ((SRV.m_UsageState &
	     (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) == 0) {
		throw HRError("Trying to set a SRV root view on a buffer that is not in a SRV state.");
	}

	m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

void GraphicsContext::SetBufferUAV(UINT RootIndex, const GpuBuffer &UAV, UINT64 Offset)
{
	if ((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == 0) {
		throw HRError("Trying to set a UAV root view on a buffer that is not in the UAV state.");
	}

	m_CommandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

void GraphicsContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
{
	m_CommandList->SetGraphicsRootDescriptorTable(RootIndex, FirstHandle);
}

void GraphicsContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

void GraphicsContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count,
					    const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void GraphicsContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

void GraphicsContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count,
					 const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void GraphicsContext::SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &IBView)
{
	m_CommandList->IASetIndexBuffer(&IBView);
}

void GraphicsContext::SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW &VBView)
{
	SetVertexBuffers(Slot, 1, &VBView);
}

void GraphicsContext::SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[])
{
	m_CommandList->IASetVertexBuffers(StartSlot, Count, VBViews);
}

void GraphicsContext::SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void *VertexData)
{
	if (!Math::IsAligned(VertexData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicVB must be 16-byte aligned.");
	}
	size_t BufferSize = Math::AlignUp(NumVertices * VertexStride, 16);
	DynAlloc vb = m_CpuLinearAllocator->Allocate(BufferSize);

	SIMDMemCopy(vb.DataPtr, VertexData, BufferSize >> 4);

	D3D12_VERTEX_BUFFER_VIEW VBView;
	VBView.BufferLocation = vb.GpuAddress;
	VBView.SizeInBytes = (UINT)BufferSize;
	VBView.StrideInBytes = (UINT)VertexStride;

	m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
}

void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t *IndexData)
{
	if (!Math::IsAligned(IndexData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicIB must be 16-byte aligned.");
	}
	size_t BufferSize = Math::AlignUp(IndexCount * sizeof(uint16_t), 16);
	DynAlloc ib = m_CpuLinearAllocator->Allocate(BufferSize);

	SIMDMemCopy(ib.DataPtr, IndexData, BufferSize >> 4);

	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = ib.GpuAddress;
	IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
	IBView.Format = DXGI_FORMAT_R16_UINT;

	m_CommandList->IASetIndexBuffer(&IBView);
}

void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	if (!Math::IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicSRV must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
}

void GraphicsContext::Draw(UINT VertexCount, UINT VertexStartOffset)
{
	DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
}

void GraphicsContext::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
	DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

void GraphicsContext::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation,
				    UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

void GraphicsContext::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
					   INT BaseVertexLocation, UINT StartInstanceLocation)
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation,
					    BaseVertexLocation, StartInstanceLocation);
}

void GraphicsContext::DrawIndirect(GpuBuffer &ArgumentBuffer, uint64_t ArgumentBufferOffset)
{
	ExecuteIndirect(m_DeviceInstance->GetDrawIndirectCommandSignature(), ArgumentBuffer, ArgumentBufferOffset);
}

void GraphicsContext::ExecuteIndirect(CommandSignature &CommandSig, GpuBuffer &ArgumentBuffer,
				      uint64_t ArgumentStartOffset, uint32_t MaxCommands,
				      GpuBuffer *CommandCounterBuffer, uint64_t CounterOffset)
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
	m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands, ArgumentBuffer.GetResource(),
				       ArgumentStartOffset,
				       CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(),
				       CounterOffset);
}

void ComputeContext::ClearUAV(GpuBuffer &Target)
{
	FlushResourceBarriers();

	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
	const UINT ClearColor[4] = {};
	m_CommandList->ClearUnorderedAccessViewUint(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(), ClearColor,
						    0, nullptr);
}

void ComputeContext::SetRootSignature(const RootSignature &RootSig)
{
	if (RootSig.GetSignature() == m_CurComputeRootSignature)
		return;

	m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature = RootSig.GetSignature());

	m_DynamicViewDescriptorHeap.ParseComputeRootSignature(RootSig);
	m_DynamicSamplerDescriptorHeap.ParseComputeRootSignature(RootSig);
}

void ComputeContext::SetConstantArray(UINT RootIndex, UINT NumConstants, const void *pConstants)
{
	m_CommandList->SetComputeRoot32BitConstants(RootIndex, NumConstants, pConstants, 0);
}

void ComputeContext::SetConstant(UINT RootIndex, UINT Offset, UINT Val)
{
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, Val, Offset);
}

void ComputeContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
	m_CommandList->SetComputeRootConstantBufferView(RootIndex, CBV);
}

void ComputeContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	if (!Math::IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicConstantBufferView must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	//memcpy(cb.DataPtr, BufferData, BufferSize);
	m_CommandList->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress);
}

void ComputeContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	if (BufferData == nullptr) {
		return;
	}
	if (!Math::IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicSRV must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	m_CommandList->SetComputeRootShaderResourceView(RootIndex, cb.GpuAddress);
}

void ComputeContext::SetBufferSRV(UINT RootIndex, const GpuBuffer &SRV, UINT64 Offset)
{
	if ((SRV.m_UsageState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) == 0) {
		throw HRError(
			"ComputeContext::SetBufferSRV: SRV resource is not in D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE state.");
	}
	m_CommandList->SetComputeRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

void ComputeContext::SetBufferUAV(UINT RootIndex, const GpuBuffer &UAV, UINT64 Offset)
{
	if ((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) == 0) {
		throw HRError(
			"ComputeContext::SetBufferUAV: UAV resource is not in D3D12_RESOURCE_STATE_UNORDERED_ACCESS state.");
	}

	m_CommandList->SetComputeRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

void ComputeContext::SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle)
{
	m_CommandList->SetComputeRootDescriptorTable(RootIndex, FirstHandle);
}

void ComputeContext::SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

void ComputeContext::SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count,
					   const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_DynamicViewDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void ComputeContext::SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

void ComputeContext::SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count,
					const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_DynamicSamplerDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

void ComputeContext::Dispatch(size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ)
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
	m_CommandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
}

void ComputeContext::Dispatch1D(size_t ThreadCountX, size_t GroupSizeX)
{
	Dispatch(Math::DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1);
}

void ComputeContext::Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY)
{
	Dispatch(Math::DivideByMultiple(ThreadCountX, GroupSizeX), Math::DivideByMultiple(ThreadCountY, GroupSizeY), 1);
}

void ComputeContext::Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX,
				size_t GroupSizeY, size_t GroupSizeZ)
{
	Dispatch(Math::DivideByMultiple(ThreadCountX, GroupSizeX), Math::DivideByMultiple(ThreadCountY, GroupSizeY),
		 Math::DivideByMultiple(ThreadCountZ, GroupSizeZ));
}

void ComputeContext::DispatchIndirect(GpuBuffer &ArgumentBuffer, uint64_t ArgumentBufferOffset)
{
	ExecuteIndirect(m_DeviceInstance->GetDispatchIndirectCommandSignature(), ArgumentBuffer, ArgumentBufferOffset);
}

void ComputeContext::ExecuteIndirect(CommandSignature &CommandSig, GpuBuffer &ArgumentBuffer,
				     uint64_t ArgumentStartOffset, uint32_t MaxCommands,
				     GpuBuffer *CommandCounterBuffer, uint64_t CounterOffset)
{
	FlushResourceBarriers();
	m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
	m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
	m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands, ArgumentBuffer.GetResource(),
				       ArgumentStartOffset,
				       CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(),
				       CounterOffset);
}

SamplerDesc::SamplerDesc(D3D12DeviceInstance *DeviceInstance) : m_DeviceInstance(DeviceInstance)
{
	Sampler.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	Filter = D3D12_FILTER_ANISOTROPIC;
	AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	MipLODBias = 0.0f;
	MaxAnisotropy = 16;
	ComparisonFunc = D3D12_COMPARISON_FUNC(0);
	BorderColor[0] = 1.0f;
	BorderColor[1] = 1.0f;
	BorderColor[2] = 1.0f;
	BorderColor[3] = 1.0f;
	MinLOD = 0.0f;
	MaxLOD = D3D12_FLOAT32_MAX;
}

SamplerDesc::~SamplerDesc()
{
	if (Sampler.ptr != D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		Sampler.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}
}

void SamplerDesc::SetTextureAddressMode(D3D12_TEXTURE_ADDRESS_MODE AddressMode)
{
	AddressU = AddressMode;
	AddressV = AddressMode;
	AddressW = AddressMode;
}

void SamplerDesc::SetBorderColor(Color Border)
{
	BorderColor[0] = Border.x;
	BorderColor[1] = Border.y;
	BorderColor[2] = Border.z;
	BorderColor[3] = Border.w;
}

void SamplerDesc::CreateDescriptor(void)
{
	Sampler = m_DeviceInstance->AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	m_DeviceInstance->GetDevice()->CreateSampler(this, Sampler);
}

void SamplerDesc::CreateDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE Handle)
{
	if (Handle.ptr == 0 || Handle.ptr == -1) {
		throw HRError("Invalid descriptor handle for sampler creation");
	}
	m_DeviceInstance->GetDevice()->CreateSampler(this, Handle);
}

class HagsStatus {
public:
	enum DriverSupport { ALWAYS_OFF, ALWAYS_ON, EXPERIMENTAL, STABLE, UNKNOWN };

	bool enabled;
	bool enabled_by_default;
	DriverSupport support;

	explicit HagsStatus(const D3DKMT_WDDM_2_7_CAPS *caps);

	void SetDriverSupport(const UINT DXGKVal);

	std::string ToString() const;

private:
	const char *DriverSupportToString() const;
};

HagsStatus::HagsStatus(const D3DKMT_WDDM_2_7_CAPS *caps)
{
	enabled = caps->HwSchEnabled;
	enabled_by_default = caps->HwSchEnabledByDefault;
	support = caps->HwSchSupported ? DriverSupport::STABLE : DriverSupport::ALWAYS_OFF;
}

void HagsStatus::SetDriverSupport(const UINT DXGKVal)
{
	switch (DXGKVal) {
	case DXGK_FEATURE_SUPPORT_ALWAYS_OFF:
		support = ALWAYS_OFF;
		break;
	case DXGK_FEATURE_SUPPORT_ALWAYS_ON:
		support = ALWAYS_ON;
		break;
	case DXGK_FEATURE_SUPPORT_EXPERIMENTAL:
		support = EXPERIMENTAL;
		break;
	case DXGK_FEATURE_SUPPORT_STABLE:
		support = STABLE;
		break;
	default:
		support = UNKNOWN;
	}
}

std::string HagsStatus::ToString() const
{
	std::string status = enabled ? "Enabled" : "Disabled";
	status += " (Default: ";
	status += enabled_by_default ? "Yes" : "No";
	status += ", Driver status: ";
	status += DriverSupportToString();
	status += ")";

	return status;
}

const char *HagsStatus::DriverSupportToString() const
{
	switch (support) {
	case ALWAYS_OFF:
		return "Unsupported";
	case ALWAYS_ON:
		return "Always On";
	case EXPERIMENTAL:
		return "Experimental";
	case STABLE:
		return "Supported";
	default:
		return "Unknown";
	}
}

D3D12DeviceInstance::D3D12DeviceInstance() {}

#define DEBUG_D3D12 0

void D3D12DeviceInstance::Initialize(int32_t adaptorIndex)
{
	ComPtr<ID3D12Device> pDevice;
	uint32_t useDebugLayers = 0;
	DWORD dxgiFactoryFlags = 0;
#if DEBUG_D3D12
	useDebugLayers = 1;
	dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	if (useDebugLayers) {
		EnableDebugLayer();
	}

	// Obtain the DXGI factory
	ComPtr<IDXGIFactory6> dxgiFactory;
	HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		throw HRError("Failed to create DXGI Factory", hr);
	}

	m_DxgiFactory = dxgiFactory.Detach();
	CreateD3DAdapterAndDevice(adaptorIndex);
	if (useDebugLayers) {
		EnableDebugInofQueue();
	}

	// We like to do read-modify-write operations on UAVs during post processing.  To support that, we
	// need to either have the hardware do typed UAV loads of R11G11B10_FLOAT or we need to manually
	// decode an R32_UINT representation of the same buffer.  This code determines if we get the hardware
	// load support.

	CheckFeatureSupports();

	m_PageManager[0] = std::make_unique<LinearAllocatorPageManager>(this, kGpuExclusive);
	m_PageManager[1] = std::make_unique<LinearAllocatorPageManager>(this, kCpuWritable);

	m_DispatchIndirectCommandSignature = std::make_unique<CommandSignature>(this, 1);
	m_DrawIndirectCommandSignature = std::make_unique<CommandSignature>(this, 1);

	m_CommandManager = std::make_unique<CommandListManager>();
	m_ContextManager = std::make_unique<ContextManager>(this);

	m_CommandManager->Create(this);
}

void D3D12DeviceInstance::Uninitialize() {}

ID3D12Device *D3D12DeviceInstance::GetDevice()
{
	return m_Device;
}

IDXGIAdapter1 *D3D12DeviceInstance::GetAdapter()
{
	return m_Adapter;
}

IDXGIFactory6 *D3D12DeviceInstance::GetDxgiFactory()
{
	return m_DxgiFactory;
}

CommandListManager &D3D12DeviceInstance::GetCommandManager()
{
	return *m_CommandManager;
}

ContextManager &D3D12DeviceInstance::GetContextManager()
{
	return *m_ContextManager;
}

std::map<size_t, ComPtr<ID3D12RootSignature>> &D3D12DeviceInstance::GetRootSignatureHashMap()
{
	return m_RootSignatureHashMap;
}

LinearAllocatorPageManager *D3D12DeviceInstance::GetPageManager(LinearAllocatorType AllocatorType)
{
	return m_PageManager[AllocatorType].get();
}

std::map<size_t, ComPtr<ID3D12PipelineState>> &D3D12DeviceInstance::GetGraphicsPSOHashMap()
{
	return m_GraphicsPSOHashMap;
}

std::map<size_t, ComPtr<ID3D12PipelineState>> &D3D12DeviceInstance::GetComputePSOHashMap()
{
	return m_ComputePSOHashMap;
}

CommandSignature &D3D12DeviceInstance::GetDispatchIndirectCommandSignature()
{
	return *m_DispatchIndirectCommandSignature;
}

CommandSignature &D3D12DeviceInstance::GetDrawIndirectCommandSignature()
{
	return *m_DrawIndirectCommandSignature;
}

DescriptorAllocator *D3D12DeviceInstance::GetDescriptorAllocator()
{
	return m_DescriptorAllocator;
}

ID3D12DescriptorHeap *D3D12DeviceInstance::RequestCommonHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Desc.Type = Type;
	Desc.NumDescriptors = kMaxNumDescriptors;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Desc.NodeMask = 1;

	ComPtr<ID3D12DescriptorHeap> pHeap;
	HRESULT hr = m_Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap));
	if (FAILED(hr)) {
		throw HRError("create common heap failed", hr);
	}

	m_DescriptorHeapPool.emplace_back(pHeap);
	return pHeap.Get();
}

ID3D12DescriptorHeap *D3D12DeviceInstance::RequestDynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
{
	uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

	while (!m_DynamicRetiredDescriptorHeaps[idx].empty() &&
	       m_CommandManager->IsFenceComplete(m_DynamicRetiredDescriptorHeaps[idx].front().first)) {
		m_DynamicAvailableDescriptorHeaps[idx].push(m_DynamicRetiredDescriptorHeaps[idx].front().second);
		m_DynamicRetiredDescriptorHeaps[idx].pop();
	}

	if (!m_DynamicAvailableDescriptorHeaps[idx].empty()) {
		ID3D12DescriptorHeap *HeapPtr = m_DynamicAvailableDescriptorHeaps[idx].front();
		m_DynamicAvailableDescriptorHeaps[idx].pop();
		return HeapPtr;
	} else {
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.Type = HeapType;
		HeapDesc.NumDescriptors = kMaxNumDescriptors;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HeapDesc.NodeMask = 1;
		ComPtr<ID3D12DescriptorHeap> HeapPtr;
		HRESULT hr = m_Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr));
		if (FAILED(hr)) {
			throw HRError("create shader heap failed", hr);
		}

		m_DynamicDescriptorHeapPool[idx].emplace_back(HeapPtr);
		return HeapPtr.Get();
	}
}

void D3D12DeviceInstance::DiscardDynamicDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType,
							uint64_t FenceValueForReset,
							const std::vector<ID3D12DescriptorHeap *> &UsedHeaps)
{
	uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
	for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter)
		m_DynamicRetiredDescriptorHeaps[idx].push(std::make_pair(FenceValueForReset, *iter));
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12DeviceInstance::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count)
{
	return m_DescriptorAllocator[Type].Allocate(Count);
}

GraphicsContext *D3D12DeviceInstance::GetNewGraphicsContext(const std::wstring &ID)
{
	CommandContext *NewContext = m_ContextManager->AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	NewContext->SetID(ID);
	return (GraphicsContext *)NewContext;
}

ComputeContext *D3D12DeviceInstance::GetNewComputeContext(const std::wstring &ID, bool Async)
{
	CommandContext *NewContext = m_ContextManager->AllocateContext(Async ? D3D12_COMMAND_LIST_TYPE_COMPUTE
									     : D3D12_COMMAND_LIST_TYPE_DIRECT);
	NewContext->SetID(ID);
	return (ComputeContext *)NewContext;
}

inline void MemcpySubresource(_In_ const D3D12_MEMCPY_DEST *pDest, _In_ const D3D12_SUBRESOURCE_DATA *pSrc,
			      SIZE_T RowSizeInBytes, UINT NumRows, UINT NumSlices) noexcept
{
	for (UINT z = 0; z < NumSlices; ++z) {
		auto pDestSlice = static_cast<BYTE *>(pDest->pData) + pDest->SlicePitch * z;
		auto pSrcSlice = static_cast<const BYTE *>(pSrc->pData) + pSrc->SlicePitch * LONG_PTR(z);
		for (UINT y = 0; y < NumRows; ++y) {
			memcpy(pDestSlice + pDest->RowPitch * y, pSrcSlice + pSrc->RowPitch * LONG_PTR(y),
			       RowSizeInBytes);
		}
	}
}

//------------------------------------------------------------------------------------------------
// Row-by-row memcpy
inline void MemcpySubresource(_In_ const D3D12_MEMCPY_DEST *pDest, _In_ const void *pResourceData,
			      _In_ const D3D12_SUBRESOURCE_INFO *pSrc, SIZE_T RowSizeInBytes, UINT NumRows,
			      UINT NumSlices) noexcept
{
	for (UINT z = 0; z < NumSlices; ++z) {
		auto pDestSlice = static_cast<BYTE *>(pDest->pData) + pDest->SlicePitch * z;
		auto pSrcSlice =
			(static_cast<const BYTE *>(pResourceData) + pSrc->Offset) + pSrc->DepthPitch * ULONG_PTR(z);
		for (UINT y = 0; y < NumRows; ++y) {
			memcpy(pDestSlice + pDest->RowPitch * y, pSrcSlice + pSrc->RowPitch * ULONG_PTR(y),
			       RowSizeInBytes);
		}
	}
}

inline UINT64 UpdateSubresources(_In_ ID3D12GraphicsCommandList *pCmdList, _In_ ID3D12Resource *pDestinationResource,
				 _In_ ID3D12Resource *pIntermediate,
				 _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
				 _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
				 UINT64 RequiredSize,
				 _In_reads_(NumSubresources) const D3D12_PLACED_SUBRESOURCE_FOOTPRINT *pLayouts,
				 _In_reads_(NumSubresources) const UINT *pNumRows,
				 _In_reads_(NumSubresources) const UINT64 *pRowSizesInBytes,
				 _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA *pSrcData) noexcept
{
	// Minor validation
	auto IntermediateDesc = pIntermediate->GetDesc();
	auto DestinationDesc = pDestinationResource->GetDesc();
	if (IntermediateDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER ||
	    IntermediateDesc.Width < RequiredSize + pLayouts[0].Offset || RequiredSize > SIZE_T(-1) ||
	    (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER &&
	     (FirstSubresource != 0 || NumSubresources != 1))) {
		return 0;
	}

	BYTE *pData;
	HRESULT hr = pIntermediate->Map(0, nullptr, reinterpret_cast<void **>(&pData));
	if (FAILED(hr)) {
		return 0;
	}

	for (UINT i = 0; i < NumSubresources; ++i) {
		if (pRowSizesInBytes[i] > SIZE_T(-1))
			return 0;
		D3D12_MEMCPY_DEST DestData = {pData + pLayouts[i].Offset, pLayouts[i].Footprint.RowPitch,
					      SIZE_T(pLayouts[i].Footprint.RowPitch) * SIZE_T(pNumRows[i])};
		MemcpySubresource(&DestData, &pSrcData[i], static_cast<SIZE_T>(pRowSizesInBytes[i]), pNumRows[i],
				  pLayouts[i].Footprint.Depth);
	}
	pIntermediate->Unmap(0, nullptr);

	if (DestinationDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
		pCmdList->CopyBufferRegion(pDestinationResource, 0, pIntermediate, pLayouts[0].Offset,
					   pLayouts[0].Footprint.Width);
	} else {
		for (UINT i = 0; i < NumSubresources; ++i) {
			CD3DX12_TEXTURE_COPY_LOCATION Dst(pDestinationResource, i + FirstSubresource);
			CD3DX12_TEXTURE_COPY_LOCATION Src(pIntermediate, pLayouts[i]);
			pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}
	}
	return RequiredSize;
}

inline UINT64 UpdateSubresources(_In_ ID3D12GraphicsCommandList *pCmdList, _In_ ID3D12Resource *pDestinationResource,
				 _In_ ID3D12Resource *pIntermediate, UINT64 IntermediateOffset,
				 _In_range_(0, D3D12_REQ_SUBRESOURCES) UINT FirstSubresource,
				 _In_range_(0, D3D12_REQ_SUBRESOURCES - FirstSubresource) UINT NumSubresources,
				 _In_reads_(NumSubresources) const D3D12_SUBRESOURCE_DATA *pSrcData) noexcept
{
	UINT64 RequiredSize = 0;
	auto MemToAlloc =
		static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) *
		NumSubresources;
	if (MemToAlloc > SIZE_MAX) {
		return 0;
	}
	void *pMem = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(MemToAlloc));
	if (pMem == nullptr) {
		return 0;
	}
	auto pLayouts = static_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT *>(pMem);
	auto pRowSizesInBytes = reinterpret_cast<UINT64 *>(pLayouts + NumSubresources);
	auto pNumRows = reinterpret_cast<UINT *>(pRowSizesInBytes + NumSubresources);

	auto Desc = pDestinationResource->GetDesc();
	ID3D12Device *pDevice = nullptr;
	pDestinationResource->GetDevice(IID_ID3D12Device, reinterpret_cast<void **>(&pDevice));
	pDevice->GetCopyableFootprints(&Desc, FirstSubresource, NumSubresources, IntermediateOffset, pLayouts, pNumRows,
				       pRowSizesInBytes, &RequiredSize);
	pDevice->Release();

	UINT64 Result = UpdateSubresources(pCmdList, pDestinationResource, pIntermediate, FirstSubresource,
					   NumSubresources, RequiredSize, pLayouts, pNumRows, pRowSizesInBytes,
					   pSrcData);
	HeapFree(GetProcessHeap(), 0, pMem);
	return Result;
}

void D3D12DeviceInstance::InitializeTexture(GpuResource &Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

	CommandContext &InitContext = *GetNewGraphicsContext();

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	DynAlloc mem = InitContext.ReserveUploadMemory(uploadBufferSize);
	UpdateSubresources(InitContext.m_CommandList, Dest.GetResource(), mem.Buffer.GetResource(), 0, 0,
			   NumSubresources, SubData);
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	InitContext.Finish(true);
}

void D3D12DeviceInstance::InitializeBuffer(GpuBuffer &Dest, const void *BufferData, size_t NumBytes, size_t DestOffset)
{
	CommandContext &InitContext = *GetNewGraphicsContext();

	DynAlloc mem = InitContext.ReserveUploadMemory(NumBytes);
	SIMDMemCopy(mem.DataPtr, BufferData, Math::DivideByMultiple(NumBytes, 16));

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, mem.Buffer.GetResource(), 0,
						    NumBytes);
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	InitContext.Finish(true);
}

void D3D12DeviceInstance::InitializeBuffer(GpuBuffer &Dest, const UploadBuffer &Src, size_t SrcOffset, size_t NumBytes,
					   size_t DestOffset)
{
	CommandContext &InitContext = *GetNewGraphicsContext();

	size_t MaxBytes = std::min<size_t>(Dest.GetBufferSize() - DestOffset, Src.GetBufferSize() - SrcOffset);
	NumBytes = std::min<size_t>(MaxBytes, NumBytes);

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST, true);
	InitContext.m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, (ID3D12Resource *)Src.GetResource(),
						    SrcOffset, NumBytes);
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ, true);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	InitContext.Finish(true);
}

void D3D12DeviceInstance::InitializeTextureArraySlice(GpuResource &Dest, UINT SliceIndex, GpuResource &Src)
{
	CommandContext &Context = *GetNewGraphicsContext();

	Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	Context.FlushResourceBarriers();

	const D3D12_RESOURCE_DESC &DestDesc = Dest.GetResource()->GetDesc();
	const D3D12_RESOURCE_DESC &SrcDesc = Src.GetResource()->GetDesc();

	if (!(SliceIndex < DestDesc.DepthOrArraySize && SrcDesc.DepthOrArraySize == 1 &&
	      DestDesc.Width == SrcDesc.Width && DestDesc.Height == SrcDesc.Height &&
	      DestDesc.MipLevels <= SrcDesc.MipLevels)) {
		throw HRError("InitializeTextureArraySlice: incompatible source and destination textures");
	}

	UINT SubResourceIndex = SliceIndex * DestDesc.MipLevels;

	for (UINT i = 0; i < DestDesc.MipLevels; ++i) {
		D3D12_TEXTURE_COPY_LOCATION destCopyLocation = {
			Dest.GetResource(), D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, SubResourceIndex + i};

		D3D12_TEXTURE_COPY_LOCATION srcCopyLocation = {Src.GetResource(),
							       D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i};

		Context.m_CommandList->CopyTextureRegion(&destCopyLocation, 0, 0, 0, &srcCopyLocation, nullptr);
	}

	Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);
	Context.Finish(true);
}

bool D3D12DeviceInstance::IsNV12TextureSupported() const
{
	return m_NV12Supported;
}

bool D3D12DeviceInstance::IsP010TextureSupported() const
{
	return m_P010Supported;
}

bool D3D12DeviceInstance::FastClearSupported() const
{
	return m_FastClearSupported;
}

void D3D12DeviceInstance::EnableDebugLayer()
{
	ComPtr<ID3D12Debug> debugInterface;
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)))) {
		debugInterface->EnableDebugLayer();

		uint32_t useGPUBasedValidation = 1;
		if (useGPUBasedValidation) {
			ComPtr<ID3D12Debug1> debugInterface1;
			if (SUCCEEDED((debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1))))) {
				debugInterface1->SetEnableGPUBasedValidation(true);
			}
		}
	} else {
	}

	ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiInfoQueue)))) {

		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
		dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

		DXGI_INFO_QUEUE_MESSAGE_ID hide[] = {
			80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */
			,
		};
		DXGI_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(hide);
		filter.DenyList.pIDList = hide;
		dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
	}
}

void D3D12DeviceInstance::EnableDebugInofQueue()
{
	ID3D12InfoQueue *pInfoQueue = nullptr;
	if (SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&pInfoQueue)))) {
		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY Categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY Severities[] = {D3D12_MESSAGE_SEVERITY_INFO};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID DenyIds[] = {
			// This occurs when there are uninitialized descriptors in a descriptor table, even when a
			// shader does not access the missing descriptors.  I find this is common when switching
			// shader permutations and not wanting to change much code to reorder resources.
			D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,

			// Triggered when a shader does not export all color components of a render target, such as
			// when only writing RGB to an R10G10B10A2 buffer, ignoring alpha.
			D3D12_MESSAGE_ID_CREATEGRAPHICSPIPELINESTATE_PS_OUTPUT_RT_OUTPUT_MISMATCH,

			// This occurs when a descriptor table is unbound even when a shader does not access the missing
			// descriptors.  This is common with a root signature shared between disparate shaders that
			// don't all need the same types of resources.
			D3D12_MESSAGE_ID_COMMAND_LIST_DESCRIPTOR_TABLE_NOT_SET,

			// RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_DUPLICATE_SUBRESOURCE_TRANSITIONS,

			// Suppress errors from calling ResolveQueryData with timestamps that weren't requested on a given frame.
			D3D12_MESSAGE_ID_RESOLVE_QUERY_INVALID_QUERY_STATE,

			// Ignoring InitialState D3D12_RESOURCE_STATE_COPY_DEST. Buffers are effectively created in state D3D12_RESOURCE_STATE_COMMON.
			D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
		};

		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		//NewFilter.DenyList.NumCategories = _countof(Categories);
		//NewFilter.DenyList.pCategoryList = Categories;
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		pInfoQueue->PushStorageFilter(&NewFilter);
		pInfoQueue->Release();
	}
}

void D3D12DeviceInstance::CheckFeatureSupports()
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS FeatureData = {};
	if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS, &FeatureData, sizeof(FeatureData)))) {
		if (FeatureData.TypedUAVLoadAdditionalFormats) {
			// HDR
			D3D12_FEATURE_DATA_FORMAT_SUPPORT Support = {
				DXGI_FORMAT_R11G11B10_FLOAT, D3D12_FORMAT_SUPPORT1_NONE, D3D12_FORMAT_SUPPORT2_NONE};

			if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support,
								    sizeof(Support))) &&
			    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0) {
				m_TypedUAVLoadSupport_R11G11B10_FLOAT = true;
			}

			// SRGB_LINEAR
			Support.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;

			if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support,
								    sizeof(Support))) &&
			    (Support.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0) {
				m_TypedUAVLoadSupport_R16G16B16A16_FLOAT = true;
			}

			// DXGI_FORMAT_NV12
			Support.Format = DXGI_FORMAT_NV12;
			if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support,
								    sizeof(Support))) &&
			    (Support.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0) {
				m_NV12Supported = true;
			}

			// DXGI_FORMAT_P010
			Support.Format = DXGI_FORMAT_P010;
			if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &Support,
								    sizeof(Support))) &&
			    (Support.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0) {
				m_P010Supported = true;
			}
		}
	}
}

std::optional<HagsStatus> D3D12DeviceInstance::GetAdapterHagsStatus(const DXGI_ADAPTER_DESC *desc)
{
	std::optional<HagsStatus> ret;
	D3DKMT_OPENADAPTERFROMLUID d3dkmt_openluid{};
	d3dkmt_openluid.AdapterLuid = desc->AdapterLuid;

	NTSTATUS res = D3DKMTOpenAdapterFromLuid(&d3dkmt_openluid);
	if (FAILED(res)) {
		blog(LOG_DEBUG, "Failed opening D3DKMT adapter: %x", res);
		return ret;
	}

	D3DKMT_WDDM_2_7_CAPS caps = {};
	D3DKMT_QUERYADAPTERINFO args = {};
	args.hAdapter = d3dkmt_openluid.hAdapter;
	args.Type = KMTQAITYPE_WDDM_2_7_CAPS;
	args.pPrivateDriverData = &caps;
	args.PrivateDriverDataSize = sizeof(caps);
	res = D3DKMTQueryAdapterInfo(&args);

	/* If this still fails we're likely on Windows 10 pre-2004
	 * where HAGS is not supported anyway. */
	if (SUCCEEDED(res)) {
		HagsStatus status(&caps);

		/* Starting with Windows 10 21H2 we can query more detailed
		 * support information (e.g. experimental status).
		 * This Is optional and failure doesn't matter. */
		D3DKMT_WDDM_2_9_CAPS ext_caps = {};
		args.hAdapter = d3dkmt_openluid.hAdapter;
		args.Type = KMTQAITYPE_WDDM_2_9_CAPS;
		args.pPrivateDriverData = &ext_caps;
		args.PrivateDriverDataSize = sizeof(ext_caps);
		res = D3DKMTQueryAdapterInfo(&args);

		if (SUCCEEDED(res))
			status.SetDriverSupport(ext_caps.HwSchSupportState);

		ret = status;
	} else {
		blog(LOG_WARNING, "Failed querying WDDM 2.7 caps: %x", res);
	}

	D3DKMT_CLOSEADAPTER d3dkmt_close = {d3dkmt_openluid.hAdapter};
	res = D3DKMTCloseAdapter(&d3dkmt_close);
	if (FAILED(res)) {
		blog(LOG_DEBUG, "Failed closing D3DKMT adapter %x: %x", d3dkmt_openluid.hAdapter, res);
	}

	return ret;
}

void D3D12DeviceInstance::EnumD3DAdapters(bool (*callback)(void *, const char *, uint32_t), void *param)
{
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw HRError("Failed to create DXGIFactory", hr);

	for (i = 0; factory->EnumAdapters1(i, adapter.Assign()) == S_OK; ++i) {
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		os_wcs_to_utf8(desc.Description, 0, name, sizeof(name));

		if (!callback(param, name, i))
			break;
	}
}

bool D3D12DeviceInstance::GetMonitorTarget(const MONITORINFOEX &info, DISPLAYCONFIG_TARGET_DEVICE_NAME &target)
{
	bool found = false;

	UINT32 numPath, numMode;
	if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &numPath, &numMode) == ERROR_SUCCESS) {
		std::vector<DISPLAYCONFIG_PATH_INFO> paths(numPath);
		std::vector<DISPLAYCONFIG_MODE_INFO> modes(numMode);
		if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPath, paths.data(), &numMode, modes.data(),
				       nullptr) == ERROR_SUCCESS) {
			paths.resize(numPath);
			for (size_t i = 0; i < numPath; ++i) {
				const DISPLAYCONFIG_PATH_INFO &path = paths[i];

				DISPLAYCONFIG_SOURCE_DEVICE_NAME
				source;
				source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
				source.header.size = sizeof(source);
				source.header.adapterId = path.sourceInfo.adapterId;
				source.header.id = path.sourceInfo.id;
				if (DisplayConfigGetDeviceInfo(&source.header) == ERROR_SUCCESS &&
				    wcscmp(info.szDevice, source.viewGdiDeviceName) == 0) {
					target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
					target.header.size = sizeof(target);
					target.header.adapterId = path.sourceInfo.adapterId;
					target.header.id = path.targetInfo.id;
					found = DisplayConfigGetDeviceInfo(&target.header) == ERROR_SUCCESS;
					break;
				}
			}
		}
	}

	return found;
}

bool D3D12DeviceInstance::GetOutputDesc1(IDXGIOutput *const output, DXGI_OUTPUT_DESC1 *desc1)
{
	ComPtr<IDXGIOutput6> output6;
	HRESULT hr = output->QueryInterface(IID_PPV_ARGS(output6.Assign()));
	bool success = SUCCEEDED(hr);
	if (success) {
		hr = output6->GetDesc1(desc1);
		success = SUCCEEDED(hr);
		if (!success) {
			blog(LOG_WARNING, "IDXGIOutput6::GetDesc1 failed: 0x%08lX", hr);
		}
	}

	return success;
}

// Note: Since an hmon can represent multiple monitors while in clone, this function as written will return
//  the value for the internal monitor if one exists, and otherwise the highest clone-path priority.
HRESULT D3D12DeviceInstance::GetPathInfo(_In_ PCWSTR pszDeviceName, _Out_ DISPLAYCONFIG_PATH_INFO *pPathInfo)
{
	HRESULT hr = S_OK;
	UINT32 NumPathArrayElements = 0;
	UINT32 NumModeInfoArrayElements = 0;
	DISPLAYCONFIG_PATH_INFO *PathInfoArray = nullptr;
	DISPLAYCONFIG_MODE_INFO *ModeInfoArray = nullptr;

	do {
		// In case this isn't the first time through the loop, delete the buffers allocated
		delete[] PathInfoArray;
		PathInfoArray = nullptr;

		delete[] ModeInfoArray;
		ModeInfoArray = nullptr;

		hr = HRESULT_FROM_WIN32(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements,
								    &NumModeInfoArrayElements));
		if (FAILED(hr)) {
			break;
		}

		PathInfoArray = new (std::nothrow) DISPLAYCONFIG_PATH_INFO[NumPathArrayElements];
		if (PathInfoArray == nullptr) {
			hr = E_OUTOFMEMORY;
			break;
		}

		ModeInfoArray = new (std::nothrow) DISPLAYCONFIG_MODE_INFO[NumModeInfoArrayElements];
		if (ModeInfoArray == nullptr) {
			hr = E_OUTOFMEMORY;
			break;
		}

		hr = HRESULT_FROM_WIN32(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &NumPathArrayElements, PathInfoArray,
							   &NumModeInfoArrayElements, ModeInfoArray, nullptr));
	} while (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER));

	INT DesiredPathIdx = -1;

	if (SUCCEEDED(hr)) {
		// Loop through all sources until the one which matches the 'monitor' is found.
		for (UINT PathIdx = 0; PathIdx < NumPathArrayElements; ++PathIdx) {
			DISPLAYCONFIG_SOURCE_DEVICE_NAME SourceName = {};
			SourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
			SourceName.header.size = sizeof(SourceName);
			SourceName.header.adapterId = PathInfoArray[PathIdx].sourceInfo.adapterId;
			SourceName.header.id = PathInfoArray[PathIdx].sourceInfo.id;

			hr = HRESULT_FROM_WIN32(DisplayConfigGetDeviceInfo(&SourceName.header));
			if (SUCCEEDED(hr)) {
				if (wcscmp(pszDeviceName, SourceName.viewGdiDeviceName) == 0) {
					// Found the source which matches this hmonitor. The paths are given in path-priority order
					// so the first found is the most desired, unless we later find an internal.
					if (DesiredPathIdx == -1 ||
					    IsInternalVideoOutput(PathInfoArray[PathIdx].targetInfo.outputTechnology)) {
						DesiredPathIdx = PathIdx;
					}
				}
			}
		}
	}

	if (DesiredPathIdx != -1) {
		*pPathInfo = PathInfoArray[DesiredPathIdx];
	} else {
		hr = E_INVALIDARG;
	}

	delete[] PathInfoArray;
	PathInfoArray = nullptr;

	delete[] ModeInfoArray;
	ModeInfoArray = nullptr;

	return hr;
}

// Overloaded function accepts an HMONITOR and converts to DeviceName
HRESULT D3D12DeviceInstance::GetPathInfo(HMONITOR hMonitor, _Out_ DISPLAYCONFIG_PATH_INFO *pPathInfo)
{
	HRESULT hr = S_OK;

	// Get the name of the 'monitor' being requested
	MONITORINFOEXW ViewInfo;
	RtlZeroMemory(&ViewInfo, sizeof(ViewInfo));
	ViewInfo.cbSize = sizeof(ViewInfo);
	if (!GetMonitorInfoW(hMonitor, &ViewInfo)) {
		// Error condition, likely invalid monitor handle, could log error
		hr = HRESULT_FROM_WIN32(GetLastError());
	}

	if (SUCCEEDED(hr)) {
		hr = GetPathInfo(ViewInfo.szDevice, pPathInfo);
	}

	return hr;
}

ULONG D3D12DeviceInstance::GetSdrMaxNits(HMONITOR monitor)
{
	ULONG nits = 80;

	DISPLAYCONFIG_PATH_INFO info;
	if (SUCCEEDED(GetPathInfo(monitor, &info))) {
		const DISPLAYCONFIG_PATH_TARGET_INFO &targetInfo = info.targetInfo;

		DISPLAYCONFIG_SDR_WHITE_LEVEL level;
		DISPLAYCONFIG_DEVICE_INFO_HEADER &header = level.header;
		header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SDR_WHITE_LEVEL;
		header.size = sizeof(level);
		header.adapterId = targetInfo.adapterId;
		header.id = targetInfo.id;
		if (DisplayConfigGetDeviceInfo(&header) == ERROR_SUCCESS)
			nits = (level.SDRWhiteLevel * 80) / 1000;
	}

	return nits;
}

MonitorColorInfo D3D12DeviceInstance::GetMonitorColorInfo(HMONITOR hMonitor)
{
	std::vector<std::pair<HMONITOR, MonitorColorInfo>> monitor_to_hdr;
	ComPtr<IDXGIFactory6> factory1;
	CreateDXGIFactory2(0, IID_PPV_ARGS(&factory1));

	ComPtr<IDXGIAdapter> adapter;
	ComPtr<IDXGIOutput> output;
	ComPtr<IDXGIOutput6> output6;
	for (UINT adapterIndex = 0; SUCCEEDED(factory1->EnumAdapters(adapterIndex, &adapter)); ++adapterIndex) {
		for (UINT outputIndex = 0; SUCCEEDED(adapter->EnumOutputs(outputIndex, &output)); ++outputIndex) {
			DXGI_OUTPUT_DESC1 desc1;
			if (SUCCEEDED(output->QueryInterface(&output6)) && SUCCEEDED(output6->GetDesc1(&desc1)) &&
			    (desc1.Monitor == hMonitor)) {
				const bool hdr = desc1.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020;
				const UINT bits = desc1.BitsPerColor;
				const ULONG nits = GetSdrMaxNits(desc1.Monitor);
				return monitor_to_hdr.emplace_back(hMonitor, MonitorColorInfo(hdr, bits, nits)).second;
			}
		}
	}

	return MonitorColorInfo(false, 8, 80);
}

void D3D12DeviceInstance::PopulateMonitorIds(HMONITOR handle, char *id, char *alt_id, size_t capacity)
{
	MONITORINFOEXA mi;
	mi.cbSize = sizeof(mi);
	if (GetMonitorInfoA(handle, (LPMONITORINFO)&mi)) {
		strcpy_s(alt_id, capacity, mi.szDevice);
		DISPLAY_DEVICEA device;
		device.cb = sizeof(device);
		if (EnumDisplayDevicesA(mi.szDevice, 0, &device, EDD_GET_DEVICE_INTERFACE_NAME)) {
			strcpy_s(id, capacity, device.DeviceID);
		}
	}
}

void D3D12DeviceInstance::LogAdapterMonitors(IDXGIAdapter1 *adapter)
{
	UINT i;
	ComPtr<IDXGIOutput> output;

	for (i = 0; adapter->EnumOutputs(i, &output) == S_OK; ++i) {
		DXGI_OUTPUT_DESC desc;
		if (FAILED(output->GetDesc(&desc)))
			continue;

		unsigned refresh = 0;

		bool target_found = false;
		DISPLAYCONFIG_TARGET_DEVICE_NAME target;

		constexpr size_t id_capacity = 128;
		char id[id_capacity]{};
		char alt_id[id_capacity]{};
		PopulateMonitorIds(desc.Monitor, id, alt_id, id_capacity);

		MONITORINFOEX info;
		info.cbSize = sizeof(info);
		if (GetMonitorInfo(desc.Monitor, &info)) {
			target_found = GetMonitorTarget(info, target);

			DEVMODE mode;
			mode.dmSize = sizeof(mode);
			mode.dmDriverExtra = 0;
			if (EnumDisplaySettings(info.szDevice, ENUM_CURRENT_SETTINGS, &mode)) {
				refresh = mode.dmDisplayFrequency;
			}
		}

		if (!target_found) {
			target.monitorFriendlyDeviceName[0] = 0;
		}

		UINT bits_per_color = 8;
		DXGI_COLOR_SPACE_TYPE type = DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709;
		FLOAT primaries[4][2]{};
		double gamut_size = 0.;
		FLOAT min_luminance = 0.f;
		FLOAT max_luminance = 0.f;
		FLOAT max_full_frame_luminance = 0.f;
		DXGI_OUTPUT_DESC1 desc1;
		if (GetOutputDesc1(output, &desc1)) {
			bits_per_color = desc1.BitsPerColor;
			type = desc1.ColorSpace;
			primaries[0][0] = desc1.RedPrimary[0];
			primaries[0][1] = desc1.RedPrimary[1];
			primaries[1][0] = desc1.GreenPrimary[0];
			primaries[1][1] = desc1.GreenPrimary[1];
			primaries[2][0] = desc1.BluePrimary[0];
			primaries[2][1] = desc1.BluePrimary[1];
			primaries[3][0] = desc1.WhitePoint[0];
			primaries[3][1] = desc1.WhitePoint[1];
			gamut_size = DoubleTriangleArea(desc1.RedPrimary[0], desc1.RedPrimary[1], desc1.GreenPrimary[0],
							desc1.GreenPrimary[1], desc1.BluePrimary[0],
							desc1.BluePrimary[1]);
			min_luminance = desc1.MinLuminance;
			max_luminance = desc1.MaxLuminance;
			max_full_frame_luminance = desc1.MaxFullFrameLuminance;
		}

		const char *space = "Unknown";
		switch (type) {
		case DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709:
			space = "RGB_FULL_G22_NONE_P709";
			break;
		case DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020:
			space = "RGB_FULL_G2084_NONE_P2020";
			break;
		default:
			blog(LOG_WARNING, "Unexpected DXGI_COLOR_SPACE_TYPE: %u", (unsigned)type);
		}

		// These are always identical, but you still have to supply both, thanks Microsoft!
		UINT dpiX, dpiY;
		unsigned scaling = 100;
		if (GetDpiForMonitor(desc.Monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY) == S_OK) {
			scaling = (unsigned)(dpiX * 100.0f / 96.0f);
		} else {
			dpiX = 0;
		}

		const RECT &rect = desc.DesktopCoordinates;
		const ULONG sdr_white_nits = GetSdrMaxNits(desc.Monitor);

		char *friendly_name;
		os_wcs_to_utf8_ptr(target.monitorFriendlyDeviceName, 0, &friendly_name);

		blog(LOG_INFO,
		     "\t  output %u:\n"
		     "\t    name=%s\n"
		     "\t    pos={%d, %d}\n"
		     "\t    size={%d, %d}\n"
		     "\t    attached=%s\n"
		     "\t    refresh=%u\n"
		     "\t    bits_per_color=%u\n"
		     "\t    space=%s\n"
		     "\t    primaries=[r=(%f, %f), g=(%f, %f), b=(%f, %f), wp=(%f, %f)]\n"
		     "\t    relative_gamut_area=[709=%f, P3=%f, 2020=%f]\n"
		     "\t    sdr_white_nits=%lu\n"
		     "\t    nit_range=[min=%f, max=%f, max_full_frame=%f]\n"
		     "\t    dpi=%u (%u%%)\n"
		     "\t    id=%s\n"
		     "\t    alt_id=%s",
		     i, friendly_name, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
		     desc.AttachedToDesktop ? "true" : "false", refresh, bits_per_color, space, primaries[0][0],
		     primaries[0][1], primaries[1][0], primaries[1][1], primaries[2][0], primaries[2][1],
		     primaries[3][0], primaries[3][1], gamut_size / DoubleTriangleArea(.64, .33, .3, .6, .15, .06),
		     gamut_size / DoubleTriangleArea(.68, .32, .265, .69, .15, .060),
		     gamut_size / DoubleTriangleArea(.708, .292, .17, .797, .131, .046), sdr_white_nits, min_luminance,
		     max_luminance, max_full_frame_luminance, dpiX, scaling, id, alt_id);
		bfree(friendly_name);
	}
}

void D3D12DeviceInstance::LogD3DAdapters()
{
	ComPtr<IDXGIFactory1> factory;
	ComPtr<IDXGIAdapter1> adapter;
	HRESULT hr;
	UINT i;

	blog(LOG_INFO, "Available Video Adapters: ");

	hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
		throw HRError("Failed to create DXGIFactory", hr);

	for (i = 0; factory->EnumAdapters1(i, adapter.Assign()) == S_OK; ++i) {
		DXGI_ADAPTER_DESC desc;
		char name[512] = "";

		hr = adapter->GetDesc(&desc);
		if (FAILED(hr))
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		os_wcs_to_utf8(desc.Description, 0, name, sizeof(name));
		blog(LOG_INFO, "\tAdapter %u: %s", i, name);
		blog(LOG_INFO, "\t  Dedicated VRAM: %" PRIu64 " (%.01f GiB)", desc.DedicatedVideoMemory,
		     to_GiB(desc.DedicatedVideoMemory));
		blog(LOG_INFO, "\t  Shared VRAM:    %" PRIu64 " (%.01f GiB)", desc.SharedSystemMemory,
		     to_GiB(desc.SharedSystemMemory));
		blog(LOG_INFO, "\t  PCI ID:         %x:%.4x", desc.VendorId, desc.DeviceId);

		if (auto hags_support = GetAdapterHagsStatus(&desc)) {
			blog(LOG_INFO, "\t  HAGS Status:    %s", hags_support->ToString().c_str());
		} else {
			blog(LOG_WARNING, "\t  HAGS Status:    Unknown");
		}

		/* driver version */
		LARGE_INTEGER umd;
		hr = adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd);
		if (SUCCEEDED(hr)) {
			const uint64_t version = umd.QuadPart;
			const uint16_t aa = (version >> 48) & 0xffff;
			const uint16_t bb = (version >> 32) & 0xffff;
			const uint16_t ccccc = (version >> 16) & 0xffff;
			const uint16_t ddddd = version & 0xffff;
			blog(LOG_INFO, "\t  Driver Version: %" PRIu16 ".%" PRIu16 ".%" PRIu16 ".%" PRIu16, aa, bb,
			     ccccc, ddddd);
		} else {
			blog(LOG_INFO, "\t  Driver Version: Unknown (0x%X)", (unsigned)hr);
		}

		LogAdapterMonitors(adapter);
	}
}

// Returns true if this is an integrated display panel e.g. the screen attached to tablets or laptops.
bool D3D12DeviceInstance::IsInternalVideoOutput(const DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY VideoOutputTechnologyType)
{
	switch (VideoOutputTechnologyType) {
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL:
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED:
	case DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED:
		return TRUE;

	default:
		return FALSE;
	}
}

void D3D12DeviceInstance::CreateD3DAdapterAndDevice(uint32_t index)
{
	SIZE_T MaxSize = 0;
	std::vector<ComPtr<IDXGIAdapter1>> adapters;
	ComPtr<IDXGIAdapter1> pAdapter;
	ComPtr<ID3D12Device> pDevice;
	for (uint32_t Idx = 0;
	     DXGI_ERROR_NOT_FOUND !=
	     m_DxgiFactory->EnumAdapterByGpuPreference(Idx, DXGI_GPU_PREFERENCE::DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
						       IID_PPV_ARGS(&pAdapter));
	     ++Idx) {
		DXGI_ADAPTER_DESC1 desc;
		HRESULT hr = pAdapter->GetDesc1(&desc);
		if (FAILED(hr))
			continue;
		// Is a software adapter?
		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			continue;

		/* ignore Microsoft's 'basic' renderer' */
		if (desc.VendorId == 0x1414 && desc.DeviceId == 0x8c)
			continue;

		// Can create a D3D12 device?
		if (FAILED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&pDevice))))
			continue;

		// Does support DXR if required?
		/* if (RequireDXRSupport && !IsDirectXRaytracingSupported(pDevice.Get()))
			continue;*/

		// By default, search for the adapter with the most memory because that's usually the dGPU.
		if (desc.DedicatedVideoMemory < MaxSize)
			continue;

		MaxSize = desc.DedicatedVideoMemory;

		if (m_Device != nullptr)
			m_Device->Release();

		adapters.push_back(pAdapter);
	}

	if (adapters.size() == 0) {
		throw HRError("No compatible D3D12 adapters found");
	}

	if (index >= adapters.size()) {
		throw HRError("Requested adapter index out of range");
	}

	m_Adapter = adapters[index];
	if (FAILED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device)))) {
		throw HRError("D3D12CreateDevice failed");
	}

	std::wstring adapterName;
	DXGI_ADAPTER_DESC desc;
	adapterName = (m_Adapter->GetDesc(&desc) == S_OK) ? desc.Description : L"<unknown>";

	BPtr<char> adapterNameUTF8;
	os_wcs_to_utf8_ptr(adapterName.c_str(), 0, &adapterNameUTF8);
	blog(LOG_INFO, "Loading up D3D12 on adapter %s (%" PRIu32 ")", adapterNameUTF8.Get(), index);

	LARGE_INTEGER umd;
	uint64_t driverVersion = 0;
	HRESULT hr = m_Adapter->CheckInterfaceSupport(__uuidof(IDXGIDevice), &umd);
	if (SUCCEEDED(hr))
		driverVersion = umd.QuadPart;

	/* Always true for non-NVIDIA GPUs */
	if (desc.VendorId != 0x10de)
		m_FastClearSupported = true;
	else {
		const uint16_t aa = (driverVersion >> 48) & 0xffff;
		const uint16_t bb = (driverVersion >> 32) & 0xffff;
		const uint16_t ccccc = (driverVersion >> 16) & 0xffff;
		const uint16_t ddddd = driverVersion & 0xffff;

		/* Check for NVIDIA driver version >= 31.0.15.2737 */
		m_FastClearSupported = aa >= 31 && bb >= 0 && ccccc >= 15 && ddddd >= 2737;
	}
}

const wchar_t *D3D12DeviceInstance::GPUVendorToString(uint32_t vendorID)
{
	switch (vendorID) {
	case vendorID_Nvidia:
		return L"Nvidia";
	case vendorID_AMD:
		return L"AMD";
	case vendorID_Intel:
		return L"Intel";
	default:
		return L"Unknown";
		break;
	}
}

uint32_t D3D12DeviceInstance::GetVendorIdFromDevice(ID3D12Device *pDevice)
{
	LUID luid = pDevice->GetAdapterLuid();

	ComPtr<IDXGIFactory4> dxgiFactory;
	HRESULT hr = CreateDXGIFactory2(0, IID_PPV_ARGS(&dxgiFactory));
	if (FAILED(hr)) {
		throw HRError("CreateDXGIFactory2 failed", hr);
	}

	ComPtr<IDXGIAdapter1> pAdapter;
	if (SUCCEEDED(dxgiFactory->EnumAdapterByLuid(luid, IID_PPV_ARGS(&pAdapter)))) {
		DXGI_ADAPTER_DESC1 desc;
		if (SUCCEEDED(pAdapter->GetDesc1(&desc))) {
			return desc.VendorId;
		}
	}

	return 0;
}

bool D3D12DeviceInstance::IsDeviceNvidia(ID3D12Device *pDevice)
{
	return GetVendorIdFromDevice(pDevice) == vendorID_Nvidia;
}

bool D3D12DeviceInstance::IsDeviceAMD(ID3D12Device *pDevice)
{
	return GetVendorIdFromDevice(pDevice) == vendorID_AMD;
}

bool D3D12DeviceInstance::IsDeviceIntel(ID3D12Device *pDevice)
{
	return GetVendorIdFromDevice(pDevice) == vendorID_Intel;
}

// Check adapter support for DirectX Raytracing.
bool D3D12DeviceInstance::IsDirectXRaytracingSupported(ID3D12Device *testDevice)
{
	D3D12_FEATURE_DATA_D3D12_OPTIONS5 featureSupport = {};

	if (FAILED(testDevice->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &featureSupport,
						   sizeof(featureSupport))))
		return false;

	return featureSupport.RaytracingTier != D3D12_RAYTRACING_TIER_NOT_SUPPORTED;
}

} // namespace D3D12Graphics
