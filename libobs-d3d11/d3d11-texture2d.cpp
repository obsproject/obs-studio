/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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
#include "d3d11-subsystem.hpp"

void gs_texture_2d::InitSRD(vector<D3D11_SUBRESOURCE_DATA> &srd)
{
	uint32_t rowSizeBytes  = width  * gs_get_format_bpp(format);
	uint32_t texSizeBytes  = height * rowSizeBytes / 8;
	size_t   textures      = type == GS_TEXTURE_2D ? 1 : 6;
	uint32_t actual_levels = levels;
	size_t   curTex = 0;

	if (!actual_levels)
		actual_levels = gs_get_total_levels(width, height);

	rowSizeBytes /= 8;

	for (size_t i = 0; i < textures; i++) {
		uint32_t newRowSize = rowSizeBytes;
		uint32_t newTexSize = texSizeBytes;

		for (uint32_t j = 0; j < actual_levels; j++) {
			D3D11_SUBRESOURCE_DATA newSRD;
			newSRD.pSysMem          = data[curTex++].data();
			newSRD.SysMemPitch      = newRowSize;
			newSRD.SysMemSlicePitch = newTexSize;
			srd.push_back(newSRD);

			newRowSize /= 2;
			newTexSize /= 4;
		}
	}
}

void gs_texture_2d::BackupTexture(const uint8_t **data)
{
	this->data.resize(levels);

	uint32_t w = width;
	uint32_t h = height;
	uint32_t bbp = gs_get_format_bpp(format);

	for (uint32_t i = 0; i < levels; i++) {
		if (!data[i])
			break;

		uint32_t texSize = bbp * w * h / 8;
		this->data[i].resize(texSize);

		vector<uint8_t> &subData = this->data[i];
		memcpy(&subData[0], data[i], texSize);

		w /= 2;
		h /= 2;
	}
}

void gs_texture_2d::InitTexture(const uint8_t **data)
{
	HRESULT hr;

	memset(&td, 0, sizeof(td));
	td.Width            = width;
	td.Height           = height;
	td.MipLevels        = genMipmaps ? 0 : levels;
	td.ArraySize        = type == GS_TEXTURE_CUBE ? 6 : 1;
	td.Format           = dxgiFormat;
	td.BindFlags        = D3D11_BIND_SHADER_RESOURCE;
	td.SampleDesc.Count = 1;
	td.CPUAccessFlags   = isDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	td.Usage            = isDynamic ? D3D11_USAGE_DYNAMIC :
	                                  D3D11_USAGE_DEFAULT;

	if (type == GS_TEXTURE_CUBE)
		td.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

	if (isRenderTarget || isGDICompatible)
		td.BindFlags |= D3D11_BIND_RENDER_TARGET;

	if (isGDICompatible)
		td.MiscFlags |= D3D11_RESOURCE_MISC_GDI_COMPATIBLE;

	if (data) {
		BackupTexture(data);
		InitSRD(srd);
	}

	hr = device->device->CreateTexture2D(&td, data ? srd.data() : NULL,
			texture.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create 2D texture", hr);

	if (isGDICompatible) {
		hr = texture->QueryInterface(__uuidof(IDXGISurface1),
				(void**)gdiSurface.Assign());
		if (FAILED(hr))
			throw HRError("Failed to create GDI surface", hr);
	}
}

void gs_texture_2d::InitResourceView()
{
	HRESULT hr;

	memset(&resourceDesc, 0, sizeof(resourceDesc));
	resourceDesc.Format = dxgiFormat;

	if (type == GS_TEXTURE_CUBE) {
		resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
		resourceDesc.TextureCube.MipLevels = genMipmaps ? -1 : 1;
	} else {
		resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MipLevels = genMipmaps ? -1 : 1;
	}

	hr = device->device->CreateShaderResourceView(texture, &resourceDesc,
			shaderRes.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create resource view", hr);
}

void gs_texture_2d::InitRenderTargets()
{
	HRESULT hr;
	if (type == GS_TEXTURE_2D) {
		hr = device->device->CreateRenderTargetView(texture, NULL,
				renderTarget[0].Assign());
		if (FAILED(hr))
			throw HRError("Failed to create render target view",
					hr);
	} else {
		D3D11_RENDER_TARGET_VIEW_DESC rtv;
		rtv.Format = dxgiFormat;
		rtv.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
		rtv.Texture2DArray.MipSlice  = 0;
		rtv.Texture2DArray.ArraySize = 1;

		for (UINT i = 0; i < 6; i++) {
			rtv.Texture2DArray.FirstArraySlice = i;
			hr = device->device->CreateRenderTargetView(texture,
					&rtv, renderTarget[i].Assign());
			if (FAILED(hr))
				throw HRError("Failed to create cube render "
				              "target view", hr);
		}
	}
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, uint32_t width,
		uint32_t height, gs_color_format colorFormat, uint32_t levels,
		const uint8_t **data, uint32_t flags, gs_texture_type type,
		bool gdiCompatible, bool shared)
	: gs_texture      (device, gs_type::gs_texture_2d, type, levels,
	                   colorFormat),
	  width           (width),
	  height          (height),
	  dxgiFormat      (ConvertGSTextureFormat(format)),
	  isRenderTarget  ((flags & GS_RENDER_TARGET) != 0),
	  isGDICompatible (gdiCompatible),
	  isDynamic       ((flags & GS_DYNAMIC) != 0),
	  isShared        (shared),
	  genMipmaps      ((flags & GS_BUILD_MIPMAPS) != 0)
{
	InitTexture(data);
	InitResourceView();

	if (isRenderTarget)
		InitRenderTargets();
}

gs_texture_2d::gs_texture_2d(gs_device_t *device, uint32_t handle)
	: gs_texture      (device, gs_type::gs_texture_2d,
	                   GS_TEXTURE_2D),
	  isShared        (true),
	  sharedHandle    (handle)
{
	HRESULT hr;
	hr = device->device->OpenSharedResource((HANDLE)(uintptr_t)handle,
			__uuidof(ID3D11Texture2D), (void**)texture.Assign());
	if (FAILED(hr))
		throw HRError("Failed to open shared 2D texture", hr);

	texture->GetDesc(&td);

	this->type       = GS_TEXTURE_2D;
	this->format     = ConvertDXGITextureFormat(td.Format);
	this->levels     = 1;
	this->device     = device;

	this->width      = td.Width;
	this->height     = td.Height;
	this->dxgiFormat = td.Format;

	memset(&resourceDesc, 0, sizeof(resourceDesc));
	resourceDesc.Format              = td.Format;
	resourceDesc.ViewDimension       = D3D11_SRV_DIMENSION_TEXTURE2D;
	resourceDesc.Texture2D.MipLevels = 1;

	hr = device->device->CreateShaderResourceView(texture, &resourceDesc,
			shaderRes.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create shader resource view", hr);
}
