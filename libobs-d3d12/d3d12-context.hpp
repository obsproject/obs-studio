#pragma once

#include <d3d12-common.hpp>
#include <d3d12-dynamic-descriptorheap.hpp>

namespace D3D12Graphics {

class D3D12DeviceInstance;
class GpuBuffer;
class GpuResource;
class UploadBuffer;
class ReadbackBuffer;
struct DynAlloc;
class PSO;
class DynamicDescriptorHeap;
class LinearAllocator;
class RootSignature;
class DepthBuffer;

struct NonCopyable {
	NonCopyable() = default;
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable &operator=(const NonCopyable &) = delete;
};

class CommandContext : NonCopyable {
public:
	CommandContext(D3D12DeviceInstance *DeviceInstance, D3D12_COMMAND_LIST_TYPE Type);

	~CommandContext(void);

	void Reset(void);
	// Flush existing commands to the GPU but keep the context alive
	uint64_t Flush(bool WaitForCompletion = false);

	// Flush existing commands and release the current context
	uint64_t Finish(bool WaitForCompletion = false);

	// Prepare to render by reserving a command list and command allocator
	void Initialize(void);

	ID3D12GraphicsCommandList *GetCommandList();

	void CopyBuffer(GpuResource &Dest, GpuResource &Src);
	void CopyBufferRegion(GpuResource &Dest, size_t DestOffset, GpuResource &Src, size_t SrcOffset,
			      size_t NumBytes);
	void CopySubresource(GpuResource &Dest, UINT DestSubIndex, GpuResource &Src, UINT SrcSubIndex);
	void CopyTextureRegion(GpuResource &Dest, UINT x, UINT y, UINT z, GpuResource &Source, RECT &rect);
	void UpdateTexture(GpuResource &Dest, UploadBuffer &buffer);

	// Creates a readback buffer of sufficient size, copies the texture into it,
	// and returns row pitch in bytes.
	uint32_t ReadbackTexture(ReadbackBuffer &DstBuffer, GpuResource &SrcBuffer);

	DynAlloc ReserveUploadMemory(size_t SizeInBytes);

	void WriteBuffer(GpuResource &Dest, size_t DestOffset, const void *Data, size_t NumBytes);

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

	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	ID3D12GraphicsCommandList *m_CommandList;
	ID3D12CommandAllocator *m_CurrentAllocator;

	ID3D12RootSignature *m_CurGraphicsRootSignature;
	ID3D12RootSignature *m_CurComputeRootSignature;
	ID3D12PipelineState *m_CurPipelineState;

	DynamicDescriptorHeap m_DynamicViewDescriptorHeap;    // HEAP_TYPE_CBV_SRV_UAV
	DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap; // HEAP_TYPE_SAMPLER

	D3D12_RESOURCE_BARRIER m_ResourceBarrierBuffer[kMaxNumDescriptorTables];
	UINT m_NumBarriersToFlush;

	ID3D12DescriptorHeap *m_CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	std::unique_ptr<LinearAllocator> m_CpuLinearAllocator;
	std::unique_ptr<LinearAllocator> m_GpuLinearAllocator;

	std::wstring m_ID;
	void SetID(const std::wstring &ID) { m_ID = ID; }

	D3D12_COMMAND_LIST_TYPE m_Type;
};

class GraphicsContext : public CommandContext {
public:
	void ClearUAV(GpuBuffer &Target);

	void ClearColor(D3D12_CPU_DESCRIPTOR_HANDLE RenderTargetView, const FLOAT ColorRGBA[4], UINT NumRects = 0,
			const D3D12_RECT *pRects = nullptr);

	void ClearDepth(DepthBuffer &Target);
	void ClearStencil(DepthBuffer &Target);
	void ClearDepthAndStencil(DepthBuffer &Target);

	void BeginQuery(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
	void EndQuery(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
	void ResolveQueryData(ID3D12QueryHeap *QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries,
			      ID3D12Resource *DestinationBuffer, UINT64 DestinationBufferOffset);

	void SetRootSignature(const RootSignature &RootSig);

	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
	void SetRenderTargets(UINT NumRenderTargetDescriptors,
			      const D3D12_CPU_DESCRIPTOR_HANDLE *pRenderTargetDescriptors,
			      BOOL RTsSingleHandleToDescriptorRange,
			      const D3D12_CPU_DESCRIPTOR_HANDLE *pDepthStencilDescriptor);
	void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
	void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV);
	void SetNullRenderTarget();
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
	void SetConstant(UINT RootIndex, UINT Offset, UINT Val);
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
};

class ComputeContext : public CommandContext {
public:
	void ClearUAV(GpuBuffer &Target);

	void SetRootSignature(const RootSignature &RootSig);

	void SetConstantArray(UINT RootIndex, UINT NumConstants, const void *pConstants);
	void SetConstant(UINT RootIndex, UINT Offset, UINT Val);
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
};

class ContextManager {
public:
	ContextManager(D3D12DeviceInstance *DeviceInstance);

	CommandContext *AllocateContext(D3D12_COMMAND_LIST_TYPE Type);
	void FreeContext(CommandContext *);
	void DestroyAllContexts();

private:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	std::vector<std::unique_ptr<CommandContext>> m_ContextPool[4];
	std::queue<CommandContext *> m_AvailableContexts[4];
};

} // namespace D3D12Graphics
