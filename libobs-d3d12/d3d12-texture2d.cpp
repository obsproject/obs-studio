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

#include <util/base.h>
#include "d3dx12.h"
#include "d3d12-subsystem.hpp"

void gs_texture_2d::InitSRD(std::vector<D3D12_SUBRESOURCE_DATA> &srd)
{
	uint32_t rowSizeBytes = width * gs_get_format_bpp(format);
	uint32_t texSizeBytes = height * rowSizeBytes / 8;
	size_t textures = type == GS_TEXTURE_2D ? 1 : 6;
	uint32_t actual_levels = levels;
	size_t curTex = 0;

	if (!actual_levels)
		actual_levels = gs_get_total_levels(width, height, 1);

	rowSizeBytes /= 8;

	for (size_t i = 0; i < textures; i++) {
		uint32_t newRowSize = rowSizeBytes;
		uint32_t newTexSize = texSizeBytes;

		for (uint32_t j = 0; j < actual_levels; j++) {
			D3D12_SUBRESOURCE_DATA newSRD;
			newSRD.pData = data[curTex++].data();
			newSRD.RowPitch = newRowSize;
			newSRD.SlicePitch = newTexSize;
			srd.push_back(newSRD);

			newRowSize /= 2;
			newTexSize /= 4;
		}
	}
}

void gs_texture_2d::BackupTexture(const uint8_t *const *data)
{
	uint32_t textures = type == GS_TEXTURE_CUBE ? 6 : 1;
	uint32_t bbp = gs_get_format_bpp(format);

	this->data.resize(levels * textures);

	for (uint32_t t = 0; t < textures; t++) {
		uint32_t w = width;
		uint32_t h = height;

		for (uint32_t lv = 0; lv < levels; lv++) {
			uint32_t i = levels * t + lv;
			if (!data[i])
				break;

			uint32_t texSize = bbp * w * h / 8;

			std::vector<uint8_t> &subData = this->data[i];
			subData.resize(texSize);
			memcpy(&subData[0], data[i], texSize);

			if (w > 1)
				w /= 2;
			if (h > 1)
				h /= 2;
		}
	}
}

void gs_texture_2d::InitTexture(const uint8_t *const *data)
{
	HRESULT hr;

	if (type == GS_TEXTURE_CUBE)
		layerCountOrDepth = 6;
	else
		layerCountOrDepth = 1;

	bool isMultisampled = sampleCount > 1;

	memset(&td, 0, sizeof(td));
	td.Width = width;
	td.Height = height;
	td.MipLevels = genMipmaps ? 0 : levels;
	td.DepthOrArraySize = layerCountOrDepth;
	td.SampleDesc.Count = sampleCount;
	td.SampleDesc.Quality = isMultisampled ? -1 : 0;
	td.Format = twoPlane ? ((format == GS_R16) ? DXGI_FORMAT_P010 : DXGI_FORMAT_NV12) : dxgiFormatResource;

	D3D12_RESOURCE_FLAGS resFlags = D3D12_RESOURCE_FLAG_NONE;

	td.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	td.Flags = D3D12_RESOURCE_FLAG_NONE;
	td.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	td.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	D3D12_CLEAR_VALUE clearValue;
	memset(&clearValue, 0, sizeof(clearValue));

	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

	clearValue.Format = td.Format;

	memset(&heapProp, 0, sizeof(heapProp));
	heapProp.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProp.CreationNodeMask = 0;
	heapProp.VisibleNodeMask = 0;

	D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE;
	if (isRenderTarget) {
		resFlags = resFlags | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
	}

	td.Flags = resFlags;
	hr = device->device->CreateCommittedResource(&heapProp, heapFlags, &td, initialState, nullptr,
						     IID_PPV_ARGS(&texture));

	resourceState = initialState;
	if (FAILED(hr))
		throw HRError("Failed to create 2D texture", hr);

	if (!isRenderTarget) {
		auto desc = texture->GetDesc();
		uint64_t requiredSize = 0;
		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedTextureDesc;
		std::vector<uint32_t> numRows;
		std::vector<uint64_t> rowSizeInBytes;

		placedTextureDesc.resize(levels * layerCountOrDepth);

		numRows.resize(levels * layerCountOrDepth);

		rowSizeInBytes.resize(levels * layerCountOrDepth);

		device->device->GetCopyableFootprints(&desc, 0, levels * layerCountOrDepth, 0, placedTextureDesc.data(),
						      numRows.data(), rowSizeInBytes.data(), &requiredSize);

		uint32_t bbp = gs_get_format_bpp(format);
		uint32_t rowPitch = bbp * width / 8;

		uint64_t actually_size = rowPitch * height * levels * layerCountOrDepth;

		requiredSize = (requiredSize + (D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1)) &
			       ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
	}

	if (data) {
		BackupTexture(data);
		InitSRD(srd);
	}
}

void gs_texture_2d::UpdateSubresources()
{
	/* if (!needUpdate)
		return;

	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedTextureDesc;
	std::vector<uint32_t> numRows;
	std::vector<uint64_t> rowSizeInBytes;

	placedTextureDesc.resize(levels * layerCountOrDepth);
	numRows.resize(levels * layerCountOrDepth);
	rowSizeInBytes.resize(levels * layerCountOrDepth);

	needUpdate = false;
	auto desc = texture->GetDesc();
	device->device->GetCopyableFootprints(&desc, 0, levels * layerCountOrDepth, 0, placedTextureDesc.data(),
					      numRows.data(), rowSizeInBytes.data(), nullptr);

	if (isDynamic) {
		device->currentCommandContext->TransitionResource(texture, resourceState,
								  D3D12_RESOURCE_STATE_COPY_DEST);
		resourceState = D3D12_RESOURCE_STATE_COPY_DEST;

		if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
			device->currentCommandContext->CommandList()->CopyBufferRegion(
				texture, 0, upload_buffer->resource, placedTextureDesc[0].Offset,
				placedTextureDesc[0].Footprint.Width);
		} else {

			D3D12_TEXTURE_COPY_LOCATION dst;
			dst.pResource = texture;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.PlacedFootprint = {};
			dst.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION src;
			src.pResource = upload_buffer->resource;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = placedTextureDesc[0];
			device->currentCommandContext->CommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
		device->currentCommandContext->TransitionResource(texture, resourceState,
								  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		resourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		return;
	}

	uint8_t *pData = nullptr;
	upload_buffer->resource->Map(0, nullptr, (void **)&pData);

	for (size_t i = 0; i < srd.size(); ++i) {
		uint8_t *pDest = pData + placedTextureDesc[i].Offset;
		uint8_t *src = (uint8_t *)srd[i].pData;

		for (size_t j = 0; j < placedTextureDesc[i].Footprint.Depth; ++j) {
			uint8_t *pDestSlice = pDest + placedTextureDesc[i].Footprint.RowPitch * numRows[i] * j;
			uint8_t *pSrcSlice = src + srd[i].SlicePitch * j;
			for (size_t k = 0; k < numRows[i]; ++k) {
				memcpy(pDestSlice + placedTextureDesc[i].Footprint.RowPitch * k,
				       pSrcSlice + srd[i].RowPitch * k, rowSizeInBytes[i]);
			}
		}
	}

	upload_buffer->resource->Unmap(0, nullptr);

	device->currentCommandContext->TransitionResource(texture, resourceState, D3D12_RESOURCE_STATE_COPY_DEST);
	resourceState = D3D12_RESOURCE_STATE_COPY_DEST;
	if (desc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER) {
		device->currentCommandContext->CommandList()->CopyBufferRegion(texture, 0, upload_buffer->resource,
									       placedTextureDesc[0].Offset,
									       placedTextureDesc[0].Footprint.Width);
	} else {
		for (UINT i = 0; i < srd.size(); ++i) {
			D3D12_TEXTURE_COPY_LOCATION dst;
			dst.pResource = texture;
			dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			dst.PlacedFootprint = {};
			dst.SubresourceIndex = i;

			D3D12_TEXTURE_COPY_LOCATION src;
			src.pResource = upload_buffer->resource;
			src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			src.PlacedFootprint = placedTextureDesc[i];
			device->currentCommandContext->CommandList()->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
		}
	}
	device->currentCommandContext->TransitionResource(texture, resourceState,
							  D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	resourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	*/
}

void gs_texture_2d::InitResourceView()
{
	D3D12_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
	memset(&resourceViewDesc, 0, sizeof(resourceViewDesc));

	resourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	resourceViewDesc.Format = dxgiFormatView;

	if (type == GS_TEXTURE_CUBE) {
		resourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		resourceViewDesc.Texture2D.MipLevels = genMipmaps || !levels ? -1 : levels;
	} else {
		resourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		resourceViewDesc.Texture2D.MipLevels = genMipmaps || !levels ? -1 : levels;
	}
	textureCpuDescriptorHandle = device->cbvSrvUavHeapDescriptor->Allocate();
	device->device->CreateShaderResourceView(texture, &resourceViewDesc, textureCpuDescriptorHandle);
}

void gs_texture_2d::InitRenderTargets()
{
	D3D12_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	memset(&renderTargetViewDesc, 0, sizeof(renderTargetViewDesc));

	bool isMultisampled = sampleCount > 1;
	int32_t layerCount = layerCountOrDepth;

	if (type == GS_TEXTURE_2D) {
		renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		if (isMultisampled)
			renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
		renderTargetViewDesc.Texture2D.MipSlice = 0;
		renderTargetViewDesc.Format = dxgiFormatView;
		renderTargetViewDesc.Texture2D.PlaneSlice = 0;

		renderTargetCpuDescriptorHandle[0] = device->rtvHeapDescriptor->Allocate();
		device->device->CreateRenderTargetView(texture, &renderTargetViewDesc,
						       renderTargetCpuDescriptorHandle[0]);
		if (dxgiFormatView == dxgiFormatViewLinear) {
			renderTargetCpuLinearDescriptorHandle[0] = renderTargetCpuDescriptorHandle[0];
		} else {
			renderTargetViewDesc.Format = dxgiFormatViewLinear;
			renderTargetCpuLinearDescriptorHandle[0] = device->rtvHeapDescriptor->Allocate();
			device->device->CreateRenderTargetView(texture, &renderTargetViewDesc,
							       renderTargetCpuLinearDescriptorHandle[0]);
		}
		return;
	}

	renderTargetViewDesc.Format = dxgiFormatView;
	for (int32_t layerIndex = 0; layerIndex < layerCount; ++layerIndex) {
		for (int32_t levelIndex = 0; levelIndex < levels; ++levelIndex) {
			int32_t currentIndex = levelIndex + (layerIndex * levels);
			renderTargetCpuDescriptorHandle[currentIndex] = device->rtvHeapDescriptor->Allocate();
			renderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
			renderTargetViewDesc.Texture2DArray.FirstArraySlice = layerIndex;
			renderTargetViewDesc.Texture2DArray.MipSlice = levelIndex;
			renderTargetViewDesc.Texture2DArray.ArraySize = 1;
			renderTargetViewDesc.Texture2DArray.PlaneSlice = 0;
			device->device->CreateRenderTargetView(texture, &renderTargetViewDesc,
							       renderTargetCpuDescriptorHandle[currentIndex]);
			if (dxgiFormatView == dxgiFormatViewLinear) {
				renderTargetCpuLinearDescriptorHandle[currentIndex] =
					renderTargetCpuDescriptorHandle[currentIndex];
			} else {
				renderTargetViewDesc.Format = dxgiFormatViewLinear;
				renderTargetCpuLinearDescriptorHandle[currentIndex] =
					device->rtvHeapDescriptor->Allocate();
				device->device->CreateRenderTargetView(
					texture, &renderTargetViewDesc,
					renderTargetCpuLinearDescriptorHandle[currentIndex]);
			}
		}
	}
}

#define SHARED_FLAGS (GS_SHARED_TEX | GS_SHARED_KM_TEX)

gs_texture_2d::gs_texture_2d(gs_device_t *device, uint32_t width, uint32_t height, gs_color_format colorFormat,
			     uint32_t levels, const uint8_t *const *data, uint32_t flags_, gs_texture_type type,
			     bool gdiCompatible, bool twoPlane_)
	: gs_texture(device, gs_type::gs_texture_2d, type, levels, colorFormat),
	  width(width),
	  height(height),
	  flags(flags_),
	  dxgiFormatResource(ConvertGSTextureFormatResource(format)),
	  dxgiFormatView(ConvertGSTextureFormatView(format)),
	  dxgiFormatViewLinear(ConvertGSTextureFormatViewLinear(format)),
	  isRenderTarget((flags_ & GS_RENDER_TARGET) != 0),
	  isDynamic((flags_ & GS_DYNAMIC) != 0),
	  genMipmaps((flags_ & GS_BUILD_MIPMAPS) != 0),
	  twoPlane(twoPlane_)
{
	InitTexture(data);
	InitResourceView();

	if (isRenderTarget)
		InitRenderTargets();
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, ID3D12Resource *nv12tex, uint32_t flags_)
	: gs_texture(device, gs_type::gs_texture_2d, GS_TEXTURE_2D),
	  isRenderTarget((flags_ & GS_RENDER_TARGET) != 0),
	  isDynamic((flags_ & GS_DYNAMIC) != 0),
	  genMipmaps((flags_ & GS_BUILD_MIPMAPS) != 0),
	  twoPlane(true),
	  texture(nv12tex)
{
	td = texture->GetDesc();

	const bool p010 = td.Format == DXGI_FORMAT_P010;
	const DXGI_FORMAT dxgi_format = p010 ? DXGI_FORMAT_R16G16_UNORM : DXGI_FORMAT_R8G8_UNORM;

	this->type = GS_TEXTURE_2D;
	this->format = p010 ? GS_RG16 : GS_R8G8;
	this->flags = flags_;
	this->levels = 1;
	this->device = device;
	this->chroma = true;
	this->width = td.Width / 2;
	this->height = td.Height / 2;
	this->dxgiFormatResource = dxgi_format;
	this->dxgiFormatView = dxgi_format;
	this->dxgiFormatViewLinear = dxgi_format;

	InitResourceView();
	if (isRenderTarget)
		InitRenderTargets();
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, uint32_t handle, bool ntHandle)
	: gs_texture(device, gs_type::gs_texture_2d, GS_TEXTURE_2D)
{
	HRESULT hr;
	hr = device->device->OpenSharedHandle((HANDLE)(uintptr_t)handle, __uuidof(ID3D12Resource),
					      (void **)texture.Assign());

	if (FAILED(hr))
		throw HRError("Failed to open shared 2D texture", hr);

	td = texture->GetDesc();

	const gs_color_format format = ConvertDXGITextureFormat(td.Format);

	this->type = GS_TEXTURE_2D;
	this->format = format;
	this->levels = 1;
	this->device = device;

	this->width = td.Width;
	this->height = td.Height;
	this->dxgiFormatResource = ConvertGSTextureFormatResource(format);
	this->dxgiFormatView = ConvertGSTextureFormatView(format);
	this->dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(format);

	InitResourceView();
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, ID3D12Resource *obj)
	: gs_texture(device, gs_type::gs_texture_2d, GS_TEXTURE_2D)
{
	texture = obj;

	td = texture->GetDesc();

	const gs_color_format format = ConvertDXGITextureFormat(td.Format);

	this->type = GS_TEXTURE_2D;
	this->format = format;
	this->levels = 1;
	this->device = device;

	this->width = td.Width;
	this->height = td.Height;
	this->dxgiFormatResource = ConvertGSTextureFormatResource(format);
	this->dxgiFormatView = ConvertGSTextureFormatView(format);
	this->dxgiFormatViewLinear = ConvertGSTextureFormatViewLinear(format);

	InitResourceView();
}

bool gs_texture_2d::Map(int32_t subresourceIndex, D3D12_MEMCPY_DEST *map)
{
	/** auto desc = texture->GetDesc();
	uint8_t *pData = nullptr;
	std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedTextureDesc;
	std::vector<uint32_t> numRows;
	std::vector<uint64_t> rowSizeInBytes;

	placedTextureDesc.resize(levels * layerCountOrDepth);
	numRows.resize(levels * layerCountOrDepth);
	rowSizeInBytes.resize(levels * layerCountOrDepth);

	device->device->GetCopyableFootprints(&desc, 0, levels * layerCountOrDepth, 0, placedTextureDesc.data(),
					      numRows.data(), rowSizeInBytes.data(), nullptr);

	upload_buffer->resource->Map(0, nullptr, (void **)&pData);
	map->pData = pData + placedTextureDesc[subresourceIndex].Offset;
	map->RowPitch = placedTextureDesc[subresourceIndex].Footprint.RowPitch;
	map->SlicePitch = placedTextureDesc[subresourceIndex].Footprint.RowPitch * numRows[subresourceIndex];
	*/
	return false;
}

void gs_texture_2d::Unmap(int32_t subresourceIndex)
{
}
