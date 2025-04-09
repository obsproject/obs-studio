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

gs_stage_surface::gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height, gs_color_format colorFormat)
	: gs_obj(device, gs_type::gs_stage_surface),
	  width(width),
	  height(height),
	  format(colorFormat),
	  dxgiFormat(ConvertGSTextureFormatView(colorFormat))
{
	HRESULT hr;

	D3D12_RESOURCE_DESC downloadDesc;
	memset(&downloadDesc, 0, sizeof(downloadDesc));
	downloadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	downloadDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	downloadDesc.Height = 1;
	downloadDesc.DepthOrArraySize = 1;
	downloadDesc.MipLevels = 1;
	downloadDesc.Format = DXGI_FORMAT_UNKNOWN;
	downloadDesc.SampleDesc.Count = 1;
	downloadDesc.SampleDesc.Quality = 0;
	downloadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	downloadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProperties;
	memset(&heapProperties, 0, sizeof(heapProperties));
	heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));

	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.Format = dxgiFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// NV12 textures must have even width and height
	if (dxgiFormat == DXGI_FORMAT_NV12 || dxgiFormat == DXGI_FORMAT_P010) {
		textureDesc.Width = (textureDesc.Width + 1) & ~1;
		textureDesc.Height = (textureDesc.Height + 1) & ~1;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTextureDesc;
	uint32_t row, NumRows, RowPitch;
	uint64_t RowLength;
	device->device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &placedTextureDesc, &NumRows, &RowLength,
					      &downloadDesc.Width);
	RowPitch = placedTextureDesc.Footprint.RowPitch;

	hr = device->device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &downloadDesc,
						     D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
	if (FAILED(hr))
		throw HRError("Failed to create staging surface", hr);
}

gs_stage_surface::gs_stage_surface(gs_device_t *device, uint32_t width, uint32_t height, bool p010)
	: gs_obj(device, gs_type::gs_stage_surface),
	  width(width),
	  height(height),
	  format(GS_UNKNOWN),
	  dxgiFormat(p010 ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12)
{
	HRESULT hr;

	D3D12_RESOURCE_DESC downloadDesc;
	memset(&downloadDesc, 0, sizeof(downloadDesc));
	downloadDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	downloadDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	downloadDesc.Height = 1;
	downloadDesc.DepthOrArraySize = 1;
	downloadDesc.MipLevels = 1;
	downloadDesc.Format = DXGI_FORMAT_UNKNOWN;
	downloadDesc.SampleDesc.Count = 1;
	downloadDesc.SampleDesc.Quality = 0;
	downloadDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	downloadDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	D3D12_HEAP_PROPERTIES heapProperties;
	memset(&heapProperties, 0, sizeof(heapProperties));
	heapProperties.Type = D3D12_HEAP_TYPE_READBACK;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC textureDesc;
	memset(&textureDesc, 0, sizeof(textureDesc));

	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.MipLevels = 1;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.Format = dxgiFormat;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// NV12 textures must have even width and height
	if (dxgiFormat == DXGI_FORMAT_NV12 || dxgiFormat == DXGI_FORMAT_P010) {
		textureDesc.Width = (textureDesc.Width + 1) & ~1;
		textureDesc.Height = (textureDesc.Height + 1) & ~1;
	}

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT placedTextureDesc;
	uint32_t row, NumRows, RowPitch;
	uint64_t RowLength;
	device->device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &placedTextureDesc, &NumRows, &RowLength,
					      &downloadDesc.Width);
	RowPitch = placedTextureDesc.Footprint.RowPitch;

	hr = device->device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &downloadDesc,
						     D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&texture));
	if (FAILED(hr))
		throw HRError("Failed to create staging surface", hr);
}
