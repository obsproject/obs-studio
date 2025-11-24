/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "d3d12-command-context.hpp"

#include <map>
#include <thread>

// Static members
static std::map<size_t, ComPtr<ID3D12RootSignature>> s_RootSignatureHashMap;
static std::vector<ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool2[2];
static std::queue<std::pair<uint64_t, ID3D12DescriptorHeap *>> sm_RetiredDescriptorHeaps2[2];
static std::queue<ID3D12DescriptorHeap *> sm_AvailableDescriptorHeaps2[2];
static std::vector<ComPtr<ID3D12DescriptorHeap>> sm_DescriptorHeapPool;
static LinearAllocatorPageManager sm_PageManager[2];

static RootSignature g_CommonRS;

static std::map<size_t, ComPtr<ID3D12PipelineState>> s_GraphicsPSOHashMap;
static std::map<size_t, ComPtr<ID3D12PipelineState>> s_ComputePSOHashMap;

static CommandSignature DispatchIndirectCommandSignature(1);
static CommandSignature DrawIndirectCommandSignature(1);

static ComputePSO g_GenerateMipsGammaPSO[4] = {
	{L"Generate Mips Gamma CS"},
	{L"Generate Mips Gamma Odd X CS"},
	{L"Generate Mips Gamma Odd Y CS"},
	{L"Generate Mips Gamma Odd CS"},
};

static ComputePSO g_GenerateMipsLinearPSO[4] = {
	{L"Generate Mips Linear CS"},
	{L"Generate Mips Linear Odd X CS"},
	{L"Generate Mips Linear Odd Y CS"},
	{L"Generate Mips Linear Odd CS"},
};

static ID3D12Device *g_Device = nullptr;
static CommandListManager g_CommandManager;
static ContextManager g_ContextManager;

static D3D_FEATURE_LEVEL g_D3DFeatureLevel = D3D_FEATURE_LEVEL_11_0;

static DescriptorAllocator g_DescriptorAllocator[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {
	D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
	D3D12_DESCRIPTOR_HEAP_TYPE_DSV};

inline D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE Type, UINT Count = 1)
{
	return g_DescriptorAllocator[Type].Allocate(Count);
}

void RootSignature::InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC &NonStaticSamplerDesc,
				      D3D12_SHADER_VISIBILITY Visibility)
{
	ASSERT(m_NumInitializedStaticSamplers < m_NumSamplers);
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
		WARN_ONCE_IF_NOT(
			// Transparent Black
			NonStaticSamplerDesc.BorderColor[0] == 0.0f && NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
					NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
					NonStaticSamplerDesc.BorderColor[3] == 0.0f ||
				// Opaque Black
				NonStaticSamplerDesc.BorderColor[0] == 0.0f &&
					NonStaticSamplerDesc.BorderColor[1] == 0.0f &&
					NonStaticSamplerDesc.BorderColor[2] == 0.0f &&
					NonStaticSamplerDesc.BorderColor[3] == 1.0f ||
				// Opaque White
				NonStaticSamplerDesc.BorderColor[0] == 1.0f &&
					NonStaticSamplerDesc.BorderColor[1] == 1.0f &&
					NonStaticSamplerDesc.BorderColor[2] == 1.0f &&
					NonStaticSamplerDesc.BorderColor[3] == 1.0f,
			"Sampler border color does not match static sampler limitations");

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

	ASSERT(m_NumInitializedStaticSamplers == m_NumSamplers);

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
			ASSERT(RootParam.DescriptorTable.pDescriptorRanges != nullptr);

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

	ID3D12RootSignature **RSRef = nullptr;
	bool firstCompile = false;
	{
		auto iter = s_RootSignatureHashMap.find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_RootSignatureHashMap.end()) {
			RSRef = s_RootSignatureHashMap[HashCode].GetAddressOf();
			firstCompile = true;
		} else
			RSRef = iter->second.GetAddressOf();
	}

	if (firstCompile) {
		ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

		ASSERT_SUCCEEDED(D3D12SerializeRootSignature(&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1,
							     pOutBlob.GetAddressOf(), pErrorBlob.GetAddressOf()));

		ASSERT_SUCCEEDED(g_Device->CreateRootSignature(1, pOutBlob->GetBufferPointer(),
							       pOutBlob->GetBufferSize(), IID_PPV_ARGS(&m_Signature)));

		m_Signature->SetName(name.c_str());

		s_RootSignatureHashMap[HashCode].Attach(m_Signature);
		ASSERT(*RSRef == m_Signature);
	} else {
		while (*RSRef == nullptr)
			std::this_thread::yield();
		m_Signature = *RSRef;
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
	if (RequiresRootSignature) {
		ASSERT(pRootSig != nullptr);
	} else {
		pRootSig = nullptr;
	}

	ASSERT_SUCCEEDED(g_Device->CreateCommandSignature(&CommandSignatureDesc, pRootSig, IID_PPV_ARGS(&m_Signature)));

	m_Signature->SetName(L"CommandSignature");

	m_Finalized = TRUE;
}

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type)
	: m_Type(Type),
	  m_CurrentHeap(nullptr),
	  m_DescriptorSize(0),
	  m_RemainingFreeHandles(0)
{
	m_CurrentHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocator::Allocate(uint32_t Count)
{
	if (m_CurrentHeap == nullptr || m_RemainingFreeHandles < Count) {
		m_CurrentHeap = RequestNewHeap(m_Type);
		m_CurrentHandle = m_CurrentHeap->GetCPUDescriptorHandleForHeapStart();
		m_RemainingFreeHandles = sm_NumDescriptorsPerHeap;

		if (m_DescriptorSize == 0)
			m_DescriptorSize = g_Device->GetDescriptorHandleIncrementSize(m_Type);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ret = m_CurrentHandle;
	m_CurrentHandle.ptr += Count * m_DescriptorSize;
	m_RemainingFreeHandles -= Count;
	return ret;
}

ID3D12DescriptorHeap *DescriptorAllocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc;
	Desc.Type = Type;
	Desc.NumDescriptors = sm_NumDescriptorsPerHeap;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Desc.NodeMask = 1;

	ComPtr<ID3D12DescriptorHeap> pHeap;
	ASSERT_SUCCEEDED(g_Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&pHeap)));
	sm_DescriptorHeapPool.emplace_back(pHeap);
	return pHeap.Get();
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

DescriptorHeap::DescriptorHeap(void) {}

DescriptorHeap::~DescriptorHeap(void) {}

void DescriptorHeap::Create(const std::wstring &DebugHeapName, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount)
{
	m_HeapDesc.Type = Type;
	m_HeapDesc.NumDescriptors = MaxCount;
	m_HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_HeapDesc.NodeMask = 1;

	ASSERT_SUCCEEDED(g_Device->CreateDescriptorHeap(&m_HeapDesc, IID_PPV_ARGS(m_Heap.Assign())));

	m_Heap->SetName(DebugHeapName.c_str());

	m_DescriptorSize = g_Device->GetDescriptorHandleIncrementSize(m_HeapDesc.Type);
	m_NumFreeDescriptors = m_HeapDesc.NumDescriptors;
	m_FirstHandle = DescriptorHandle(m_Heap->GetCPUDescriptorHandleForHeapStart(),
					 m_Heap->GetGPUDescriptorHandleForHeapStart());
	m_NextFreeHandle = m_FirstHandle;
}

void DescriptorHeap::Destroy(void)
{
	m_Heap = nullptr;
}

bool DescriptorHeap::HasAvailableSpace(uint32_t Count) const
{
	return Count <= m_NumFreeDescriptors;
}

DescriptorHandle DescriptorHeap::Alloc(uint32_t Count)
{
	ASSERT(HasAvailableSpace(Count), "Descriptor Heap out of space.  Increase heap size.");
	DescriptorHandle ret = m_NextFreeHandle;
	m_NextFreeHandle += Count * m_DescriptorSize;
	m_NumFreeDescriptors -= Count;
	return ret;
}

uint32_t DescriptorHeap::GetOffsetOfHandle(const DescriptorHandle &DHandle)
{
	return (uint32_t)(DHandle.GetCpuPtr() - m_FirstHandle.GetCpuPtr()) / m_DescriptorSize;
}

bool DescriptorHeap::ValidateHandle(const DescriptorHandle &DHandle) const
{
	if (DHandle.GetCpuPtr() < m_FirstHandle.GetCpuPtr() ||
	    DHandle.GetCpuPtr() >= m_FirstHandle.GetCpuPtr() + m_HeapDesc.NumDescriptors * m_DescriptorSize)
		return false;

	if (DHandle.GetGpuPtr() - m_FirstHandle.GetGpuPtr() != DHandle.GetCpuPtr() - m_FirstHandle.GetCpuPtr())
		return false;

	return true;
}

ID3D12DescriptorHeap *DescriptorHeap::GetHeapPointer() const
{
	return m_Heap.Get();
}

uint32_t DescriptorHeap::GetDescriptorSize(void) const
{
	return m_DescriptorSize;
}

DescriptorHandleCache::DescriptorHandleCache()
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
		ASSERT(TRUE == _BitScanReverse((unsigned long *)&MaxSetHandle,
					       m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
		       "Root entry marked as stale but has no stale descriptors");

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
		ASSERT(TRUE == _BitScanReverse((unsigned long *)&MaxSetHandle,
					       m_RootDescriptorTable[RootIndex].AssignedHandlesBitMap),
		       "Root entry marked as stale but has no stale descriptors");

		NeededSpace += MaxSetHandle + 1;
		TableSize[StaleParamCount] = MaxSetHandle + 1;

		++StaleParamCount;
	}

	ASSERT(StaleParamCount <= kMaxNumDescriptorTables, "We're only equipped to handle so many descriptor tables");

	m_StaleRootParamsBitMap = 0;

	static const uint32_t kMaxDescriptorsPerCopy = 16;
	UINT NumDestDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pDestDescriptorRangeStarts[kMaxDescriptorsPerCopy];
	UINT pDestDescriptorRangeSizes[kMaxDescriptorsPerCopy];

	UINT NumSrcDescriptorRanges = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE pSrcDescriptorRangeStarts[kMaxDescriptorsPerCopy];
	UINT pSrcDescriptorRangeSizes[kMaxDescriptorsPerCopy];

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
			if (NumSrcDescriptorRanges + DescriptorCount > kMaxDescriptorsPerCopy) {
				g_Device->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts,
							  pDestDescriptorRangeSizes, NumSrcDescriptorRanges,
							  pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, Type);

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

	g_Device->CopyDescriptors(NumDestDescriptorRanges, pDestDescriptorRangeStarts, pDestDescriptorRangeSizes,
				  NumSrcDescriptorRanges, pSrcDescriptorRangeStarts, pSrcDescriptorRangeSizes, Type);
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
	ASSERT(((1 << RootIndex) & m_RootDescriptorTablesBitMap) != 0,
	       "Root parameter is not a CBV_SRV_UAV descriptor table");
	ASSERT(Offset + NumHandles <= m_RootDescriptorTable[RootIndex].TableSize);

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

	ASSERT(RootSig.m_NumParameters <= 16, "Maybe we need to support something greater");

	m_StaleRootParamsBitMap = 0;
	m_RootDescriptorTablesBitMap = (Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? RootSig.m_SamplerTableBitMap
										   : RootSig.m_DescriptorTableBitMap);

	unsigned long TableParams = m_RootDescriptorTablesBitMap;
	unsigned long RootIndex;
	while (_BitScanForward(&RootIndex, TableParams)) {
		TableParams ^= (1 << RootIndex);

		UINT TableSize = RootSig.m_DescriptorTableSize[RootIndex];
		ASSERT(TableSize > 0);

		DescriptorTableCache &RootDescriptorTable = m_RootDescriptorTable[RootIndex];
		RootDescriptorTable.AssignedHandlesBitMap = 0;
		RootDescriptorTable.TableStart = m_HandleCache + CurrentOffset;
		RootDescriptorTable.TableSize = TableSize;

		CurrentOffset += TableSize;
	}

	m_MaxCachedDescriptors = CurrentOffset;

	ASSERT(m_MaxCachedDescriptors <= kMaxNumDescriptors, "Exceeded user-supplied maximum cache size");
}

DynamicDescriptorHeap::DynamicDescriptorHeap(CommandContext &OwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
	: m_OwningContext(OwningContext),
	  m_DescriptorType(HeapType)
{
	m_CurrentHeapPtr = nullptr;
	m_CurrentOffset = 0;
	m_DescriptorSize = g_Device->GetDescriptorHandleIncrementSize(HeapType);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap() {}

void DynamicDescriptorHeap::CleanupUsedHeaps(uint64_t fenceValue)
{
	RetireCurrentHeap();
	RetireUsedHeaps(fenceValue);
	m_GraphicsHandleCache.ClearCache();
	m_ComputeHandleCache.ClearCache();
}

void DynamicDescriptorHeap::SetGraphicsDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
							 const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_GraphicsHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
}

void DynamicDescriptorHeap::SetComputeDescriptorHandles(UINT RootIndex, UINT Offset, UINT NumHandles,
							const D3D12_CPU_DESCRIPTOR_HANDLE Handles[])
{
	m_ComputeHandleCache.StageDescriptorHandles(RootIndex, Offset, NumHandles, Handles);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::UploadDirect(D3D12_CPU_DESCRIPTOR_HANDLE Handles)
{
	if (!HasSpace(1)) {
		RetireCurrentHeap();
		UnbindAllValid();
	}

	m_OwningContext.SetDescriptorHeap(m_DescriptorType, GetHeapPointer());

	DescriptorHandle DestHandle = m_FirstDescriptor + m_CurrentOffset * m_DescriptorSize;
	m_CurrentOffset += 1;

	g_Device->CopyDescriptorsSimple(1, DestHandle, Handles, m_DescriptorType);

	return DestHandle;
}

void DynamicDescriptorHeap::ParseGraphicsRootSignature(const RootSignature &RootSig)
{
	m_GraphicsHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
}

void DynamicDescriptorHeap::ParseComputeRootSignature(const RootSignature &RootSig)
{
	m_ComputeHandleCache.ParseRootSignature(m_DescriptorType, RootSig);
}

void DynamicDescriptorHeap::CommitGraphicsRootDescriptorTables(ID3D12GraphicsCommandList *CmdList)
{
	if (m_GraphicsHandleCache.m_StaleRootParamsBitMap != 0)
		CopyAndBindStagedTables(m_GraphicsHandleCache, CmdList,
					&ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitComputeRootDescriptorTables(ID3D12GraphicsCommandList *CmdList)
{
	if (m_ComputeHandleCache.m_StaleRootParamsBitMap != 0)
		CopyAndBindStagedTables(m_ComputeHandleCache, CmdList,
					&ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

ID3D12DescriptorHeap *DynamicDescriptorHeap::RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType)
{
	uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;

	while (!sm_RetiredDescriptorHeaps2[idx].empty() &&
	       g_CommandManager.IsFenceComplete(sm_RetiredDescriptorHeaps2[idx].front().first)) {
		sm_AvailableDescriptorHeaps2[idx].push(sm_RetiredDescriptorHeaps2[idx].front().second);
		sm_RetiredDescriptorHeaps2[idx].pop();
	}

	if (!sm_AvailableDescriptorHeaps2[idx].empty()) {
		ID3D12DescriptorHeap *HeapPtr = sm_AvailableDescriptorHeaps2[idx].front();
		sm_AvailableDescriptorHeaps2[idx].pop();
		return HeapPtr;
	} else {
		D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
		HeapDesc.Type = HeapType;
		HeapDesc.NumDescriptors = kNumDescriptorsPerHeap;
		HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		HeapDesc.NodeMask = 1;
		ComPtr<ID3D12DescriptorHeap> HeapPtr;
		ASSERT_SUCCEEDED(g_Device->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&HeapPtr)));
		sm_DescriptorHeapPool2[idx].emplace_back(HeapPtr);
		return HeapPtr.Get();
	}
}

void DynamicDescriptorHeap::DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValue,
						   const std::vector<ID3D12DescriptorHeap *> &UsedHeaps)
{
	uint32_t idx = HeapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER ? 1 : 0;
	for (auto iter = UsedHeaps.begin(); iter != UsedHeaps.end(); ++iter)
		sm_RetiredDescriptorHeaps2[idx].push(std::make_pair(FenceValue, *iter));
}

bool DynamicDescriptorHeap::HasSpace(uint32_t Count)
{
	return (m_CurrentHeapPtr != nullptr && m_CurrentOffset + Count <= kNumDescriptorsPerHeap);
}

void DynamicDescriptorHeap::RetireCurrentHeap(void)
{
	if (m_CurrentOffset == 0) {
		ASSERT(m_CurrentHeapPtr == nullptr);
		return;
	}

	ASSERT(m_CurrentHeapPtr != nullptr);
	m_RetiredHeaps.push_back(m_CurrentHeapPtr);
	m_CurrentHeapPtr = nullptr;
	m_CurrentOffset = 0;
}

void DynamicDescriptorHeap::RetireUsedHeaps(uint64_t fenceValue)
{
	DiscardDescriptorHeaps(m_DescriptorType, fenceValue, m_RetiredHeaps);
	m_RetiredHeaps.clear();
}

ID3D12DescriptorHeap *DynamicDescriptorHeap::GetHeapPointer()
{
	if (m_CurrentHeapPtr == nullptr) {
		ASSERT(m_CurrentOffset == 0);
		m_CurrentHeapPtr = RequestDescriptorHeap(m_DescriptorType);
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
	m_GraphicsHandleCache.UnbindAllValid();
	m_ComputeHandleCache.UnbindAllValid();
}

ContextManager::ContextManager(void) {}

CommandContext *ContextManager::AllocateContext(D3D12_COMMAND_LIST_TYPE Type)
{
	auto &AvailableContexts = sm_AvailableContexts[Type];

	CommandContext *ret = nullptr;
	if (AvailableContexts.empty()) {
		ret = new CommandContext(Type);
		sm_ContextPool[Type].emplace_back(ret);
		ret->Initialize();
	} else {
		ret = AvailableContexts.front();
		AvailableContexts.pop();
		ret->Reset();
	}
	ASSERT(ret != nullptr);

	ASSERT(ret->m_Type == Type);

	return ret;
}

void ContextManager::FreeContext(CommandContext *UsedContext)
{
	ASSERT(UsedContext != nullptr);
	sm_AvailableContexts[UsedContext->m_Type].push(UsedContext);
}

void ContextManager::DestroyAllContexts()
{
	for (uint32_t i = 0; i < 4; ++i)
		sm_ContextPool[i].clear();
}

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

	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
							   D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
							   IID_PPV_ARGS(&m_pResource)));

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

EsramAllocator::EsramAllocator() {}

void EsramAllocator::PushStack() {}

void EsramAllocator::PopStack() {}

D3D12_GPU_VIRTUAL_ADDRESS EsramAllocator::Alloc(size_t size, size_t align, const std::wstring &bufferName)
{
	(size);
	(align);
	(bufferName);
	return D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

intptr_t EsramAllocator::SizeOfFreeSpace(void) const
{
	return 0;
}

GpuBuffer::GpuBuffer(void) : m_BufferSize(0), m_ElementCount(0), m_ElementSize(0)
{
	m_ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

GpuBuffer::~GpuBuffer()
{
	Destroy();
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

	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
							   m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	if (initialData)
		CommandContext::InitializeBuffer(*this, initialData, m_BufferSize);

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

	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
							   m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	CommandContext::InitializeBuffer(*this, srcData, srcOffset);

#ifdef RELEASE
	(name);
#else
	m_pResource->SetName(name.c_str());
#endif

	CreateDerivedViews();
}

// Create a buffer in ESRAM.  On Windows, ESRAM is not used.
void GpuBuffer::Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize, EsramAllocator &Allocator,
		       const void *initialData)
{
	(void)Allocator;
	Create(name, NumElements, ElementSize, initialData);
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

	ASSERT_SUCCEEDED(g_Device->CreatePlacedResource(pBackingHeap, HeapOffset, &ResourceDesc, m_UsageState, nullptr,
							IID_PPV_ARGS(&m_pResource)));

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

	if (initialData)
		CommandContext::InitializeBuffer(*this, initialData, m_BufferSize);

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
	ASSERT(Offset + Size <= m_BufferSize);

	Size = Math::AlignUp(Size, 16);

	D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
	CBVDesc.BufferLocation = m_GpuVirtualAddress + (size_t)Offset;
	CBVDesc.SizeInBytes = Size;

	D3D12_CPU_DESCRIPTOR_HANDLE hCBV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateConstantBufferView(&CBVDesc, hCBV);
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
	ASSERT(m_BufferSize != 0);

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
		m_SRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_UAV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
}

void ByteAddressBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

	if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_SRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

	if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_UAV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
}

IndirectArgsBuffer::IndirectArgsBuffer() {}

void StructuredBuffer::Destroy(void)
{
	m_CounterBuffer.Destroy();
	GpuBuffer::Destroy();
}

void StructuredBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = m_ElementCount;
	SRVDesc.Buffer.StructureByteStride = m_ElementSize;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_SRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
	UAVDesc.Buffer.CounterOffsetInBytes = 0;
	UAVDesc.Buffer.NumElements = m_ElementCount;
	UAVDesc.Buffer.StructureByteStride = m_ElementSize;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	m_CounterBuffer.Create(L"StructuredBuffer::Counter", 1, 4);

	if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_UAV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateUnorderedAccessView(m_pResource.Get(), m_CounterBuffer.GetResource(), &UAVDesc, m_UAV);
}

ByteAddressBuffer &StructuredBuffer::GetCounterBuffer(void)
{
	return m_CounterBuffer;
}

const D3D12_CPU_DESCRIPTOR_HANDLE &StructuredBuffer::GetCounterSRV(CommandContext &Context)
{
	Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	return m_CounterBuffer.GetSRV();
}

const D3D12_CPU_DESCRIPTOR_HANDLE &StructuredBuffer::GetCounterUAV(CommandContext &Context)
{
	Context.TransitionResource(m_CounterBuffer, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	return m_CounterBuffer.GetUAV();
}

TypedBuffer::TypedBuffer(DXGI_FORMAT Format) : m_DataFormat(Format) {}

void TypedBuffer::CreateDerivedViews(void)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	SRVDesc.Format = m_DataFormat;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.Buffer.NumElements = m_ElementCount;
	SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

	if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_SRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
	UAVDesc.Format = m_DataFormat;
	UAVDesc.Buffer.NumElements = m_ElementCount;
	UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_UAV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
}

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

	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
							   D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
							   IID_PPV_ARGS(&m_pResource)));

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
	Desc.Format = GetBaseFormat(Format);
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

	ASSERT(Resource != nullptr);
	D3D12_RESOURCE_DESC ResourceDesc = Resource->GetDesc();

	m_pResource.Attach(Resource);
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

	{
		CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
		ASSERT_SUCCEEDED(Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
								 D3D12_RESOURCE_STATE_COMMON, &ClearValue,
								 IID_PPV_ARGS(&m_pResource)));
	}

	m_UsageState = D3D12_RESOURCE_STATE_COMMON;
	m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;

#ifndef RELEASE
	m_pResource->SetName(Name.c_str());
#else
	(Name);
#endif
}

void PixelBuffer::CreateTextureResource(ID3D12Device *Device, const std::wstring &Name,
					const D3D12_RESOURCE_DESC &ResourceDesc, D3D12_CLEAR_VALUE ClearValue,
					EsramAllocator &Allocator)
{
	(Allocator);
	CreateTextureResource(Device, Name, ResourceDesc, ClearValue);
}

DXGI_FORMAT PixelBuffer::GetBaseFormat(DXGI_FORMAT Format)
{
	switch (Format) {
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
		return Format;
	}
}

DXGI_FORMAT PixelBuffer::GetUAVFormat(DXGI_FORMAT Format)
{
	switch (Format) {
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

#ifdef _DEBUG
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

		ASSERT(false, "Requested a UAV Format for a depth stencil Format.");
#endif

	default:
		return Format;
	}
}

DXGI_FORMAT PixelBuffer::GetDSVFormat(DXGI_FORMAT Format)
{
	switch (Format) {
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
		return Format;
	}
}

DXGI_FORMAT PixelBuffer::GetDepthFormat(DXGI_FORMAT Format)
{
	switch (Format) {
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

DXGI_FORMAT PixelBuffer::GetStencilFormat(DXGI_FORMAT Format)
{
	switch (Format) {
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

ColorBuffer::ColorBuffer(Color ClearColor)
	: m_ClearColor(ClearColor),
	  m_NumMipMaps(0),
	  m_FragmentCount(1),
	  m_SampleCount(1)
{
	m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	for (int i = 0; i < _countof(m_UAVHandle); ++i)
		m_UAVHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

void ColorBuffer::CreateFromSwapChain(const std::wstring &Name, ID3D12Resource *BaseResource)
{
	AssociateWithResource(g_Device, Name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

	//m_UAVHandle[0] = Graphics::AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	//Graphics::g_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, nullptr, m_UAVHandle[0]);

	m_RTVHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	g_Device->CreateRenderTargetView(m_pResource.Get(), nullptr, m_RTVHandle);
}

void ColorBuffer::Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
			 DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
	D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, NumMips, Format, Flags);

	ResourceDesc.SampleDesc.Count = m_FragmentCount;
	ResourceDesc.SampleDesc.Quality = 0;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	ClearValue.Color[0] = m_ClearColor.x;
	ClearValue.Color[1] = m_ClearColor.y;
	ClearValue.Color[2] = m_ClearColor.z;
	ClearValue.Color[3] = m_ClearColor.w;

	CreateTextureResource(g_Device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(g_Device, Format, 1, NumMips);
}

void ColorBuffer::Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
			 DXGI_FORMAT Format, EsramAllocator &Allocator)
{
	Create(Name, Width, Height, NumMips, Format);
}

void ColorBuffer::CreateArray(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
			      DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, ArrayCount, 1, Format, Flags);

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;
	ClearValue.Color[0] = m_ClearColor.x;
	ClearValue.Color[1] = m_ClearColor.y;
	ClearValue.Color[2] = m_ClearColor.z;
	ClearValue.Color[3] = m_ClearColor.w;

	CreateTextureResource(g_Device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(g_Device, Format, ArrayCount, 1);
}

void ColorBuffer::CreateArray(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
			      DXGI_FORMAT Format, EsramAllocator &Allocator)
{
	CreateArray(Name, Width, Height, ArrayCount, Format);
}

const D3D12_CPU_DESCRIPTOR_HANDLE &ColorBuffer::GetSRV(void) const
{
	return m_SRVHandle;
}

const D3D12_CPU_DESCRIPTOR_HANDLE &ColorBuffer::GetRTV(void) const
{
	return m_RTVHandle;
}

const D3D12_CPU_DESCRIPTOR_HANDLE &ColorBuffer::GetUAV(void) const
{
	return m_UAVHandle[0];
}

void ColorBuffer::SetClearColor(Color ClearColor)
{
	m_ClearColor = ClearColor;
}

void ColorBuffer::SetMsaaMode(uint32_t NumColorSamples, uint32_t NumCoverageSamples)
{
	ASSERT(NumCoverageSamples >= NumColorSamples);
	m_FragmentCount = NumColorSamples;
	m_SampleCount = NumCoverageSamples;
}

Color ColorBuffer::GetClearColor(void) const
{
	return m_ClearColor;
}

void ColorBuffer::GenerateMipMaps(CommandContext &BaseContext)
{
	if (m_NumMipMaps == 0)
		return;

	ComputeContext &Context = BaseContext.GetComputeContext();

	Context.SetRootSignature(g_CommonRS);

	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	Context.SetDynamicDescriptor(1, 0, m_SRVHandle);

	for (uint32_t TopMip = 0; TopMip < m_NumMipMaps;) {
		uint32_t SrcWidth = m_Width >> TopMip;
		uint32_t SrcHeight = m_Height >> TopMip;
		uint32_t DstWidth = SrcWidth >> 1;
		uint32_t DstHeight = SrcHeight >> 1;

		// Determine if the first downsample is more than 2:1.  This happens whenever
		// the source width or height is odd.
		uint32_t NonPowerOfTwo = (SrcWidth & 1) | (SrcHeight & 1) << 1;
		if (m_Format == DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
			Context.SetPipelineState(g_GenerateMipsGammaPSO[NonPowerOfTwo]);
		else
			Context.SetPipelineState(g_GenerateMipsLinearPSO[NonPowerOfTwo]);

		// We can downsample up to four times, but if the ratio between levels is not
		// exactly 2:1, we have to shift our blend weights, which gets complicated or
		// expensive.  Maybe we can update the code later to compute sample weights for
		// each successive downsample.  We use _BitScanForward to count number of zeros
		// in the low bits.  Zeros indicate we can divide by two without truncating.
		uint32_t AdditionalMips;
		_BitScanForward((unsigned long *)&AdditionalMips,
				(DstWidth == 1 ? DstHeight : DstWidth) | (DstHeight == 1 ? DstWidth : DstHeight));
		uint32_t NumMips = 1 + (AdditionalMips > 3 ? 3 : AdditionalMips);
		if (TopMip + NumMips > m_NumMipMaps)
			NumMips = m_NumMipMaps - TopMip;

		// These are clamped to 1 after computing additional mips because clamped
		// dimensions should not limit us from downsampling multiple times.  (E.g.
		// 16x1 -> 8x1 -> 4x1 -> 2x1 -> 1x1.)
		if (DstWidth == 0)
			DstWidth = 1;
		if (DstHeight == 0)
			DstHeight = 1;

		Context.SetConstants(0, TopMip, NumMips, 1.0f / DstWidth, 1.0f / DstHeight);
		Context.SetDynamicDescriptors(2, 0, NumMips, m_UAVHandle + TopMip + 1);
		Context.Dispatch2D(DstWidth, DstHeight);

		Context.InsertUAVBarrier(*this);

		TopMip += NumMips;
	}

	Context.TransitionResource(*this, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE |
						  D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE);
}

D3D12_RESOURCE_FLAGS ColorBuffer::CombineResourceFlags(void) const
{
	D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

	if (Flags == D3D12_RESOURCE_FLAG_NONE && m_FragmentCount == 1)
		Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
}

inline uint32_t ColorBuffer::ComputeNumMips(uint32_t Width, uint32_t Height)
{
	uint32_t HighBit;
	_BitScanReverse((unsigned long *)&HighBit, Width | Height);
	return HighBit + 1;
}

void ColorBuffer::CreateDerivedViews(ID3D12Device *Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
{
	ASSERT(ArraySize == 1 || NumMips == 1, "We don't support auto-mips on texture arrays");

	m_NumMipMaps = NumMips - 1;

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

	RTVDesc.Format = Format;
	UAVDesc.Format = GetUAVFormat(Format);
	SRVDesc.Format = Format;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (ArraySize > 1) {
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.MipSlice = 0;
		RTVDesc.Texture2DArray.FirstArraySlice = 0;
		RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
		UAVDesc.Texture2DArray.MipSlice = 0;
		UAVDesc.Texture2DArray.FirstArraySlice = 0;
		UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		SRVDesc.Texture2DArray.MipLevels = NumMips;
		SRVDesc.Texture2DArray.MostDetailedMip = 0;
		SRVDesc.Texture2DArray.FirstArraySlice = 0;
		SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
	} else if (m_FragmentCount > 1) {
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	} else {
		RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;

		UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		UAVDesc.Texture2D.MipSlice = 0;

		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = NumMips;
		SRVDesc.Texture2D.MostDetailedMip = 0;
	}

	if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_RTVHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_SRVHandle = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}

	ID3D12Resource *Resource = m_pResource.Get();

	// Create the render target view
	Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);

	// Create the shader resource view
	Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);

	if (m_FragmentCount > 1)
		return;

	// Create the UAVs for each mip level (RWTexture2D)
	for (uint32_t i = 0; i < NumMips; ++i) {
		if (m_UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_UAVHandle[i] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, m_UAVHandle[i]);

		UAVDesc.Texture2D.MipSlice++;
	}
}

DepthBuffer::DepthBuffer(float ClearDepth, uint8_t ClearStencil)
	: m_ClearDepth(ClearDepth),
	  m_ClearStencil(ClearStencil)
{
	m_hDSV[0].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDSV[1].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDSV[2].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDSV[3].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hDepthSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	m_hStencilSRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
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
	CreateTextureResource(g_Device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(g_Device, Format);
}

void DepthBuffer::Create(const std::wstring &Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
			 EsramAllocator &Allocator)
{
	Create(Name, Width, Height, 1, Format, Allocator);
}

void DepthBuffer::Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t Samples,
			 DXGI_FORMAT Format, EsramAllocator &)
{
	Create(Name, Width, Height, Samples, Format);
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

void DepthBuffer::CreateDerivedViews(ID3D12Device *Device, DXGI_FORMAT Format)
{
	ID3D12Resource *Resource = m_pResource.Get();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = GetDSVFormat(Format);
	if (Resource->GetDesc().SampleDesc.Count == 1) {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	} else {
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	if (m_hDSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
		m_hDSV[0] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		m_hDSV[1] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[1]);

	DXGI_FORMAT stencilReadFormat = GetStencilFormat(Format);
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN) {
		if (m_hDSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) {
			m_hDSV[2] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			m_hDSV[3] = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
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
		m_hDepthSRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = GetDepthFormat(Format);
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
			m_hStencilSRV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		SRVDesc.Format = stencilReadFormat;
		SRVDesc.Texture2D.PlaneSlice = 1;

		Device->CreateShaderResourceView(Resource, &SRVDesc, m_hStencilSRV);
	}
}

LinearAllocatorPageManager::LinearAllocatorPageManager()
{
	m_AllocationType = sm_AutoType;
	sm_AutoType = (LinearAllocatorType)(sm_AutoType + 1);
	ASSERT(sm_AutoType <= kNumAllocatorTypes);
}

LinearAllocationPage *LinearAllocatorPageManager::RequestPage(void)
{
	while (!m_RetiredPages.empty() && g_CommandManager.IsFenceComplete(m_RetiredPages.front().first)) {
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
	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc,
							   DefaultUsage, nullptr, IID_PPV_ARGS(&pBuffer)));

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
	while (!m_DeletionQueue.empty() && g_CommandManager.IsFenceComplete(m_DeletionQueue.front().first)) {
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

LinearAllocator::LinearAllocator(LinearAllocatorType Type)
	: m_AllocationType(Type),
	  m_PageSize(0),
	  m_CurOffset(~(size_t)0),
	  m_CurPage(nullptr)
{
	ASSERT(Type > kInvalidAllocator && Type < kNumAllocatorTypes);
	m_PageSize = (Type == kGpuExclusive ? kGpuAllocatorPageSize : kCpuAllocatorPageSize);
}

DynAlloc LinearAllocator::Allocate(size_t SizeInBytes, size_t Alignment)
{
	const size_t AlignmentMask = Alignment - 1;

	// Assert that it's a power of two.
	ASSERT((AlignmentMask & Alignment) == 0);

	// Align the allocation
	const size_t AlignedSize = Math::AlignUpWithMask(SizeInBytes, AlignmentMask);

	if (AlignedSize > m_PageSize)
		return AllocateLargePage(AlignedSize);

	m_CurOffset = Math::AlignUp(m_CurOffset, Alignment);

	if (m_CurOffset + AlignedSize > m_PageSize) {
		ASSERT(m_CurPage != nullptr);
		m_RetiredPages.push_back(m_CurPage);
		m_CurPage = nullptr;
	}

	if (m_CurPage == nullptr) {
		m_CurPage = sm_PageManager[m_AllocationType].RequestPage();
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

	sm_PageManager[m_AllocationType].DiscardPages(FenceID, m_RetiredPages);
	m_RetiredPages.clear();

	sm_PageManager[m_AllocationType].FreeLargePages(FenceID, m_LargePageList);
	m_LargePageList.clear();
}

DynAlloc LinearAllocator::AllocateLargePage(size_t SizeInBytes)
{
	LinearAllocationPage *OneOff = sm_PageManager[m_AllocationType].CreateNewPage(SizeInBytes);
	m_LargePageList.push_back(OneOff);

	DynAlloc ret(*OneOff, 0, SizeInBytes);
	ret.DataPtr = OneOff->m_CpuVirtualAddress;
	ret.GpuAddress = OneOff->m_GpuVirtualAddress;

	return ret;
}

GraphicsPSO::GraphicsPSO(const wchar_t *Name) : PSO(Name)
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
	ASSERT(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED, "Can't draw with undefined topology");
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
	ASSERT(NumRTVs == 0 || RTVFormats != nullptr, "Null format array conflicts with non-zero length");
	for (UINT i = 0; i < NumRTVs; ++i) {
		ASSERT(RTVFormats[i] != DXGI_FORMAT_UNKNOWN);
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
	ASSERT(m_PSODesc.pRootSignature != nullptr);

	m_PSODesc.InputLayout.pInputElementDescs = nullptr;
	size_t HashCode = Utility::HashState(&m_PSODesc);
	HashCode = Utility::HashState(m_InputLayouts.get(), m_PSODesc.InputLayout.NumElements, HashCode);
	m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.get();

	ID3D12PipelineState **PSORef = nullptr;
	bool firstCompile = false;
	{
		auto iter = s_GraphicsPSOHashMap.find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_GraphicsPSOHashMap.end()) {
			firstCompile = true;
			PSORef = s_GraphicsPSOHashMap[HashCode].GetAddressOf();
		} else
			PSORef = iter->second.GetAddressOf();
	}

	if (firstCompile) {
		ASSERT(m_PSODesc.DepthStencilState.DepthEnable != (m_PSODesc.DSVFormat == DXGI_FORMAT_UNKNOWN));
		ASSERT_SUCCEEDED(g_Device->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
		s_GraphicsPSOHashMap[HashCode].Attach(m_PSO);
		m_PSO->SetName(m_Name);
	} else {
		while (*PSORef == nullptr)
			std::this_thread::yield();
		m_PSO = *PSORef;
	}
}

ComputePSO::ComputePSO(const wchar_t *Name) : PSO(Name)
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
	ASSERT(m_PSODesc.pRootSignature != nullptr);

	size_t HashCode = Utility::HashState(&m_PSODesc);

	ID3D12PipelineState **PSORef = nullptr;
	bool firstCompile = false;
	{
		auto iter = s_ComputePSOHashMap.find(HashCode);

		// Reserve space so the next inquiry will find that someone got here first.
		if (iter == s_ComputePSOHashMap.end()) {
			firstCompile = true;
			PSORef = s_ComputePSOHashMap[HashCode].GetAddressOf();
		} else
			PSORef = iter->second.GetAddressOf();
	}

	if (firstCompile) {
		ASSERT_SUCCEEDED(g_Device->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
		s_ComputePSOHashMap[HashCode].Attach(m_PSO);
		m_PSO->SetName(m_Name);
	} else {
		while (*PSORef == nullptr)
			std::this_thread::yield();
		m_PSO = *PSORef;
	}
}

CommandAllocatorPool::CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type) : m_cCommandListType(Type), m_Device(nullptr)
{
}

CommandAllocatorPool::~CommandAllocatorPool()
{
	Shutdown();
}

void CommandAllocatorPool::Create(ID3D12Device *pDevice)
{
	m_Device = pDevice;
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
			ASSERT_SUCCEEDED(pAllocator->Reset());
			m_ReadyAllocators.pop();
		}
	}

	// If no allocator's were ready to be reused, create a new one
	if (pAllocator == nullptr) {
		ASSERT_SUCCEEDED(m_Device->CreateCommandAllocator(m_cCommandListType, IID_PPV_ARGS(&pAllocator)));
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
	  m_AllocatorPool(Type)
{
}

CommandQueue::~CommandQueue()
{
	Shutdown();
}

void CommandQueue::Create(ID3D12Device *pDevice)
{
	ASSERT(pDevice != nullptr);
	ASSERT(!IsReady());
	ASSERT(m_AllocatorPool.Size() == 0);

	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = m_Type;
	QueueDesc.NodeMask = 1;
	pDevice->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&m_CommandQueue));
	m_CommandQueue->SetName(L"CommandListManager::m_CommandQueue");

	ASSERT_SUCCEEDED(pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_pFence)));
	m_pFence->SetName(L"CommandListManager::m_pFence");
	m_pFence->Signal((uint64_t)m_Type << 56);

	m_FenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
	ASSERT(m_FenceEventHandle != NULL);

	m_AllocatorPool.Create(pDevice);

	ASSERT(IsReady());
}

void CommandQueue::Shutdown()
{
	if (m_CommandQueue == nullptr)
		return;

	m_AllocatorPool.Shutdown();

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
	CommandQueue &Producer = g_CommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	m_CommandQueue->Wait(Producer.m_pFence, FenceValue);
}

void CommandQueue::StallForProducer(CommandQueue &Producer)
{
	ASSERT(Producer.m_NextFenceValue > 0);
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
	ASSERT_SUCCEEDED(((ID3D12GraphicsCommandList *)List)->Close());

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

	return m_AllocatorPool.RequestAllocator(CompletedFence);
}

void CommandQueue::DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator *Allocator)
{
	m_AllocatorPool.DiscardAllocator(FenceValueForReset, Allocator);
}

CommandListManager::CommandListManager()
	: m_Device(nullptr),
	  m_GraphicsQueue(D3D12_COMMAND_LIST_TYPE_DIRECT),
	  m_ComputeQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE),
	  m_CopyQueue(D3D12_COMMAND_LIST_TYPE_COPY)
{
}

CommandListManager::~CommandListManager()
{
	Shutdown();
}

void CommandListManager::Create(ID3D12Device *pDevice)
{
	ASSERT(pDevice != nullptr);

	m_Device = pDevice;

	m_GraphicsQueue.Create(pDevice);
	m_ComputeQueue.Create(pDevice);
	m_CopyQueue.Create(pDevice);
}

void CommandListManager::Shutdown()
{
	m_GraphicsQueue.Shutdown();
	m_ComputeQueue.Shutdown();
	m_CopyQueue.Shutdown();
}

CommandQueue &CommandListManager::GetGraphicsQueue(void)
{
	return m_GraphicsQueue;
}

CommandQueue &CommandListManager::GetComputeQueue(void)
{
	return m_ComputeQueue;
}

CommandQueue &CommandListManager::GetCopyQueue(void)
{
	return m_CopyQueue;
}

CommandQueue &CommandListManager::GetQueue(D3D12_COMMAND_LIST_TYPE Type)
{
	switch (Type) {
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return m_ComputeQueue;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return m_CopyQueue;
	default:
		return m_GraphicsQueue;
	}
}

ID3D12CommandQueue *CommandListManager::GetCommandQueue()
{
	return m_GraphicsQueue.GetCommandQueue();
}

void CommandListManager::CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList **List,
					      ID3D12CommandAllocator **Allocator)
{
	ASSERT(Type != D3D12_COMMAND_LIST_TYPE_BUNDLE, "Bundles are not yet supported");
	switch (Type) {
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		*Allocator = m_GraphicsQueue.RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_BUNDLE:
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		*Allocator = m_ComputeQueue.RequestAllocator();
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		*Allocator = m_CopyQueue.RequestAllocator();
		break;
	}

	ASSERT_SUCCEEDED(m_Device->CreateCommandList(1, Type, *Allocator, nullptr, IID_PPV_ARGS(List)));
	(*List)->SetName(L"CommandList");
}

bool CommandListManager::IsFenceComplete(uint64_t FenceValue)
{
	return GetQueue(D3D12_COMMAND_LIST_TYPE(FenceValue >> 56)).IsFenceComplete(FenceValue);
}

void CommandListManager::WaitForFence(uint64_t FenceValue)
{
	CommandQueue &Producer = g_CommandManager.GetQueue((D3D12_COMMAND_LIST_TYPE)(FenceValue >> 56));
	Producer.WaitForFence(FenceValue);
}

void CommandListManager::IdleGPU(void)
{
	m_GraphicsQueue.WaitForIdle();
	m_ComputeQueue.WaitForIdle();
	m_CopyQueue.WaitForIdle();
}

CommandContext::CommandContext(D3D12_COMMAND_LIST_TYPE Type)
	: m_Type(Type),
	  m_DynamicViewDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV),
	  m_DynamicSamplerDescriptorHeap(*this, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER),
	  m_CpuLinearAllocator(kCpuWritable),
	  m_GpuLinearAllocator(kGpuExclusive)
{
	m_OwningManager = nullptr;
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
{ // We only call Reset() on previously freed contexts.  The command list persists, but we must
	// request a new allocator.
	ASSERT(m_CommandList != nullptr && m_CurrentAllocator == nullptr);
	m_CurrentAllocator = g_CommandManager.GetQueue(m_Type).RequestAllocator();
	m_CommandList->Reset(m_CurrentAllocator, nullptr);

	m_CurGraphicsRootSignature = nullptr;
	m_CurComputeRootSignature = nullptr;
	m_CurPipelineState = nullptr;
	m_NumBarriersToFlush = 0;

	BindDescriptorHeaps();
}

CommandContext &CommandContext::Begin(const std::wstring ID)
{
	CommandContext *NewContext = g_ContextManager.AllocateContext(D3D12_COMMAND_LIST_TYPE_DIRECT);
	NewContext->SetID(ID);
	if (ID.length() > 0)
		EngineProfiling::GetInstace()->BeginBlock(ID, NewContext);
	return *NewContext;
}

uint64_t CommandContext::Flush(bool WaitForCompletion)
{
	FlushResourceBarriers();

	ASSERT(m_CurrentAllocator != nullptr);

	uint64_t FenceValue = g_CommandManager.GetQueue(m_Type).ExecuteCommandList(m_CommandList);

	if (WaitForCompletion)
		g_CommandManager.WaitForFence(FenceValue);

	//
	// Reset the command list and restore previous state
	//

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
	ASSERT(m_Type == D3D12_COMMAND_LIST_TYPE_DIRECT || m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE);

	FlushResourceBarriers();

	if (m_ID.length() > 0)
		EngineProfiling::GetInstace()->EndBlock(this);

	ASSERT(m_CurrentAllocator != nullptr);

	CommandQueue &Queue = g_CommandManager.GetQueue(m_Type);

	uint64_t FenceValue = Queue.ExecuteCommandList(m_CommandList);
	Queue.DiscardAllocator(FenceValue, m_CurrentAllocator);
	m_CurrentAllocator = nullptr;

	m_CpuLinearAllocator.CleanupUsedPages(FenceValue);
	m_GpuLinearAllocator.CleanupUsedPages(FenceValue);
	m_DynamicViewDescriptorHeap.CleanupUsedHeaps(FenceValue);
	m_DynamicSamplerDescriptorHeap.CleanupUsedHeaps(FenceValue);

	if (WaitForCompletion)
		g_CommandManager.WaitForFence(FenceValue);

	g_ContextManager.FreeContext(this);

	return FenceValue;
}

void CommandContext::Initialize(void)
{
	g_CommandManager.CreateNewCommandList(m_Type, &m_CommandList, &m_CurrentAllocator);
}

GraphicsContext &CommandContext::GetGraphicsContext()
{
	ASSERT(m_Type != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
	return reinterpret_cast<GraphicsContext &>(*this);
}

ComputeContext &CommandContext::GetComputeContext()
{
	return reinterpret_cast<ComputeContext &>(*this);
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

void CommandContext::CopyCounter(GpuResource &Dest, size_t DestOffset, StructuredBuffer &Src)
{
	TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionResource(Src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
	FlushResourceBarriers();
	m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetCounterBuffer().GetResource(), 0, 4);
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

void CommandContext::ResetCounter(StructuredBuffer &Buf, uint32_t Value)
{
	FillBuffer(Buf.GetCounterBuffer(), 0, Value, sizeof(uint32_t));
	TransitionResource(Buf.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

uint32_t CommandContext::ReadbackTexture(ReadbackBuffer &DstBuffer, PixelBuffer &SrcBuffer)
{
	uint64_t CopySize = 0;

	// The footprint may depend on the device of the resource, but we assume there is only one device.
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT PlacedFootprint;

	auto resourceDesc = SrcBuffer.GetResource()->GetDesc();
	g_Device->GetCopyableFootprints(&resourceDesc, 0, 1, 0, &PlacedFootprint, nullptr, nullptr, &CopySize);

	DstBuffer.Create(L"Readback", (uint32_t)CopySize, 1);

	TransitionResource(SrcBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, true);

	auto DescLocation = CD3DX12_TEXTURE_COPY_LOCATION(DstBuffer.GetResource(), PlacedFootprint);
	auto SrcLocation = CD3DX12_TEXTURE_COPY_LOCATION(SrcBuffer.GetResource(), 0);
	m_CommandList->CopyTextureRegion(&DescLocation, 0, 0, 0, &SrcLocation, nullptr);

	return PlacedFootprint.Footprint.RowPitch;
}

DynAlloc CommandContext::ReserveUploadMemory(size_t SizeInBytes)
{
	return m_CpuLinearAllocator.Allocate(SizeInBytes);
}

void CommandContext::InitializeTexture(GpuResource &Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
	UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.GetResource(), 0, NumSubresources);

	CommandContext &InitContext = CommandContext::Begin();

	// copy data to the intermediate upload heap and then schedule a copy from the upload heap to the default texture
	DynAlloc mem = InitContext.ReserveUploadMemory(uploadBufferSize);
	UpdateSubresources(InitContext.m_CommandList, Dest.GetResource(), mem.Buffer.GetResource(), 0, 0,
			   NumSubresources, SubData);
	InitContext.TransitionResource(Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

	// Execute the command list and wait for it to finish so we can release the upload buffer
	InitContext.Finish(true);
}

void CommandContext::InitializeBuffer(GpuBuffer &Dest, const void *BufferData, size_t NumBytes, size_t DestOffset)
{
	CommandContext &InitContext = CommandContext::Begin();

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

void CommandContext::InitializeBuffer(GpuBuffer &Dest, const UploadBuffer &Src, size_t SrcOffset, size_t NumBytes,
				      size_t DestOffset)
{
	CommandContext &InitContext = CommandContext::Begin();

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

void CommandContext::InitializeTextureArraySlice(GpuResource &Dest, UINT SliceIndex, GpuResource &Src)
{
	CommandContext &Context = CommandContext::Begin();

	Context.TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
	Context.FlushResourceBarriers();

	const D3D12_RESOURCE_DESC &DestDesc = Dest.GetResource()->GetDesc();
	const D3D12_RESOURCE_DESC &SrcDesc = Src.GetResource()->GetDesc();

	ASSERT(SliceIndex < DestDesc.DepthOrArraySize && SrcDesc.DepthOrArraySize == 1 &&
	       DestDesc.Width == SrcDesc.Width && DestDesc.Height == SrcDesc.Height &&
	       DestDesc.MipLevels <= SrcDesc.MipLevels);

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

void CommandContext::WriteBuffer(GpuResource &Dest, size_t DestOffset, const void *BufferData, size_t NumBytes)
{
	ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
	DynAlloc TempSpace = m_CpuLinearAllocator.Allocate(NumBytes, 512);
	SIMDMemCopy(TempSpace.DataPtr, BufferData, Math::DivideByMultiple(NumBytes, 16));
	CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes);
}

void CommandContext::FillBuffer(GpuResource &Dest, size_t DestOffset, DWParam Value, size_t NumBytes)
{
	DynAlloc TempSpace = m_CpuLinearAllocator.Allocate(NumBytes, 512);
	__m128 VectorValue = _mm_set1_ps(Value.Float);
	SIMDMemFill(TempSpace.DataPtr, VectorValue, Math::DivideByMultiple(NumBytes, 16));
	CopyBufferRegion(Dest, DestOffset, TempSpace.Buffer, TempSpace.Offset, NumBytes);
}

void CommandContext::TransitionResource(GpuResource &Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
	D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

	if (m_Type == D3D12_COMMAND_LIST_TYPE_COMPUTE) {
		ASSERT((OldState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == OldState);
		ASSERT((NewState & VALID_COMPUTE_QUEUE_RESOURCE_STATES) == NewState);
	}

	if (OldState != NewState) {
		ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
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

	if (FlushImmediate || m_NumBarriersToFlush == 16)
		FlushResourceBarriers();
}

void CommandContext::BeginResourceTransition(GpuResource &Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate)
{
	// If it's already transitioning, finish that transition
	if (Resource.m_TransitioningState != (D3D12_RESOURCE_STATES)-1)
		TransitionResource(Resource, Resource.m_TransitioningState);

	D3D12_RESOURCE_STATES OldState = Resource.m_UsageState;

	if (OldState != NewState) {
		ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
		D3D12_RESOURCE_BARRIER &BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Transition.pResource = Resource.GetResource();
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = OldState;
		BarrierDesc.Transition.StateAfter = NewState;

		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_BEGIN_ONLY;

		Resource.m_TransitioningState = NewState;
	}

	if (FlushImmediate || m_NumBarriersToFlush == 16)
		FlushResourceBarriers();
}

void CommandContext::InsertUAVBarrier(GpuResource &Resource, bool FlushImmediate)
{
	ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
	D3D12_RESOURCE_BARRIER &BarrierDesc = m_ResourceBarrierBuffer[m_NumBarriersToFlush++];

	BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	BarrierDesc.UAV.pResource = Resource.GetResource();

	if (FlushImmediate)
		FlushResourceBarriers();
}

void CommandContext::InsertAliasBarrier(GpuResource &Before, GpuResource &After, bool FlushImmediate)
{
	ASSERT(m_NumBarriersToFlush < 16, "Exceeded arbitrary limit on buffered barriers");
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

GraphicsContext &GraphicsContext::Begin(const std::wstring &ID)
{
	return CommandContext::Begin(ID).GetGraphicsContext();
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

void GraphicsContext::ClearUAV(ColorBuffer &Target)
{
	FlushResourceBarriers();

	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
	CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

	//TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
	const float *ClearColor = Target.GetClearColor().ptr;
	m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(),
						     ClearColor, 1, &ClearRect);
}

void GraphicsContext::ClearColor(ColorBuffer &Target, D3D12_RECT *Rect)
{
	FlushResourceBarriers();
	m_CommandList->ClearRenderTargetView(Target.GetRTV(), Target.GetClearColor().ptr, (Rect == nullptr) ? 0 : 1,
					     Rect);
}

void GraphicsContext::ClearColor(ColorBuffer &Target, float Colour[4], D3D12_RECT *Rect)
{
	FlushResourceBarriers();
	m_CommandList->ClearRenderTargetView(Target.GetRTV(), Colour, (Rect == nullptr) ? 0 : 1, Rect);
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

void GraphicsContext::SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[],
				       D3D12_CPU_DESCRIPTOR_HANDLE DSV)
{
	m_CommandList->OMSetRenderTargets(NumRTVs, RTVs, FALSE, &DSV);
}

void GraphicsContext::SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV)
{
	SetRenderTargets(1, &RTV);
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
	ASSERT(rect.left < rect.right && rect.top < rect.bottom);
	m_CommandList->RSSetScissorRects(1, &rect);
}

void GraphicsContext::SetScissor(UINT left, UINT top, UINT right, UINT bottom)
{
	SetScissor(CD3DX12_RECT(left, top, right, bottom));
}

void GraphicsContext::SetViewportAndScissor(const D3D12_VIEWPORT &vp, const D3D12_RECT &rect)
{
	ASSERT(rect.left < rect.right && rect.top < rect.bottom);
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

void GraphicsContext::SetConstant(UINT RootIndex, UINT Offset, DWParam Val)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Val.Uint, Offset);
}

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
}

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
}

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
}

void GraphicsContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
{
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Y.Uint, 1);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, Z.Uint, 2);
	m_CommandList->SetGraphicsRoot32BitConstant(RootIndex, W.Uint, 3);
}

void GraphicsContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
	m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
}

void GraphicsContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
	DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	// memcpy(cb.DataPtr, BufferData, BufferSize);
	m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

void GraphicsContext::SetBufferSRV(UINT RootIndex, const GpuBuffer &SRV, UINT64 Offset)
{
	ASSERT((SRV.m_UsageState &
		(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
	m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

void GraphicsContext::SetBufferUAV(UINT RootIndex, const GpuBuffer &UAV, UINT64 Offset)
{
	ASSERT((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
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
	ASSERT(VertexData != nullptr && Math::IsAligned(VertexData, 16));

	size_t BufferSize = Math::AlignUp(NumVertices * VertexStride, 16);
	DynAlloc vb = m_CpuLinearAllocator.Allocate(BufferSize);

	SIMDMemCopy(vb.DataPtr, VertexData, BufferSize >> 4);

	D3D12_VERTEX_BUFFER_VIEW VBView;
	VBView.BufferLocation = vb.GpuAddress;
	VBView.SizeInBytes = (UINT)BufferSize;
	VBView.StrideInBytes = (UINT)VertexStride;

	m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
}

void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t *IndexData)
{
	ASSERT(IndexData != nullptr && Math::IsAligned(IndexData, 16));

	size_t BufferSize = Math::AlignUp(IndexCount * sizeof(uint16_t), 16);
	DynAlloc ib = m_CpuLinearAllocator.Allocate(BufferSize);

	SIMDMemCopy(ib.DataPtr, IndexData, BufferSize >> 4);

	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = ib.GpuAddress;
	IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
	IBView.Format = DXGI_FORMAT_R16_UINT;

	m_CommandList->IASetIndexBuffer(&IBView);
}

void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
	DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
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
	ExecuteIndirect(DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
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

ComputeContext &ComputeContext::Begin(const std::wstring &ID, bool Async)
{
	ComputeContext &NewContext =
		g_ContextManager
			.AllocateContext(Async ? D3D12_COMMAND_LIST_TYPE_COMPUTE : D3D12_COMMAND_LIST_TYPE_DIRECT)
			->GetComputeContext();
	NewContext.SetID(ID);
	if (ID.length() > 0)
		EngineProfiling::GetInstace()->BeginBlock(ID, &NewContext);
	return NewContext;
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

void ComputeContext::ClearUAV(ColorBuffer &Target)
{
	FlushResourceBarriers();

	// After binding a UAV, we can get a GPU handle that is required to clear it as a UAV (because it essentially runs
	// a shader to set all of the values).
	D3D12_GPU_DESCRIPTOR_HANDLE GpuVisibleHandle = m_DynamicViewDescriptorHeap.UploadDirect(Target.GetUAV());
	CD3DX12_RECT ClearRect(0, 0, (LONG)Target.GetWidth(), (LONG)Target.GetHeight());

	//TODO: My Nvidia card is not clearing UAVs with either Float or Uint variants.
	const float *ClearColor = Target.GetClearColor().ptr;
	m_CommandList->ClearUnorderedAccessViewFloat(GpuVisibleHandle, Target.GetUAV(), Target.GetResource(),
						     ClearColor, 1, &ClearRect);
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

void ComputeContext::SetConstant(UINT RootIndex, UINT Offset, DWParam Val)
{
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, Val.Uint, Offset);
}

void ComputeContext::SetConstants(UINT RootIndex, DWParam X)
{
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
}

void ComputeContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y)
{
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
}

void ComputeContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z)
{
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
}

void ComputeContext::SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W)
{
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, X.Uint, 0);
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, Y.Uint, 1);
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, Z.Uint, 2);
	m_CommandList->SetComputeRoot32BitConstant(RootIndex, W.Uint, 3);
}

void ComputeContext::SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV)
{
	m_CommandList->SetComputeRootConstantBufferView(RootIndex, CBV);
}

void ComputeContext::SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
	DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	//SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	memcpy(cb.DataPtr, BufferData, BufferSize);
	m_CommandList->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress);
}

void ComputeContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
	DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
	SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
	m_CommandList->SetComputeRootShaderResourceView(RootIndex, cb.GpuAddress);
}

void ComputeContext::SetBufferSRV(UINT RootIndex, const GpuBuffer &SRV, UINT64 Offset)
{
	ASSERT((SRV.m_UsageState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
	m_CommandList->SetComputeRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

void ComputeContext::SetBufferUAV(UINT RootIndex, const GpuBuffer &UAV, UINT64 Offset)
{
	ASSERT((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
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
	ExecuteIndirect(DispatchIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
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

GpuTimeManager *GpuTimeManager::GetInstance()
{
	static GpuTimeManager *instance = new GpuTimeManager();
	return instance;
}

void GpuTimeManager::Initialize(uint32_t MaxNumTimers)
{
	uint64_t GpuFrequency;
	g_CommandManager.GetCommandQueue()->GetTimestampFrequency(&GpuFrequency);
	m_GpuTickDelta = 1.0 / static_cast<double>(GpuFrequency);

	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_READBACK;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC BufferDesc;
	BufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	BufferDesc.Alignment = 0;
	BufferDesc.Width = sizeof(uint64_t) * MaxNumTimers * 2;
	BufferDesc.Height = 1;
	BufferDesc.DepthOrArraySize = 1;
	BufferDesc.MipLevels = 1;
	BufferDesc.Format = DXGI_FORMAT_UNKNOWN;
	BufferDesc.SampleDesc.Count = 1;
	BufferDesc.SampleDesc.Quality = 0;
	BufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	BufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ASSERT_SUCCEEDED(g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &BufferDesc,
							   D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
							   IID_PPV_ARGS(&m_ReadBackBuffer)));
	m_ReadBackBuffer->SetName(L"GpuTimeStamp Buffer");

	D3D12_QUERY_HEAP_DESC QueryHeapDesc;
	QueryHeapDesc.Count = MaxNumTimers * 2;
	QueryHeapDesc.NodeMask = 1;
	QueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	ASSERT_SUCCEEDED(g_Device->CreateQueryHeap(&QueryHeapDesc, IID_PPV_ARGS(&m_QueryHeap)));
	m_QueryHeap->SetName(L"GpuTimeStamp QueryHeap");

	m_MaxNumTimers = (uint32_t)MaxNumTimers;
}

void GpuTimeManager::Shutdown()
{
	if (m_ReadBackBuffer != nullptr)
		m_ReadBackBuffer->Release();

	if (m_QueryHeap != nullptr)
		m_QueryHeap->Release();
}

uint32_t GpuTimeManager::NewTimer(void)
{
	return m_NumTimers++;
}

void GpuTimeManager::StartTimer(CommandContext &Context, uint32_t TimerIdx)
{
	Context.InsertTimeStamp(m_QueryHeap, TimerIdx * 2);
}

void GpuTimeManager::StopTimer(CommandContext &Context, uint32_t TimerIdx)
{
	Context.InsertTimeStamp(m_QueryHeap, TimerIdx * 2 + 1);
}

void GpuTimeManager::BeginReadBack(void)
{
	g_CommandManager.WaitForFence(m_Fence);

	D3D12_RANGE Range;
	Range.Begin = 0;
	Range.End = (m_NumTimers * 2) * sizeof(uint64_t);
	ASSERT_SUCCEEDED(m_ReadBackBuffer->Map(0, &Range, reinterpret_cast<void **>(&m_TimeStampBuffer)));

	m_ValidTimeStart = m_TimeStampBuffer[0];
	m_ValidTimeEnd = m_TimeStampBuffer[1];

	// On the first frame, with random values in the timestamp query heap, we can avoid a misstart.
	if (m_ValidTimeEnd < m_ValidTimeStart) {
		m_ValidTimeStart = 0ull;
		m_ValidTimeEnd = 0ull;
	}
}
void GpuTimeManager::EndReadBack(void)
{
	// Unmap with an empty range to indicate nothing was written by the CPU
	D3D12_RANGE EmptyRange = {};
	m_ReadBackBuffer->Unmap(0, &EmptyRange);
	m_TimeStampBuffer = nullptr;

	CommandContext &Context = CommandContext::Begin();
	Context.InsertTimeStamp(m_QueryHeap, 1);
	Context.ResolveTimeStamps(m_ReadBackBuffer, m_QueryHeap, m_NumTimers * 2);
	Context.InsertTimeStamp(m_QueryHeap, 0);
	m_Fence = Context.Finish();
}

float GpuTimeManager::GetTime(uint32_t TimerIdx)
{
	ASSERT(m_TimeStampBuffer != nullptr, "Time stamp readback buffer is not mapped");
	ASSERT(TimerIdx < m_NumTimers, "Invalid GPU timer index");

	uint64_t TimeStamp1 = m_TimeStampBuffer[TimerIdx * 2];
	uint64_t TimeStamp2 = m_TimeStampBuffer[TimerIdx * 2 + 1];

	if (TimeStamp1 < m_ValidTimeStart || TimeStamp2 > m_ValidTimeEnd || TimeStamp2 <= TimeStamp1)
		return 0.0f;

	return static_cast<float>(m_GpuTickDelta * (TimeStamp2 - TimeStamp1));
}

GpuTimer::GpuTimer()
{
	m_TimerIndex = GpuTimeManager::GetInstance()->NewTimer();
}

void GpuTimer::Start(CommandContext &Context)
{
	GpuTimeManager::GetInstance()->StartTimer(Context, m_TimerIndex);
}

void GpuTimer::Stop(CommandContext &Context)
{
	GpuTimeManager::GetInstance()->StopTimer(Context, m_TimerIndex);
}

float GpuTimer::GetTime(void)
{
	return GpuTimeManager::GetInstance()->GetTime(m_TimerIndex);
}

uint32_t GpuTimer::GetTimerIndex(void)
{
	return m_TimerIndex;
}

SystemTime *SystemTime::GetInstance()
{
	SystemTime *instance = new SystemTime();
	return instance;
}

SystemTime::SystemTime()
{
	LARGE_INTEGER frequency;
	ASSERT(TRUE == QueryPerformanceFrequency(&frequency), "Unable to query performance counter frequency");
	m_CpuTickDelta = 1.0 / static_cast<double>(frequency.QuadPart);
}

int64_t SystemTime::GetCurrentTick(void)
{
	LARGE_INTEGER currentTick;
	ASSERT(TRUE == QueryPerformanceCounter(&currentTick), "Unable to query performance counter value");
	return static_cast<int64_t>(currentTick.QuadPart);
}

void SystemTime::BusyLoopSleep(float SleepTime)
{
	int64_t finalTick = (int64_t)((double)SleepTime / m_CpuTickDelta) + GetCurrentTick();
	while (GetCurrentTick() < finalTick)
		;
}

double SystemTime::TicksToSeconds(int64_t TickCount)
{
	return TickCount * m_CpuTickDelta;
}

double SystemTime::TicksToMillisecs(int64_t TickCount)
{
	return TickCount * m_CpuTickDelta * 1000.0;
}

double SystemTime::TimeBetweenTicks(int64_t tick1, int64_t tick2)
{
	return TicksToSeconds(tick2 - tick1);
}

CpuTimer::CpuTimer()
{
	m_StartTick = 0ll;
	m_ElapsedTicks = 0ll;
}

void CpuTimer::Start()
{
	if (m_StartTick == 0ll)
		m_StartTick = SystemTime::GetInstance()->GetCurrentTick();
}
void CpuTimer::Stop()
{
	if (m_StartTick != 0ll) {
		m_ElapsedTicks += SystemTime::GetInstance()->GetCurrentTick() - m_StartTick;
		m_StartTick = 0ll;
	}
}

void CpuTimer::Reset()
{
	m_ElapsedTicks = 0ll;
	m_StartTick = 0ll;
}

double CpuTimer::GetTime() const
{
	return SystemTime::GetInstance()->TicksToSeconds(m_ElapsedTicks);
}

EngineProfiling *EngineProfiling::GetInstace()
{
	static EngineProfiling *instance = new EngineProfiling();
	return instance;
}

void EngineProfiling::Update() {}

void EngineProfiling::BeginBlock(const std::wstring &name, CommandContext *Context) {}

void EngineProfiling::EndBlock(CommandContext *Context) {}

void EngineProfiling::DisplayFrameRate()
{
	/*float cpuTime = NestedTimingTree::GetTotalCpuTime();
	float gpuTime = NestedTimingTree::GetTotalGpuTime();
	float frameRate = 1.0f / NestedTimingTree::GetFrameDelta();*/
}
