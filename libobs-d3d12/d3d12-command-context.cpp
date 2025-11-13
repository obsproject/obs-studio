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

#pragma once

#include "d3d12-subsystem.hpp"

gs_command_queue::gs_command_queue(ID3D12Device *device_, D3D12_COMMAND_LIST_TYPE type_) : device(device_), type(type_)
{

	D3D12_COMMAND_QUEUE_DESC QueueDesc = {};
	QueueDesc.Type = type;
	QueueDesc.NodeMask = 0;
	HRESULT hr = device->CreateCommandQueue(&QueueDesc, IID_PPV_ARGS(&commandQueue));
	if (FAILED(hr))
		throw HRError("create command queue failed", hr);

	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
	if (FAILED(hr))
		throw HRError("create fence failed", hr);

	fence->Signal(0);

	fenceEventHandle = CreateEvent(nullptr, false, false, nullptr);
}

uint64_t gs_command_queue::IncrementFence(void)
{
	commandQueue->Signal(fence, nextFenceValue);
	return nextFenceValue++;
}

bool gs_command_queue::IsFenceComplete(uint64_t fenceValue)
{
	if (fenceValue > lastCompletedFenceValue)
		lastCompletedFenceValue = (lastCompletedFenceValue > fence->GetCompletedValue()
						   ? lastCompletedFenceValue
						   : fence->GetCompletedValue());

	return fenceValue <= lastCompletedFenceValue;
}

void gs_command_queue::WaitForFence(uint64_t fenceValue)
{
	if (IsFenceComplete(fenceValue))
		return;

	fence->SetEventOnCompletion(fenceValue, fenceEventHandle);
	WaitForSingleObject(fenceEventHandle, INFINITE);
	lastCompletedFenceValue = fenceValue;
}

void gs_command_queue::WaitForIdle()
{
	WaitForFence(IncrementFence());
}

uint64_t gs_command_queue::ExecuteCommandList(ID3D12GraphicsCommandList *list)
{
	HRESULT hr = list->Close();
	if (FAILED(hr))
		throw HRError("graphics command list close failed", hr);

	commandQueue->ExecuteCommandLists(1, (ID3D12CommandList **)&list);
	commandQueue->Signal(fence, nextFenceValue);
	return nextFenceValue++;
}

ID3D12CommandAllocator *gs_command_queue::RequestAllocator()
{
	uint64_t completedFence = fence->GetCompletedValue();
	ID3D12CommandAllocator *pAllocator = nullptr;

	if (!readyAllocators.empty()) {
		std::pair<uint64_t, ID3D12CommandAllocator *> &allocatorPair = readyAllocators.front();

		if (allocatorPair.first <= completedFence) {
			pAllocator = allocatorPair.second;
			HRESULT hr = pAllocator->Reset();
			if (FAILED(hr))
				throw HRError("allocator reset failed", hr);
			readyAllocators.pop();
		}
	}

	if (pAllocator == nullptr) {
		HRESULT hr = device->CreateCommandAllocator(type, IID_PPV_ARGS(&pAllocator));
		allocatorPool.push_back(pAllocator);
	}

	return pAllocator;
}

void gs_command_queue::DiscardAllocator(uint64_t fenceValueForReset, ID3D12CommandAllocator *allocator)
{
	readyAllocators.push(std::make_pair(fenceValueForReset, allocator));
}

gs_command_context::gs_command_context(gs_device *device_) : device(device_)
{
	currentAllocator = device->commandQueue->RequestAllocator();

	HRESULT hr = device->device->CreateCommandList(1, device->commandQueue->type, currentAllocator, nullptr,
						       IID_PPV_ARGS(&commandList));
	if (FAILED(hr))
		throw HRError("create command list failed", hr);
	gpu_descriptor_heap[0] =
		gs_gpu_descriptor_heap_create(device->device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 64);
	gpu_descriptor_heap[1] = gs_gpu_descriptor_heap_create(device->device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 8);
}

void gs_command_context::Reset()
{
	currentAllocator = device->commandQueue->RequestAllocator();
	commandList->Reset(currentAllocator, nullptr);

	gs_gpu_descriptor_heap_reset(gpu_descriptor_heap[0]);
	gs_gpu_descriptor_heap_reset(gpu_descriptor_heap[1]);

	ID3D12DescriptorHeap *rootDescriptorHeaps[2];
	rootDescriptorHeaps[0] = gpu_descriptor_heap[0]->handle;
	rootDescriptorHeaps[1] = gpu_descriptor_heap[1]->handle;
	commandList->SetDescriptorHeaps(2, rootDescriptorHeaps);
}

uint64_t gs_command_context::Flush(bool waitForCompletion)
{
	if (numBarriersToFlush > 0) {
		commandList->ResourceBarrier(numBarriersToFlush, resourceBarrierBuffer);
		numBarriersToFlush = 0;
	}

	uint64_t FenceValue = device->commandQueue->ExecuteCommandList(commandList);

	if (waitForCompletion)
		device->commandQueue->WaitForFence(FenceValue);

	commandList->Reset(currentAllocator, nullptr);

	return FenceValue;
}

uint64_t gs_command_context::Finish(bool waitForCompletion)
{
	if (numBarriersToFlush > 0) {
		commandList->ResourceBarrier(numBarriersToFlush, resourceBarrierBuffer);
		numBarriersToFlush = 0;
	}

	uint64_t fenceValue = device->commandQueue->ExecuteCommandList(commandList);
	device->commandQueue->DiscardAllocator(fenceValue, currentAllocator);
	currentAllocator = nullptr;

	if (waitForCompletion)
		device->commandQueue->WaitForFence(fenceValue);

	return fenceValue;
}

void gs_command_context::TransitionResource(ID3D12Resource *resource, D3D12_RESOURCE_STATES beforeState,
					    D3D12_RESOURCE_STATES newState, bool flushImmediate)
{
	if (beforeState != newState) {
		D3D12_RESOURCE_BARRIER &BarrierDesc = resourceBarrierBuffer[numBarriersToFlush++];

		BarrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		BarrierDesc.Transition.pResource = resource;
		BarrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		BarrierDesc.Transition.StateBefore = beforeState;
		BarrierDesc.Transition.StateAfter = newState;
		BarrierDesc.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	}

	if (flushImmediate || numBarriersToFlush == 32) {
		if (numBarriersToFlush > 0) {
			commandList->ResourceBarrier(numBarriersToFlush, resourceBarrierBuffer);
			numBarriersToFlush = 0;
		}
	}
}
