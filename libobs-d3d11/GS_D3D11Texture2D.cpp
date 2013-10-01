/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include "util/base.h"
#include "GS_D3D11SubSystem.hpp"

static inline bool IsPow2(uint32_t num)
{
	return num >= 2 && (num & (num-1)) == 0;
}

void gs_texture_2d::InitSRD(D3D11_SUBRESOURCE_DATA *srd, void *data)
{
	uint32_t rowSizeBytes = width * GetFormatBPP(format);
	uint32_t texSizeBytes = rowSizeBytes * height;

	if (type == GS_TEXTURE_2D) {
		srd->pSysMem          = (uint8_t*)data;
		srd->SysMemPitch      = rowSizeBytes; 
		srd->SysMemSlicePitch = texSizeBytes;

	} else { /* GS_TEXTURE_CUBE */
		void **buffers = (void**)data;
		for (size_t i = 0; i < 6; i++) {
			srd[i].pSysMem          = buffers[i];
			srd[i].SysMemPitch      = rowSizeBytes;
			srd[i].SysMemSlicePitch = texSizeBytes;
		}
	}
}

void gs_texture_2d::InitTexture(void *data)
{
	D3D11_TEXTURE2D_DESC td;
	D3D11_SUBRESOURCE_DATA srd[6];
	HRESULT hr;

	memset(&td, 0, sizeof(td));
	td.Width            = width;
	td.Height           = height;
	td.MipLevels        = genMipmaps ? 0 : 1;
	td.ArraySize        = type == GS_TEXTURE_CUBE ? 6 : 1;
	td.Format           = dxgiFormat;
	td.BindFlags        = D3D10_BIND_SHADER_RESOURCE;
	td.SampleDesc.Count = 1;
	td.CPUAccessFlags   = isDynamic ? D3D11_CPU_ACCESS_WRITE : 0;
	td.Usage            = isDynamic ? D3D11_USAGE_DYNAMIC :
	                                  D3D11_USAGE_DEFAULT;

	if (type == GS_TEXTURE_CUBE)
		td.MiscFlags |= D3D11_RESOURCE_MISC_TEXTURECUBE;

	if (isRenderTarget || isGDICompatible)
		td.BindFlags |= D3D10_BIND_RENDER_TARGET;

	if (data)
		InitSRD(srd, data);

	hr = device->device->CreateTexture2D(&td, data ? srd : NULL,
			texture.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create 2D texture", hr);
}

void gs_texture_2d::InitResourceView()
{
	D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
	HRESULT hr;

	memset(&resourceDesc, 0, sizeof(resourceDesc));
	resourceDesc.Format = dxgiFormat;

	if (type == GS_TEXTURE_CUBE) {
		resourceDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURECUBE;
		resourceDesc.TextureCube.MipLevels = genMipmaps ? -1 : 1;
	} else {
		resourceDesc.ViewDimension = D3D10_SRV_DIMENSION_TEXTURE2D;
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

gs_texture_2d::gs_texture_2d(device_t device, uint32_t width, uint32_t height,
		gs_color_format colorFormat, void *data, uint32_t flags,
		bool isCubeMap, bool gdiCompatible, bool shared)
	: gs_texture (device, isCubeMap ? GS_TEXTURE_CUBE : GS_TEXTURE_2D),
	  width           (width),
	  height          (height),
	  format          (colorFormat),
	  dxgiFormat      (ConvertGSTextureFormat(format)),
	  isGDICompatible (gdiCompatible),
	  isShared        (shared),
	  isDynamic       ((flags & GS_DYNAMIC) != 0),
	  isRenderTarget  ((flags & GS_RENDERTARGET) != 0),
	  genMipmaps      ((flags & GS_BUILDMIPMAPS) != 0)
{
	if (genMipmaps && (!IsPow2(width) || !IsPow2(height))) {
		blog(LOG_WARNING, "Cannot generate mipmaps for a "
		                  "non-power-of-two texture.  Disabling "
		                  "mipmaps for this texture.");
		genMipmaps = false;
	}

	InitTexture(data);
	InitResourceView();

	if (isRenderTarget)
		InitRenderTargets();
}
