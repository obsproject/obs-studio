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

void gs_index_buffer::InitBuffer()
{
	D3D12_RESOURCE_DESC desc;
	D3D12_HEAP_PROPERTIES props;

	memset(&desc, 0, sizeof(D3D12_RESOURCE_DESC));
	memset(&props, 0, sizeof(D3D12_HEAP_PROPERTIES));

	props.Type = D3D12_HEAP_TYPE_UPLOAD;
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = indexSize * num;
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr;

	hr = device->device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
						     D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
						     IID_PPV_ARGS(indexBuffer.Assign()));
	if (FAILED(hr))
		throw HRError("Failed to create buffer", hr);

	view.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	view.SizeInBytes = indexSize * num;

	if (type == gs_index_type::GS_UNSIGNED_SHORT) {
		view.Format = DXGI_FORMAT_R16_UINT;
	} else if (type == gs_index_type::GS_UNSIGNED_LONG) {
		view.Format = DXGI_FORMAT_R32_UINT;
	} else {
		view.Format = DXGI_FORMAT_R16_UINT;
	}
}

gs_index_buffer::gs_index_buffer(gs_device_t *device, enum gs_index_type type, void *indices, size_t num,
				 uint32_t flags)
	: gs_obj(device, gs_type::gs_index_buffer),
	  dynamic((flags & GS_DYNAMIC) != 0),
	  type(type),
	  num(num),
	  indices(indices)
{
	switch (type) {
	case GS_UNSIGNED_SHORT:
		indexSize = 2;
		break;
	case GS_UNSIGNED_LONG:
		indexSize = 4;
		break;
	}

	InitBuffer();
}
