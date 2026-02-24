#include <d3d12-deviceinstance.hpp>

namespace D3D12Graphics {

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

	if (!IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to WriteBuffer must be 16-byte aligned.");
	}
	DynAlloc TempSpace = m_CpuLinearAllocator->Allocate(NumBytes, 512);
	memcpy(TempSpace.DataPtr, BufferData, NumBytes);
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
	if (!IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicConstantBufferView must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	memcpy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16));
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
	if (!IsAligned(VertexData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicVB must be 16-byte aligned.");
	}
	size_t BufferSize = AlignUp(NumVertices * VertexStride, 16);
	DynAlloc vb = m_CpuLinearAllocator->Allocate(BufferSize);

	memcpy(vb.DataPtr, VertexData, BufferSize);

	D3D12_VERTEX_BUFFER_VIEW VBView;
	VBView.BufferLocation = vb.GpuAddress;
	VBView.SizeInBytes = (UINT)BufferSize;
	VBView.StrideInBytes = (UINT)VertexStride;

	m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
}

void GraphicsContext::SetDynamicIB(size_t IndexCount, const uint16_t *IndexData)
{
	if (!IsAligned(IndexData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicIB must be 16-byte aligned.");
	}
	size_t BufferSize = AlignUp(IndexCount * sizeof(uint16_t), 16);
	DynAlloc ib = m_CpuLinearAllocator->Allocate(BufferSize);

	memcpy(ib.DataPtr, IndexData, BufferSize);

	D3D12_INDEX_BUFFER_VIEW IBView;
	IBView.BufferLocation = ib.GpuAddress;
	IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
	IBView.Format = DXGI_FORMAT_R16_UINT;

	m_CommandList->IASetIndexBuffer(&IBView);
}

void GraphicsContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	if (!IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicSRV must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	memcpy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16));
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
	if (!IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicConstantBufferView must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	memcpy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16));
	m_CommandList->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress);
}

void ComputeContext::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void *BufferData)
{
	if (BufferData == nullptr) {
		return;
	}
	if (!IsAligned(BufferData, 16)) {
		throw HRError("BufferData pointer passed to SetDynamicSRV must be 16-byte aligned.");
	}

	DynAlloc cb = m_CpuLinearAllocator->Allocate(BufferSize);
	memcpy(cb.DataPtr, BufferData, AlignUp(BufferSize, 16));
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

} // namespace D3D12Graphics
