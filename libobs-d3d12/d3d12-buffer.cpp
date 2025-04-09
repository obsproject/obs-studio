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

gs_buffer::gs_buffer(gs_device *device, int32_t size_, gs_type type, uint32_t flags)
	: gs_obj(device, type),
	  size(size_),
	  usageFlags(flags)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
	D3D12_HEAP_PROPERTIES heapProperties;
	D3D12_RESOURCE_DESC desc;
	D3D12_HEAP_FLAGS heapFlags = (D3D12_HEAP_FLAGS)0;
	D3D12_RESOURCE_FLAGS resourceFlags = (D3D12_RESOURCE_FLAGS)0;
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	if (usageFlags & GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE) {
		resourceFlags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}

	heapProperties.CreationNodeMask = 0; // We don't do multi-adapter operation
	heapProperties.VisibleNodeMask = 0;  // We don't do multi-adapter operation

	if (type == gs_type::gs_gpu_buffer) {
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapFlags = D3D12_HEAP_FLAG_NONE;
	} else if (type == gs_type::gs_upload_buffer) {
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapFlags = D3D12_HEAP_FLAG_NONE;
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
	} else if (type == gs_type::gs_download_buffer) {
		heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapFlags = D3D12_HEAP_FLAG_NONE;
		initialState = D3D12_RESOURCE_STATE_COPY_DEST;
	} else if (type == gs_type::gs_uniform_buffer) {
		heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
		heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;

		heapFlags = D3D12_HEAP_FLAG_NONE;
	}

	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	desc.Width = size;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = resourceFlags;

	HRESULT hr = device->device->CreateCommittedResource(&heapProperties, heapFlags, &desc, initialState, NULL,
							     IID_PPV_ARGS(&resource));
	if (FAILED(hr))
		throw HRError("failed create gs buffer", hr);

	uavDescriptor.heap = NULL;
	srvDescriptor.heap = NULL;
	cbvDescriptor.heap = NULL;

	if (usageFlags & GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE) {
		device->AssignStagingDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &uavDescriptor);

		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = size / sizeof(uint32_t);
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
		uavDesc.Buffer.CounterOffsetInBytes = 0; // TODO: support counters?
		uavDesc.Buffer.StructureByteStride = 0;

		// Create UAV
		device->device->CreateUnorderedAccessView(resource,
							  NULL, // TODO: support counters?
							  &uavDesc, uavDescriptor.cpuHandle);
	}

	if ((usageFlags & GS_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ) ||
	    (usageFlags & GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ)) {
		device->AssignStagingDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &srvDescriptor);

		srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = size / sizeof(uint32_t);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
		srvDesc.Buffer.StructureByteStride = 0;

		// Create SRV
		device->device->CreateShaderResourceView(resource, &srvDesc, srvDescriptor.cpuHandle);
	}

	// FIXME: we may not need a CBV since we use root descriptors
	if (type == gs_type::gs_uniform_buffer) {
		device->AssignStagingDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, &cbvDescriptor);

		cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = size;

		// Create CBV
		device->device->CreateConstantBufferView(&cbvDesc, cbvDescriptor.cpuHandle);
	}

	gpuVirtualAddress = resource->GetGPUVirtualAddress();
	transitioned = initialState != D3D12_RESOURCE_STATE_COMMON;
}

static D3D12_RESOURCE_STATES GetDefaultBufferResourceState(gs_buffer *buffer)
{
	if (buffer->usageFlags & GS_GPU_BUFFERUSAGE_VERTEX) {
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	} else if (buffer->usageFlags & GS_GPU_BUFFERUSAGE_INDEX) {
		return D3D12_RESOURCE_STATE_INDEX_BUFFER;
	} else if (buffer->usageFlags & GS_GPU_BUFFERUSAGE_INDIRECT) {
		return D3D12_RESOURCE_STATE_INDIRECT_ARGUMENT;
	} else if (buffer->usageFlags & GS_GPU_BUFFERUSAGE_GRAPHICS_STORAGE_READ) {
		return D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE;
	} else if (buffer->usageFlags & GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ) {
		return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	} else if (buffer->usageFlags & GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE) {
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	}
}

static void ResourceBarrier(ID3D12GraphicsCommandList *commandList, D3D12_RESOURCE_STATES sourceState,
			    D3D12_RESOURCE_STATES destinationState, ID3D12Resource *resource, uint32_t subresourceIndex,
			    bool needsUavBarrier)
{
	D3D12_RESOURCE_BARRIER barrierDesc[2];
	uint32_t numBarriers = 0;

	if (sourceState != destinationState) {
		barrierDesc[numBarriers].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrierDesc[numBarriers].Flags = (D3D12_RESOURCE_BARRIER_FLAGS)0;
		barrierDesc[numBarriers].Transition.StateBefore = sourceState;
		barrierDesc[numBarriers].Transition.StateAfter = destinationState;
		barrierDesc[numBarriers].Transition.pResource = resource;
		barrierDesc[numBarriers].Transition.Subresource = subresourceIndex;

		numBarriers += 1;
	}

	if (needsUavBarrier) {
		barrierDesc[numBarriers].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
		barrierDesc[numBarriers].Flags = (D3D12_RESOURCE_BARRIER_FLAGS)0;
		barrierDesc[numBarriers].UAV.pResource = resource;

		numBarriers += 1;
	}

	if (numBarriers > 0) {
		commandList->ResourceBarrier(numBarriers, barrierDesc);
	}
}

static void BufferTransitionFromDefaultUsage(ID3D12GraphicsCommandList *commandList,
					     D3D12_RESOURCE_STATES destinationState, gs_buffer *buffer)
{

	ResourceBarrier(commandList,
			buffer->transitioned ? GetDefaultBufferResourceState(buffer) : D3D12_RESOURCE_STATE_COMMON,
			destinationState, buffer->resource, 0,
			buffer->usageFlags & GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE);

	buffer->transitioned = true;
}

static void BufferTransitionToDefaultUsage(ID3D12GraphicsCommandList *commandList, D3D12_RESOURCE_STATES sourceState,
					   gs_buffer *buffer)
{
	ResourceBarrier(commandList, buffer->transitioned ? sourceState : D3D12_RESOURCE_STATE_COMMON,
			GetDefaultBufferResourceState(buffer), buffer->resource, 0,
			buffer->usageFlags & GS_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE);

	buffer->transitioned = true;
}

void gs_buffer::UploadToBuffer(gs_buffer *source, uint32_t source_offset, gs_buffer *dest, uint32_t dest_offset)
{
	/*BufferTransitionFromDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_DEST, dest);
	device->commandList->CopyBufferRegion(dest->resource, dest_offset, source->resource, source_offset, dest->size);
	BufferTransitionToDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_DEST, dest);*/
}

void gs_buffer::UploadToBuffer(uint8_t *data, size_t size, gs_buffer *dest, uint32_t dest_offset) {}

void gs_buffer::CpoyBufferToBuffer(gs_buffer *source, uint32_t source_offset, gs_buffer *dest, uint32_t dest_offset)
{
	/*BufferTransitionFromDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_DEST, dest);
	BufferTransitionFromDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, source);

	device->commandList->CopyBufferRegion(dest->resource, dest_offset, source->resource, source_offset, dest->size);

	BufferTransitionToDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, source);
	BufferTransitionToDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_DEST, dest);*/
}

void gs_buffer::DownloadFromBuffer(gs_buffer *source, uint32_t source_offset, gs_buffer *dest, uint32_t dest_offset)
{
	/*BufferTransitionFromDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, source);
	device->commandList->CopyBufferRegion(dest->resource, dest_offset, source->resource, source_offset,
					      source->size);
	BufferTransitionToDefaultUsage(device->commandList, D3D12_RESOURCE_STATE_COPY_SOURCE, source);*/
}
