#pragma once

#include <d3d12-common.hpp>

namespace D3D12Graphics {

class D3D12DeviceInstance;

class CommandAllocatorPool {
public:
	CommandAllocatorPool(D3D12_COMMAND_LIST_TYPE Type);
	~CommandAllocatorPool();

	void Create(D3D12DeviceInstance *DeviceInstance);
	void Shutdown();

	ID3D12CommandAllocator *RequestAllocator(uint64_t CompletedFenceValue);
	void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator *Allocator);

	inline size_t Size();

private:
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	const D3D12_COMMAND_LIST_TYPE m_cCommandListType;
	std::vector<ID3D12CommandAllocator *> m_AllocatorPool;
	std::queue<std::pair<uint64_t, ID3D12CommandAllocator *>> m_ReadyAllocators;
};

class CommandQueue {
public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE Type);
	~CommandQueue();

	void Create(D3D12DeviceInstance *DeviceInstance);
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
	D3D12DeviceInstance *m_DeviceInstance = nullptr;
	ID3D12CommandQueue *m_CommandQueue;

	const D3D12_COMMAND_LIST_TYPE m_Type;

	std::unique_ptr<CommandAllocatorPool> m_AllocatorPool;

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

	void Create(D3D12DeviceInstance *DeviceInstance);
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
	D3D12DeviceInstance *m_DeviceInstance = nullptr;

	std::unique_ptr<CommandQueue> m_GraphicsQueue;
	std::unique_ptr<CommandQueue> m_ComputeQueue;
	std::unique_ptr<CommandQueue> m_CopyQueue;
};

} // namespace D3D12Graphics
