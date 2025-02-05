/******************************************************************************
    Copyright (C) 2025 by hongqingwan <hongqingwan@obsproject.com>

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

	props.Type = dynamic ? D3D12_HEAP_TYPE_UPLOAD : D3D12_HEAP_TYPE_DEFAULT;
	props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	desc.Width = UINT(indexSize * num);
	desc.Height = 1;
	desc.DepthOrArraySize = 1;
	desc.MipLevels = 1;
	desc.Format = DXGI_FORMAT_UNKNOWN;
	desc.SampleDesc.Count = 1;
	desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	desc.Flags = D3D12_RESOURCE_FLAG_NONE;

	HRESULT hr;

	hr = device->device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
						     dynamic ? D3D12_RESOURCE_STATE_GENERIC_READ
							     : D3D12_RESOURCE_STATE_COPY_DEST,
						     nullptr, IID_PPV_ARGS(indexBuffer.Assign()));
	if (FAILED(hr))
		throw HRError("Failed to create buffer", hr);
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
