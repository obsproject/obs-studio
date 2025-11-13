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

void gs_zstencil_buffer::InitBuffer()
{
	HRESULT hr;
	D3D12_RESOURCE_DESC td;
	memset(&td, 0, sizeof(td));
	td.Width = width;
	td.Height = height;

	td.Alignment = 0;
	td.DepthOrArraySize = 1;
	td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	td.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	td.Format = dxgiFormat;
	td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	td.MipLevels = 1;
	td.SampleDesc.Count = 1;
	td.SampleDesc.Quality = 0;

	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = dxgiFormat;

	D3D12_HEAP_PROPERTIES heapProp;
	memset(&heapProp, 0, sizeof(heapProp));

	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 1;
	heapProp.VisibleNodeMask = 1;

	hr = device->device->CreateCommittedResource(&heapProp, D3D12_HEAP_FLAG_NONE, &td, D3D12_RESOURCE_STATE_COMMON,
						     &clearValue, IID_PPV_ARGS(&texture));
	if (FAILED(hr))
		throw HRError("Failed to create depth stencil texture", hr);

	device->AssignStagingDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, &textureDescriptor);

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;
	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.Format = dxgiFormat;

	device->device->CreateDepthStencilView(texture.Get(), &dsvDesc, textureDescriptor.cpuHandle);
}

gs_zstencil_buffer::gs_zstencil_buffer(gs_device_t *device, uint32_t width, uint32_t height, gs_zstencil_format format)
	: gs_obj(device, gs_type::gs_zstencil_buffer),
	  width(width),
	  height(height),
	  format(format),
	  dxgiFormat(ConvertGSZStencilFormat(format))
{
	InitBuffer();
}
