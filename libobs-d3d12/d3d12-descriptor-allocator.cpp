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

#include "d3d12-subsystem.hpp"

gs_descriptor_allocator::gs_descriptor_allocator(gs_device_t *device_, D3D12_DESCRIPTOR_HEAP_TYPE type)
	: device(device_),
	  descriptorHeadType(type)
{
}

gs_descriptor_allocator::~gs_descriptor_allocator()
{
	Release();
}

D3D12_CPU_DESCRIPTOR_HANDLE gs_descriptor_allocator::Allocate(uint32_t count)
{
	if (currentHeap == nullptr || remainingFreeHandles < count) {
		currentHeap = RequestNewHeap(descriptorHeadType);
		currentHandle = currentHeap->GetCPUDescriptorHandleForHeapStart();
		remainingFreeHandles = NUM_DESCRIPTORS_PER_HEAP;

		if (descriptorSize == 0)
			descriptorSize = device->device->GetDescriptorHandleIncrementSize(descriptorHeadType);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE ret = currentHandle;
	currentHandle.ptr += count * descriptorSize;
	remainingFreeHandles -= count;
	return ret;
}

ComPtr<ID3D12DescriptorHeap> gs_descriptor_allocator::RequestNewHeap(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc;
	desc.Type = Type;
	desc.NumDescriptors = NUM_DESCRIPTORS_PER_HEAP;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NodeMask = 1;

	ComPtr<ID3D12DescriptorHeap> heap;
	device->device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap));
	descriptorHeapPool.emplace_back(heap);
	return heap;
}

void gs_descriptor_allocator::Release(void) {}
