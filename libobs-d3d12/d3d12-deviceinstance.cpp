#include <d3d12-deviceinstance.hpp>

#include <util/base.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <util/util.hpp>

#include <map>
#include <thread>
#include <cinttypes>

#include <shellscalingapi.h>
#include <d3dkmthk.h>

namespace D3D12Graphics {

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
	memcpy(mem.DataPtr, BufferData, NumBytes);

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
