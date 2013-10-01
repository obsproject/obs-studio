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

#include "GS_D3D11SubSystem.hpp"
#include "graphics/vec4.h"

const D3D11_TEXTURE_ADDRESS_MODE convertAddressMode[] =
{
	D3D11_TEXTURE_ADDRESS_CLAMP,
	D3D11_TEXTURE_ADDRESS_WRAP,
	D3D11_TEXTURE_ADDRESS_MIRROR,
	D3D11_TEXTURE_ADDRESS_BORDER,
	D3D11_TEXTURE_ADDRESS_MIRROR_ONCE
};
	
const D3D11_FILTER convertFilter[] =
{
	D3D11_FILTER_MIN_MAG_MIP_LINEAR,
	D3D11_FILTER_MIN_MAG_MIP_POINT,
	D3D11_FILTER_ANISOTROPIC,
	D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR,
	D3D11_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT,
	D3D11_FILTER_MIN_POINT_MAG_MIP_LINEAR,
	D3D11_FILTER_MIN_LINEAR_MAG_MIP_POINT,
	D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR,
	D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT
};

gs_sampler_state::gs_sampler_state(device_t device, gs_sampler_info *info)
	: device (device),
	  info   (*info)
{
	D3D11_SAMPLER_DESC sd;
	HRESULT hr;
	vec4 v4;

	memset(&sd, 0, sizeof(sd));
	sd.AddressU       = convertAddressMode[(uint32_t)info->address_u];
	sd.AddressV       = convertAddressMode[(uint32_t)info->address_v];
	sd.AddressW       = convertAddressMode[(uint32_t)info->address_w];
	sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sd.Filter         = convertFilter[(uint32_t)info->filter];
	sd.MaxAnisotropy  = info->max_anisotropy;
	sd.MaxLOD         = FLT_MAX;

	vec4_from_rgba(&v4, info->border_color);
	memcpy(sd.BorderColor, v4.ptr, sizeof(v4));

	hr = device->device->CreateSamplerState(&sd, state.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create sampler state", hr);
}
