#pragma once

#include <util/windows/win-version.h>

#include <vector>
#include <string>
#include <memory>
#include <queue>
#include <unordered_map>

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <util/windows/ComPtr.hpp>
#include <util/windows/HRError.hpp>
#include <d3dx12.h>

#include <graphics/matrix4.h>
#include <graphics/graphics.h>

#include <Hash.h>

using Color = struct vec4;

#define PERF_GRAPH_ERROR uint32_t(0xFFFFFFFF)
#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

#define DEFAULT_ALIGN 256

static const uint32_t kHistorySize = 64;
static const uint32_t kExtendedHistorySize = 256;

typedef uint32_t GraphHandle;

#define ASSERT(isTrue, ...) (void)(isTrue)
#define WARN_ONCE_IF(isTrue, ...) (void)(isTrue)
#define WARN_ONCE_IF_NOT(isTrue, ...) (void)(isTrue)
#define DEBUGPRINT(msg, ...) do {} while(0)
#define ASSERT_SUCCEEDED(hr, ...) (void)(hr)

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

class RootParameter;
class RootSignature;
class IndirectParameter;
class CommandSignature;
class DescriptorAllocator;
class DescriptorHandle;
class DescriptorHeap;
class DescriptorHandleCache;
class DynamicDescriptorHeap;
class ContextManager;
class GpuResource;
class CommandContext;
class GraphicsContext;
class ComputeContext;
class GpuTimeManager;
class Texture;
class SystemTime;
class CpuTimer;

enum LinearAllocatorType {
	kInvalidAllocator = -1,

	kGpuExclusive = 0, // DEFAULT   GPU-writeable (via UAV)
	kCpuWritable = 1,  // UPLOAD CPU-writeable (but write combined)

	kNumAllocatorTypes
};

enum {
	kGpuAllocatorPageSize = 0x10000, // 64K
	kCpuAllocatorPageSize = 0x200000 // 2MB
};

static const uint32_t kNumDescriptorsPerHeap = 1024;
static const uint32_t kMaxNumDescriptors = 256;
static const uint32_t kMaxNumDescriptorTables = 16;
static const uint32_t sm_NumDescriptorsPerHeap = 256;
static LinearAllocatorType sm_AutoType;

class RootParameter {
public:
	RootParameter() { m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF; }

	~RootParameter() { Clear(); }

	void Clear()
	{
		if (m_RootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			delete[] m_RootParam.DescriptorTable.pDescriptorRanges;

		m_RootParam.ParameterType = (D3D12_ROOT_PARAMETER_TYPE)0xFFFFFFFF;
	}

	void InitAsConstants(UINT Register, UINT NumDwords,
			     D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
	{
		m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		m_RootParam.ShaderVisibility = Visibility;
		m_RootParam.Constants.Num32BitValues = NumDwords;
		m_RootParam.Constants.ShaderRegister = Register;
		m_RootParam.Constants.RegisterSpace = Space;
	}

	void InitAsConstantBuffer(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL,
				  UINT Space = 0)
	{
		m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
		m_RootParam.ShaderVisibility = Visibility;
		m_RootParam.Descriptor.ShaderRegister = Register;
		m_RootParam.Descriptor.RegisterSpace = Space;
	}

	void InitAsBufferSRV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL,
			     UINT Space = 0)
	{
		m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
		m_RootParam.ShaderVisibility = Visibility;
		m_RootParam.Descriptor.ShaderRegister = Register;
		m_RootParam.Descriptor.RegisterSpace = Space;
	}

	void InitAsBufferUAV(UINT Register, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL,
			     UINT Space = 0)
	{
		m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
		m_RootParam.ShaderVisibility = Visibility;
		m_RootParam.Descriptor.ShaderRegister = Register;
		m_RootParam.Descriptor.RegisterSpace = Space;
	}

	void InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count,
				   D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL, UINT Space = 0)
	{
		InitAsDescriptorTable(1, Visibility);
		SetTableRange(0, Type, Register, Count, Space);
	}

	void InitAsDescriptorTable(UINT RangeCount, D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
	{
		m_RootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		m_RootParam.ShaderVisibility = Visibility;
		m_RootParam.DescriptorTable.NumDescriptorRanges = RangeCount;
		m_RootParam.DescriptorTable.pDescriptorRanges = new D3D12_DESCRIPTOR_RANGE[RangeCount];
	}

	void SetTableRange(UINT RangeIndex, D3D12_DESCRIPTOR_RANGE_TYPE Type, UINT Register, UINT Count, UINT Space = 0)
	{
		D3D12_DESCRIPTOR_RANGE *range = const_cast<D3D12_DESCRIPTOR_RANGE *>(
			m_RootParam.DescriptorTable.pDescriptorRanges + RangeIndex);
		range->RangeType = Type;
		range->NumDescriptors = Count;
		range->BaseShaderRegister = Register;
		range->RegisterSpace = Space;
		range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
	}

	const D3D12_ROOT_PARAMETER &operator()(void) const { return m_RootParam; }

protected:
	D3D12_ROOT_PARAMETER m_RootParam;
};

// Maximum 64 DWORDS divied up amongst all root parameters.
// Root constants = 1 DWORD * NumConstants
// Root descriptor (CBV, SRV, or UAV) = 2 DWORDs each
// Descriptor table pointer = 1 DWORD
// Static samplers = 0 DWORDS (compiled into shader)
class RootSignature {
public:
	RootSignature(UINT NumRootParams = 0, UINT NumStaticSamplers = 0)
		: m_Finalized(FALSE),
		  m_NumParameters(NumRootParams)
	{
		Reset(NumRootParams, NumStaticSamplers);
	}

	~RootSignature() {}

	void Reset(UINT NumRootParams, UINT NumStaticSamplers = 0)
	{
		if (NumRootParams > 0)
			m_ParamArray.reset(new RootParameter[NumRootParams]);
		else
			m_ParamArray = nullptr;
		m_NumParameters = NumRootParams;

		if (NumStaticSamplers > 0)
			m_SamplerArray.reset(new D3D12_STATIC_SAMPLER_DESC[NumStaticSamplers]);
		else
			m_SamplerArray = nullptr;
		m_NumSamplers = NumStaticSamplers;
		m_NumInitializedStaticSamplers = 0;
	}

	RootParameter &operator[](size_t EntryIndex)
	{
		ASSERT(EntryIndex < m_NumParameters);
		return m_ParamArray.get()[EntryIndex];
	}

	const RootParameter &operator[](size_t EntryIndex) const
	{
		ASSERT(EntryIndex < m_NumParameters);
		return m_ParamArray.get()[EntryIndex];
	}

	void InitStaticSampler(UINT Register, const D3D12_SAMPLER_DESC &NonStaticSamplerDesc,
			       D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL);

	void Finalize(const std::wstring &name, D3D12_ROOT_SIGNATURE_FLAGS Flags = D3D12_ROOT_SIGNATURE_FLAG_NONE);

	ID3D12RootSignature *GetSignature() const;

	BOOL m_Finalized;
	UINT m_NumParameters;
	UINT m_NumSamplers;
	UINT m_NumInitializedStaticSamplers;
	uint32_t m_DescriptorTableBitMap;   // One bit is set for root parameters that are non-sampler descriptor tables
	uint32_t m_SamplerTableBitMap;      // One bit is set for root parameters that are sampler descriptor tables
	uint32_t m_DescriptorTableSize[16]; // Non-sampler descriptor tables need to know their descriptor count
	std::unique_ptr<RootParameter[]> m_ParamArray;
	std::unique_ptr<D3D12_STATIC_SAMPLER_DESC[]> m_SamplerArray;
	ID3D12RootSignature *m_Signature;
};


class IndirectParameter {
public:
	IndirectParameter() { m_IndirectParam.Type = (D3D12_INDIRECT_ARGUMENT_TYPE)0xFFFFFFFF; }

	void Draw(void) { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW; }

	void DrawIndexed(void) { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED; }

	void Dispatch(void) { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH; }

	void VertexBufferView(UINT Slot)
	{
		m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;
		m_IndirectParam.VertexBuffer.Slot = Slot;
	}

	void IndexBufferView(void) { m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW; }

	void Constant(UINT RootParameterIndex, UINT DestOffsetIn32BitValues, UINT Num32BitValuesToSet)
	{
		m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		m_IndirectParam.Constant.RootParameterIndex = RootParameterIndex;
		m_IndirectParam.Constant.DestOffsetIn32BitValues = DestOffsetIn32BitValues;
		m_IndirectParam.Constant.Num32BitValuesToSet = Num32BitValuesToSet;
	}

	void ConstantBufferView(UINT RootParameterIndex)
	{
		m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW;
		m_IndirectParam.ConstantBufferView.RootParameterIndex = RootParameterIndex;
	}

	void ShaderResourceView(UINT RootParameterIndex)
	{
		m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW;
		m_IndirectParam.ShaderResourceView.RootParameterIndex = RootParameterIndex;
	}

	void UnorderedAccessView(UINT RootParameterIndex)
	{
		m_IndirectParam.Type = D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW;
		m_IndirectParam.UnorderedAccessView.RootParameterIndex = RootParameterIndex;
	}

	const D3D12_INDIRECT_ARGUMENT_DESC &GetDesc(void) const { return m_IndirectParam; }

protected:
	D3D12_INDIRECT_ARGUMENT_DESC m_IndirectParam;
};

class CommandSignature {
public:
	CommandSignature(UINT NumParams = 0) : m_Finalized(FALSE), m_NumParameters(NumParams) { Reset(NumParams); }

	void Destroy(void)
	{
		m_Signature = nullptr;
		m_ParamArray = nullptr;
	}

	void Reset(UINT NumParams)
	{
		if (NumParams > 0)
			m_ParamArray.reset(new IndirectParameter[NumParams]);
		else
			m_ParamArray = nullptr;

		m_NumParameters = NumParams;
	}

	IndirectParameter &operator[](size_t EntryIndex)
	{
		ASSERT(EntryIndex < m_NumParameters);
		return m_ParamArray.get()[EntryIndex];
	}

	const IndirectParameter &operator[](size_t EntryIndex) const
	{
		ASSERT(EntryIndex < m_NumParameters);
		return m_ParamArray.get()[EntryIndex];
	}

	ID3D12CommandSignature *GetSignature() const;

	void Finalize(const RootSignature *RootSignature = nullptr);

protected:
	BOOL m_Finalized;
	UINT m_NumParameters;
	std::unique_ptr<IndirectParameter[]> m_ParamArray;
	ComPtr<ID3D12CommandSignature> m_Signature;
};

class DescriptorAllocator {
public:
	DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type);
	D3D12_CPU_DESCRIPTOR_HANDLE Allocate(uint32_t Count);

	static ID3D12DescriptorHeap *RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type);

protected:
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	ID3D12DescriptorHeap *m_CurrentHeap;
	D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentHandle;
	uint32_t m_DescriptorSize;
	uint32_t m_RemainingFreeHandles;
};

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

class DescriptorHeap {
public:
	DescriptorHeap(void);
	~DescriptorHeap(void);

	void Create(const std::wstring &DebugHeapName, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t MaxCount);
	void Destroy(void);

	bool HasAvailableSpace(uint32_t Count) const;
	DescriptorHandle Alloc(uint32_t Count = 1);
	uint32_t GetOffsetOfHandle(const DescriptorHandle &DHandle);
	bool ValidateHandle(const DescriptorHandle &DHandle) const;
	ID3D12DescriptorHeap *GetHeapPointer() const;
	uint32_t GetDescriptorSize(void) const;

	DescriptorHandle operator[](uint32_t arrayIdx) const { return m_FirstHandle + arrayIdx * m_DescriptorSize; }


private:
	ComPtr<ID3D12DescriptorHeap> m_Heap;
	D3D12_DESCRIPTOR_HEAP_DESC m_HeapDesc;
	uint32_t m_DescriptorSize;
	uint32_t m_NumFreeDescriptors;
	DescriptorHandle m_FirstHandle;
	DescriptorHandle m_NextFreeHandle;
};

struct DescriptorTableCache {
	DescriptorTableCache() : AssignedHandlesBitMap(0), TableStart(nullptr), TableSize(0) {}
	uint32_t AssignedHandlesBitMap = 0;
	D3D12_CPU_DESCRIPTOR_HANDLE *TableStart = nullptr;
	uint32_t TableSize = 0;
};

class DescriptorHandleCache {
public:
	DescriptorHandleCache();
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

	DescriptorTableCache m_RootDescriptorTable[kMaxNumDescriptorTables];
	D3D12_CPU_DESCRIPTOR_HANDLE m_HandleCache[kMaxNumDescriptors];
	uint32_t m_RootDescriptorTablesBitMap;
	uint32_t m_StaleRootParamsBitMap;
	uint32_t m_MaxCachedDescriptors;
};

class DynamicDescriptorHeap {
public:
	DynamicDescriptorHeap(CommandContext &OwningContext, D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
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


	// Static methods
	static ID3D12DescriptorHeap *RequestDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
	static void DiscardDescriptorHeaps(D3D12_DESCRIPTOR_HEAP_TYPE HeapType, uint64_t FenceValueForReset,
					   const std::vector<ID3D12DescriptorHeap *> &UsedHeaps);

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

	CommandContext &m_OwningContext;
	ID3D12DescriptorHeap *m_CurrentHeapPtr;
	const D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorType;
	uint32_t m_DescriptorSize;
	uint32_t m_CurrentOffset;
	DescriptorHandle m_FirstDescriptor;
	std::vector<ID3D12DescriptorHeap *> m_RetiredHeaps;

	DescriptorHandleCache m_GraphicsHandleCache;
	DescriptorHandleCache m_ComputeHandleCache;
};

class ContextManager {
public:
	ContextManager(void);

	CommandContext *AllocateContext(D3D12_COMMAND_LIST_TYPE Type);
	void FreeContext(CommandContext *);
	void DestroyAllContexts();

private:
	std::vector<std::unique_ptr<CommandContext>> sm_ContextPool[4];
	std::queue<CommandContext *> sm_AvailableContexts[4];
};

struct NonCopyable {
	NonCopyable() = default;
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable &operator=(const NonCopyable &) = delete;
};

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
	virtual ~UploadBuffer();

	void Create(const std::wstring &name, size_t BufferSize);

	void *Map(void);
	void Unmap(size_t begin = 0, size_t end = -1);

	size_t GetBufferSize() const;

protected:
	size_t m_BufferSize;
};

class Texture : public GpuResource {
public:
	Texture();
	Texture(D3D12_CPU_DESCRIPTOR_HANDLE Handle);

	// Create a 1-level textures
	void Create2D(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void *InitData);
	void CreateCube(size_t RowPitchBytes, size_t Width, size_t Height, DXGI_FORMAT Format, const void *InitialData);

	void CreateTGAFromMemory(const void *memBuffer, size_t fileSize, bool sRGB);
	bool CreateDDSFromMemory(const void *memBuffer, size_t fileSize, bool sRGB);
	void CreatePIXImageFromMemory(const void *memBuffer, size_t fileSize);

	virtual void Destroy() override;

	const D3D12_CPU_DESCRIPTOR_HANDLE &GetSRV() const;

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	uint32_t GetDepth() const;

protected:
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;

	D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
};


class EsramAllocator {
public:
	EsramAllocator();

	void PushStack();
	void PopStack();

	D3D12_GPU_VIRTUAL_ADDRESS Alloc(size_t size, size_t align, const std::wstring &bufferName);
	intptr_t SizeOfFreeSpace(void) const;
};

class GpuBuffer : public GpuResource {
public:
	GpuBuffer(void);
	virtual ~GpuBuffer();

	// Create a buffer.  If initial data is provided, it will be copied into the buffer using the default command context.
	void Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize,
		    const void *initialData = nullptr);

	void Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize, const UploadBuffer &srcData,
		    uint32_t srcOffset = 0);

	// Create a buffer in ESRAM.  On Windows, ESRAM is not used.
	void Create(const std::wstring &name, uint32_t NumElements, uint32_t ElementSize, EsramAllocator &Allocator,
		    const void *initialData = nullptr);

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
	D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;

	size_t m_BufferSize;
	uint32_t m_ElementCount;
	uint32_t m_ElementSize;
	D3D12_RESOURCE_FLAGS m_ResourceFlags;
};


class ByteAddressBuffer : public GpuBuffer {
public:
	virtual void CreateDerivedViews(void) override;
};

class IndirectArgsBuffer : public ByteAddressBuffer {
public:
	IndirectArgsBuffer(void);
};

class StructuredBuffer : public GpuBuffer {
public:
	virtual void Destroy(void) override;

	virtual void CreateDerivedViews(void) override;

	ByteAddressBuffer &GetCounterBuffer(void);

	const D3D12_CPU_DESCRIPTOR_HANDLE &GetCounterSRV(CommandContext &Context);
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetCounterUAV(CommandContext &Context);

private:
	ByteAddressBuffer m_CounterBuffer;
};

class TypedBuffer : public GpuBuffer {
public:
	TypedBuffer(DXGI_FORMAT Format);
	virtual void CreateDerivedViews(void) override;

protected:
	DXGI_FORMAT m_DataFormat;
};

class ReadbackBuffer : public GpuBuffer {
public:
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

	void CreateTextureResource(ID3D12Device *Device, const std::wstring &Name,
				   const D3D12_RESOURCE_DESC &ResourceDesc, D3D12_CLEAR_VALUE ClearValue,
				   EsramAllocator &Allocator);

	static DXGI_FORMAT GetBaseFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT GetUAVFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT GetDSVFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT GetDepthFormat(DXGI_FORMAT Format);
	static DXGI_FORMAT GetStencilFormat(DXGI_FORMAT Format);
	static size_t BytesPerPixel(DXGI_FORMAT Format);

protected:
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_ArraySize;
	DXGI_FORMAT m_Format;
	uint32_t m_BankRotation;
};

class ColorBuffer : public PixelBuffer {
public:
	ColorBuffer(Color ClearColor = {0.0f, 0.0f, 0.0f, 0.0f});

	// Create a color buffer from a swap chain buffer.  Unordered access is restricted.
	void CreateFromSwapChain(const std::wstring &Name, ID3D12Resource *BaseResource);

	// Create a color buffer.  If an address is supplied, memory will not be allocated.
	// The vmem address allows you to alias buffers (which can be especially useful for
	// reusing ESRAM across a frame.)
	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format,
		    D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// Create a color buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
	// this functions the same as Create() without a video address.
	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format,
		    EsramAllocator &Allocator);

	// Create a color buffer.  If an address is supplied, memory will not be allocated.
	// The vmem address allows you to alias buffers (which can be especially useful for
	// reusing ESRAM across a frame.)
	void CreateArray(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
			 DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// Create a color buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
	// this functions the same as Create() without a video address.
	void CreateArray(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
			 DXGI_FORMAT Format, EsramAllocator &Allocator);

	// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetSRV(void) const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetRTV(void) const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetUAV(void) const;

	void SetClearColor(Color ClearColor);

	void SetMsaaMode(uint32_t NumColorSamples, uint32_t NumCoverageSamples);

	Color GetClearColor(void) const;

	// This will work for all texture sizes, but it's recommended for speed and quality
	// that you use dimensions with powers of two (but not necessarily square.)  Pass
	// 0 for ArrayCount to reserve space for mips at creation time.
	void GenerateMipMaps(CommandContext &Context);

	D3D12_RESOURCE_FLAGS CombineResourceFlags(void) const;
	// Compute the number of texture levels needed to reduce to 1x1.  This uses
	// _BitScanReverse to find the highest set bit.  Each dimension reduces by
	// half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
	// as the dimension 511 (0x1FF).
	static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height);

	void CreateDerivedViews(ID3D12Device *Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

protected:
	Color m_ClearColor;
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
	uint32_t m_NumMipMaps; // number of texture sublevels
	uint32_t m_FragmentCount;
	uint32_t m_SampleCount;
};

class DepthBuffer : public PixelBuffer {
public:
	DepthBuffer(float ClearDepth = 0.0f, uint8_t ClearStencil = 0);
	// Create a depth buffer.  If an address is supplied, memory will not be allocated.
	// The vmem address allows you to alias buffers (which can be especially useful for
	// reusing ESRAM across a frame.)
	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
		    D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// Create a depth buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
	// this functions the same as Create() without a video address.
	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format,
		    EsramAllocator &Allocator);

	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
		    D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
	void Create(const std::wstring &Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format,
		    EsramAllocator &Allocator);

	// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV_DepthReadOnly() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV_StencilReadOnly() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDSV_ReadOnly() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetDepthSRV() const;
	const D3D12_CPU_DESCRIPTOR_HANDLE &GetStencilSRV() const;

	float GetClearDepth() const;
	uint8_t GetClearStencil() const;

	void CreateDerivedViews(ID3D12Device *Device, DXGI_FORMAT Format);

protected:
	float m_ClearDepth;
	uint8_t m_ClearStencil;
	D3D12_CPU_DESCRIPTOR_HANDLE m_hDSV[4];
	D3D12_CPU_DESCRIPTOR_HANDLE m_hDepthSRV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_hStencilSRV;
};


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
	LinearAllocatorPageManager();
	LinearAllocationPage *RequestPage(void);
	LinearAllocationPage *CreateNewPage(size_t PageSize = 0);

	// Discarded pages will get recycled.  This is for fixed size pages.
	void DiscardPages(uint64_t FenceID, const std::vector<LinearAllocationPage *> &Pages);

	// Freed pages will be destroyed once their fence has passed.  This is for single-use,
	// "large" pages.
	void FreeLargePages(uint64_t FenceID, const std::vector<LinearAllocationPage *> &Pages);

	void Destroy(void);

private:
	LinearAllocatorType m_AllocationType;
	std::vector<std::unique_ptr<LinearAllocationPage>> m_PagePool;
	std::queue<std::pair<uint64_t, LinearAllocationPage *>> m_RetiredPages;
	std::queue<std::pair<uint64_t, LinearAllocationPage *>> m_DeletionQueue;
	std::queue<LinearAllocationPage *> m_AvailablePages;
};

class LinearAllocator {
public:
	LinearAllocator(LinearAllocatorType Type);
	DynAlloc Allocate(size_t SizeInBytes, size_t Alignment = DEFAULT_ALIGN);
	void CleanupUsedPages(uint64_t FenceID);
	DynAlloc AllocateLargePage(size_t SizeInBytes);

private:
	LinearAllocatorType m_AllocationType;
	size_t m_PageSize;
	size_t m_CurOffset;
	LinearAllocationPage *m_CurPage;
	std::vector<LinearAllocationPage *> m_RetiredPages;
	std::vector<LinearAllocationPage *> m_LargePageList;
};

struct DWParam {
	DWParam(FLOAT f) : Float(f) {}
	DWParam(UINT u) : Uint(u) {}
	DWParam(INT i) : Int(i) {}

	void operator=(FLOAT f) { Float = f; }
	void operator=(UINT u) { Uint = u; }
	void operator=(INT i) { Int = i; }

	union {
		FLOAT Float;
		UINT Uint;
		INT Int;
	};
};


class PSO {
public:
	PSO(const wchar_t *Name) : m_Name(Name), m_RootSignature(nullptr), m_PSO(nullptr) {}

	void SetRootSignature(const RootSignature &BindMappings) { m_RootSignature = &BindMappings; }

	const RootSignature &GetRootSignature(void) const
	{
		ASSERT(m_RootSignature != nullptr);
		return *m_RootSignature;
	}

	ID3D12PipelineState *GetPipelineStateObject(void) const { return m_PSO; }

protected:
	const wchar_t *m_Name;

	const RootSignature *m_RootSignature;

	ID3D12PipelineState *m_PSO;
};

class GraphicsPSO : public PSO {
	friend class CommandContext;

public:
	// Start with empty state
	GraphicsPSO(const wchar_t *Name = L"Unnamed Graphics PSO");

	void SetBlendState(const D3D12_BLEND_DESC &BlendDesc);
	void SetRasterizerState(const D3D12_RASTERIZER_DESC &RasterizerDesc);
	void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC &DepthStencilDesc);
	void SetSampleMask(UINT SampleMask);
	void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);
	void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1,
				   UINT MsaaQuality = 0);
	void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT *RTVFormats, DXGI_FORMAT DSVFormat,
				    UINT MsaaCount = 1, UINT MsaaQuality = 0);
	void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC *pInputElementDescs);
	void SetPrimitiveRestart(D3D12_INDEX_BUFFER_STRIP_CUT_VALUE IBProps);

	// These const_casts shouldn't be necessary, but we need to fix the API to accept "const void* pShaderBytecode"
	void SetVertexShader(const void *Binary, size_t Size);
	void SetPixelShader(const void *Binary, size_t Size);
	void SetGeometryShader(const void *Binary, size_t Size);
	void SetHullShader(const void *Binary, size_t Size);
	void SetDomainShader(const void *Binary, size_t Size);

	void SetVertexShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetPixelShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetGeometryShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetHullShader(const D3D12_SHADER_BYTECODE &Binary);
	void SetDomainShader(const D3D12_SHADER_BYTECODE &Binary);

	// Perform validation and compute a hash value for fast state block comparisons
	void Finalize();

private:
	D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;
	std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;
};

class ComputePSO : public PSO {
public:
	ComputePSO(const wchar_t *Name = L"Unnamed Compute PSO");

	void SetComputeShader(const void *Binary, size_t Size);
	void SetComputeShader(const D3D12_SHADER_BYTECODE &Binary);

	void Finalize();

private:
	D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
};

class CommandAllocatorPool {
public:
	CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
	~CommandAllocatorPool();

	void Create(ID3D12Device *pDevice);
	void Shutdown();

	ID3D12CommandAllocator *RequestAllocator(uint64_t CompletedFenceValue);
	void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator *Allocator);

	inline size_t Size();

private:
	const D3D12_COMMAND_LIST_TYPE m_cCommandListType;

	ID3D12Device *m_Device;
	std::vector<ID3D12CommandAllocator *> m_AllocatorPool;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator *>> m_ReadyAllocators;
};

class CommandQueue {
public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
	~CommandQueue();

	void Create(ID3D12Device *pDevice);
	void Shutdown();

	inline bool IsReady();

	uint64_t IncrementFence(void);
	bool IsFenceComplete(uint64_t FenceValue);
	void StallForFence(uint64_t FenceValue);
	void StallForProducer(CommandQueue &Producer);
	void WaitForFence(uint64_t FenceValue);
	void WaitForIdle(void);

	ID3D12CommandQueue *GetCommandQueue();

	uint64_t GetNextFenceValue();

	uint64_t ExecuteCommandList(ID3D12CommandList *List);
	ID3D12CommandAllocator *RequestAllocator(void);
	void DiscardAllocator(uint64_t FenceValueForReset, ID3D12CommandAllocator *Allocator);

private:
	ID3D12CommandQueue *m_CommandQueue;

	const D3D12_COMMAND_LIST_TYPE m_Type;

	CommandAllocatorPool m_AllocatorPool;

	// Lifetime of these objects is managed by the descriptor cache
	ID3D12Fence *m_pFence;
	uint64_t m_NextFenceValue;
	uint64_t m_LastCompletedFenceValue;
	HANDLE m_FenceEventHandle;
};

class CommandListManager {
public:
	CommandListManager();
	~CommandListManager();

	void Create(ID3D12Device *pDevice);
	void Shutdown();

	CommandQueue &GetGraphicsQueue(void);
	CommandQueue &GetComputeQueue(void);
	CommandQueue &GetCopyQueue(void);

	CommandQueue &GetQueue(D3D12_COMMAND_LIST_TYPE Type = D3D12_COMMAND_LIST_TYPE_DIRECT);

	ID3D12CommandQueue *GetCommandQueue();

	void CreateNewCommandList(D3D12_COMMAND_LIST_TYPE Type, ID3D12GraphicsCommandList **List,
				  ID3D12CommandAllocator **Allocator);

	bool IsFenceComplete(uint64_t FenceValue);

	// The CPU will wait for a fence to reach a specified value
	void WaitForFence(uint64_t FenceValue);

	// The CPU will wait for all command queues to empty (so that the GPU is idle)
	void IdleGPU(void);

private:
	ID3D12Device *m_Device;

	CommandQueue m_GraphicsQueue;
	CommandQueue m_ComputeQueue;
	CommandQueue m_CopyQueue;
};


class CommandContext : NonCopyable {
public:
	CommandContext(D3D12_COMMAND_LIST_TYPE Type);

	~CommandContext(void);

	void Reset(void);

	static CommandContext &Begin(const std::wstring ID = L"");

	// Flush existing commands to the GPU but keep the context alive
	uint64_t Flush(bool WaitForCompletion = false);

	// Flush existing commands and release the current context
	uint64_t Finish(bool WaitForCompletion = false);

	// Prepare to render by reserving a command list and command allocator
	void Initialize(void);

	GraphicsContext &GetGraphicsContext();

	ComputeContext &GetComputeContext();

	ID3D12GraphicsCommandList *GetCommandList();

	void CopyBuffer(GpuResource &Dest, GpuResource &Src);
	void CopyBufferRegion(GpuResource &Dest, size_t DestOffset, GpuResource &Src, size_t SrcOffset,
			      size_t NumBytes);
	void CopySubresource(GpuResource &Dest, UINT DestSubIndex, GpuResource &Src, UINT SrcSubIndex);
	void CopyCounter(GpuResource &Dest, size_t DestOffset, StructuredBuffer &Src);
	void CopyTextureRegion(GpuResource &Dest, UINT x, UINT y, UINT z, GpuResource &Source, RECT &rect);
	void ResetCounter(StructuredBuffer &Buf, uint32_t Value = 0);

	// Creates a readback buffer of sufficient size, copies the texture into it,
	// and returns row pitch in bytes.
	uint32_t ReadbackTexture(ReadbackBuffer &DstBuffer, PixelBuffer &SrcBuffer);

	DynAlloc ReserveUploadMemory(size_t SizeInBytes);

	static void InitializeTexture(GpuResource &Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
	static void InitializeBuffer(GpuBuffer &Dest, const void *Data, size_t NumBytes, size_t DestOffset = 0);
	static void InitializeBuffer(GpuBuffer &Dest, const UploadBuffer &Src, size_t SrcOffset, size_t NumBytes = -1,
				     size_t DestOffset = 0);
	static void InitializeTextureArraySlice(GpuResource &Dest, UINT SliceIndex, GpuResource &Src);

	void WriteBuffer(GpuResource &Dest, size_t DestOffset, const void *Data, size_t NumBytes);
	void FillBuffer(GpuResource &Dest, size_t DestOffset, DWParam Value, size_t NumBytes);

	void TransitionResource(GpuResource &Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
	void BeginResourceTransition(GpuResource &Resource, D3D12_RESOURCE_STATES NewState,
				     bool FlushImmediate = false);
	void InsertUAVBarrier(GpuResource &Resource, bool FlushImmediate = false);
	void InsertAliasBarrier(GpuResource &Before, GpuResource &After, bool FlushImmediate = false);
	inline void FlushResourceBarriers(void);

	void InsertTimeStamp(ID3D12QueryHeap *pQueryHeap, uint32_t QueryIdx);
	void ResolveTimeStamps(ID3D12Resource *pReadbackHeap, ID3D12QueryHeap *pQueryHeap, uint32_t NumQueries);
	void PIXBeginEvent(const wchar_t *label);
	void PIXEndEvent(void);
	void PIXSetMarker(const wchar_t *label);

	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap *HeapPtr);
	void SetDescriptorHeaps(UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap *HeapPtrs[]);
	void SetPipelineState(const PSO &PSO);

	void SetPredication(ID3D12Resource *Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

	void BindDescriptorHeaps(void);

	CommandListManager *m_OwningManager;
	ID3D12GraphicsCommandList *m_CommandList;
	ID3D12CommandAllocator *m_CurrentAllocator;

	ID3D12RootSignature *m_CurGraphicsRootSignature;
	ID3D12RootSignature *m_CurComputeRootSignature;
	ID3D12PipelineState *m_CurPipelineState;

	DynamicDescriptorHeap m_DynamicViewDescriptorHeap;    // HEAP_TYPE_CBV_SRV_UAV
	DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap; // HEAP_TYPE_SAMPLER

	D3D12_RESOURCE_BARRIER m_ResourceBarrierBuffer[16];
	UINT m_NumBarriersToFlush;

	ID3D12DescriptorHeap *m_CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	LinearAllocator m_CpuLinearAllocator;
	LinearAllocator m_GpuLinearAllocator;

	std::wstring m_ID;
	void SetID(const std::wstring &ID) { m_ID = ID; }

	D3D12_COMMAND_LIST_TYPE m_Type;
};

class GraphicsContext : public CommandContext {
public:
	static GraphicsContext &Begin(const std::wstring &ID = L"");
	void ClearUAV(GpuBuffer &Target);
	void ClearUAV(ColorBuffer &Target);
	void ClearColor(ColorBuffer &Target, D3D12_RECT *Rect = nullptr);
	void ClearColor(ColorBuffer &Target, float Colour[4], D3D12_RECT *Rect = nullptr);
	void ClearDepth(DepthBuffer &Target);
	void ClearStencil(DepthBuffer &Target);
	void ClearDepthAndStencil(DepthBuffer &Target);

	void BeginQuery(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
	void EndQuery(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
	void ResolveQueryData(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries,
			      ID3D12Resource *DestinationBuffer, UINT64 DestinationBufferOffset);

	void SetRootSignature(const RootSignature &RootSig);

	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV);

	void SetViewport(const D3D12_VIEWPORT &vp);
	void SetViewport(FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f);
	void SetScissor(const D3D12_RECT &rect);
	void SetScissor(UINT left, UINT top, UINT right, UINT bottom);
	void SetViewportAndScissor(const D3D12_VIEWPORT &vp, const D3D12_RECT &rect);
	void SetViewportAndScissor(UINT x, UINT y, UINT w, UINT h);
	void SetStencilRef(UINT StencilRef);
	void SetBlendFactor(Color BlendFactor);
	void SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY Topology);

	void SetConstantArray(UINT RootIndex, UINT NumConstants, const void *pConstants);
	void SetConstant(UINT RootIndex, UINT Offset, DWParam Val);
	void SetConstants(UINT RootIndex, DWParam X);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
	void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
	void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void *BufferData);
	void SetBufferSRV(UINT RootIndex, const GpuBuffer &SRV, UINT64 Offset = 0);
	void SetBufferUAV(UINT RootIndex, const GpuBuffer &UAV, UINT64 Offset = 0);
	void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

	void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count,
				   const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
	void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

	void SetIndexBuffer(const D3D12_INDEX_BUFFER_VIEW &IBView);
	void SetVertexBuffer(UINT Slot, const D3D12_VERTEX_BUFFER_VIEW &VBView);
	void SetVertexBuffers(UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[]);
	void SetDynamicVB(UINT Slot, size_t NumVertices, size_t VertexStride, const void *VBData);
	void SetDynamicIB(size_t IndexCount, const uint16_t *IBData);
	void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData);

	void Draw(UINT VertexCount, UINT VertexStartOffset = 0);
	void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
	void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount, UINT StartVertexLocation = 0,
			   UINT StartInstanceLocation = 0);
	void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
				  INT BaseVertexLocation, UINT StartInstanceLocation);
	void DrawIndirect(GpuBuffer &ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
	void ExecuteIndirect(CommandSignature &CommandSig, GpuBuffer &ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
			     uint32_t MaxCommands = 1, GpuBuffer *CommandCounterBuffer = nullptr,
			     uint64_t CounterOffset = 0);
};

class ComputeContext : public CommandContext {
public:
	static ComputeContext &Begin(const std::wstring &ID = L"", bool Async = false);

	void ClearUAV(GpuBuffer &Target);
	void ClearUAV(ColorBuffer &Target);

	void SetRootSignature(const RootSignature &RootSig);

	void SetConstantArray(UINT RootIndex, UINT NumConstants, const void *pConstants);
	void SetConstant(UINT RootIndex, UINT Offset, DWParam Val);
	void SetConstants(UINT RootIndex, DWParam X);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z);
	void SetConstants(UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W);
	void SetConstantBuffer(UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV);
	void SetDynamicConstantBufferView(UINT RootIndex, size_t BufferSize, const void *BufferData);
	void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData);
	void SetBufferSRV(UINT RootIndex, const GpuBuffer &SRV, UINT64 Offset = 0);
	void SetBufferUAV(UINT RootIndex, const GpuBuffer &UAV, UINT64 Offset = 0);
	void SetDescriptorTable(UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle);

	void SetDynamicDescriptor(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	void SetDynamicDescriptors(UINT RootIndex, UINT Offset, UINT Count,
				   const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);
	void SetDynamicSampler(UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle);
	void SetDynamicSamplers(UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[]);

	void Dispatch(size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1);
	void Dispatch1D(size_t ThreadCountX, size_t GroupSizeX = 64);
	void Dispatch2D(size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX = 8, size_t GroupSizeY = 8);
	void Dispatch3D(size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX,
			size_t GroupSizeY, size_t GroupSizeZ);
	void DispatchIndirect(GpuBuffer &ArgumentBuffer, uint64_t ArgumentBufferOffset = 0);
	void ExecuteIndirect(CommandSignature &CommandSig, GpuBuffer &ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
			     uint32_t MaxCommands = 1, GpuBuffer *CommandCounterBuffer = nullptr,
			     uint64_t CounterOffset = 0);
};

class GpuTimeManager {
public:
	static GpuTimeManager *GetInstance();
	void Initialize(uint32_t MaxNumTimers = 4096);
	void Shutdown();

	// Reserve a unique timer index
	uint32_t NewTimer(void);

	// Write start and stop time stamps on the GPU timeline
	void StartTimer(CommandContext &Context, uint32_t TimerIdx);
	void StopTimer(CommandContext &Context, uint32_t TimerIdx);

	// Bookend all calls to GetTime() with Begin/End which correspond to Map/Unmap.  This
	// needs to happen either at the very start or very end of a frame.
	void BeginReadBack(void);
	void EndReadBack(void);

	// Returns the time in milliseconds between start and stop queries
	float GetTime(uint32_t TimerIdx);

protected:
	ID3D12QueryHeap *m_QueryHeap = nullptr;
	ID3D12Resource *m_ReadBackBuffer = nullptr;
	uint64_t *m_TimeStampBuffer = nullptr;
	uint64_t m_Fence = 0;
	uint32_t m_MaxNumTimers = 0;
	uint32_t m_NumTimers = 1;
	uint64_t m_ValidTimeStart = 0;
	uint64_t m_ValidTimeEnd = 0;
	double m_GpuTickDelta = 0.0;
};

class GpuTimer {
public:
	GpuTimer();

	void Start(CommandContext &Context);
	void Stop(CommandContext &Context);

	float GetTime(void);
	uint32_t GetTimerIndex(void);

private:
	uint32_t m_TimerIndex;
};

class SystemTime {
public:
	static SystemTime *GetInstance();
	SystemTime();
	 int64_t GetCurrentTick(void);
	 void BusyLoopSleep(float SleepTime);
	 double TicksToSeconds(int64_t TickCount);
	 double TicksToMillisecs(int64_t TickCount);
	 double TimeBetweenTicks(int64_t tick1, int64_t tick2);

 private:
	double m_CpuTickDelta = 0.0;
};

class CpuTimer {
public:
	CpuTimer();
	void Start();
	void Stop();
	void Reset();
	double GetTime() const;

private:
	int64_t m_StartTick;
	int64_t m_ElapsedTicks;
};

class EngineProfiling {
public:
	static EngineProfiling* GetInstace();
	void Update();

	void BeginBlock(const std::wstring &name, CommandContext *Context = nullptr);
	void EndBlock(CommandContext *Context = nullptr);

	void DisplayFrameRate();

private:
	bool Paused = false;
};
