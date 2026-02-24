#include <d3d12-deviceinstance.hpp>

namespace D3D12Graphics {

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

} // namespace D3D12Graphics
