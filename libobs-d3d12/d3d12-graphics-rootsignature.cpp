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

gs_root_signature::gs_root_signature(gs_device_t *device_, UINT numRootParams_,
				     UINT numStaticSamplers)
	: device(device_),
	  numParameters(numRootParams_),
	  numInitializedStaticSamplers(0),
	  descriptorTableBitMap(0),
	  samplerTableBitMap(0)
{
	paramArray.resize(numRootParams_);
	samplerArray.resize(numStaticSamplers);
	memset(descriptorTableSize, 0, sizeof(descriptorTableSize));
}

void gs_root_signature::InitStaticSampler(UINT registerIndex, const D3D12_SAMPLER_DESC &nonStaticSamplerDesc,
		       D3D12_SHADER_VISIBILITY Visibility = D3D12_SHADER_VISIBILITY_ALL)
{
	D3D12_STATIC_SAMPLER_DESC &staticSamplerDesc = samplerArray[numInitializedStaticSamplers++];

	staticSamplerDesc.Filter = nonStaticSamplerDesc.Filter;
	staticSamplerDesc.AddressU = nonStaticSamplerDesc.AddressU;
	staticSamplerDesc.AddressV = nonStaticSamplerDesc.AddressV;
	staticSamplerDesc.AddressW = nonStaticSamplerDesc.AddressW;
	staticSamplerDesc.MipLODBias = nonStaticSamplerDesc.MipLODBias;
	staticSamplerDesc.MaxAnisotropy = nonStaticSamplerDesc.MaxAnisotropy;
	staticSamplerDesc.ComparisonFunc = nonStaticSamplerDesc.ComparisonFunc;
	staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplerDesc.MinLOD = nonStaticSamplerDesc.MinLOD;
	staticSamplerDesc.MaxLOD = nonStaticSamplerDesc.MaxLOD;
	staticSamplerDesc.ShaderRegister = registerIndex;
	staticSamplerDesc.RegisterSpace = 0;
	staticSamplerDesc.ShaderVisibility = Visibility;

	if (staticSamplerDesc.AddressU == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
	    staticSamplerDesc.AddressV == D3D12_TEXTURE_ADDRESS_MODE_BORDER ||
	    staticSamplerDesc.AddressW == D3D12_TEXTURE_ADDRESS_MODE_BORDER) {

		// Transparent Black
		// nonStaticSamplerDesc.BorderColor[0] = 0.0f;
		// nonStaticSamplerDesc.BorderColor[1] = 0.0f;
		// nonStaticSamplerDesc.BorderColor[2] = 0.0f;
		// nonStaticSamplerDesc.BorderColor[3] = 0.0f;

				// Opaque Black
		// nonStaticSamplerDesc.BorderColor[0] = 0.0f;
		// nonStaticSamplerDesc.BorderColor[1] = 0.0f;
		// nonStaticSamplerDesc.BorderColor[2] = 0.0f;
		// nonStaticSamplerDesc.BorderColor[3] = 1.0f;
		
				// Opaque White
		// nonStaticSamplerDesc.BorderColor[0] = 1.0f;
		// nonStaticSamplerDesc.BorderColor[1] = 1.0f;
		// nonStaticSamplerDesc.BorderColor[2] = 1.0f;
		// nonStaticSamplerDesc.BorderColor[3] = 1.0f;

		if (nonStaticSamplerDesc.BorderColor[3] == 1.0f) {
			if (nonStaticSamplerDesc.BorderColor[0] == 1.0f)
				staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
			else
				staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
		} else {
			staticSamplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		}
	}
}

void gs_root_signature::Reset(UINT numRootParams_, UINT numStaticSamplers_ = 1) {

	paramArray.resize(numRootParams_);
	samplerArray.resize(numStaticSamplers_);
	memset(descriptorTableSize, 0, sizeof(descriptorTableSize));
	numParameters = numRootParams_;
	numSamplers = numStaticSamplers_;
	numInitializedStaticSamplers = 0;
}

void gs_root_signature::InitParamWith(gs_vertex_shader *vertexShader, gs_pixel_shader *pixelShader) {
	if (vertexShader->uniform32BitBufferCount > 0) {
		numParameters += 1;
	}

	if (pixelShader->samplerCount > 0) {
		numSamplers += 1;
	}

	if (pixelShader->textureCount > 0) {
		numParameters += 1;
	}

	if (pixelShader->uniform32BitBufferCount > 0) {
		numParameters += 1;
	}

	Reset(numParameters, numSamplers);
	if (vertexShader->uniform32BitBufferCount > 0) {
		RootParameter(0).InitAsConstants(0, 1, D3D12_SHADER_VISIBILITY_VERTEX);
	}

	if (pixelShader->samplerCount > 0) {
		for (int32_t i = 0; i < pixelShader->samplerCount; ++i) {
			InitStaticSampler(i, pixelShader->samplers[i]->GetDesc());
		}
	}

	if (pixelShader->textureCount > 0) {
		RootParameter(1).InitAsDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 0, pixelShader->textureCount);
	}

	if (pixelShader->uniform32BitBufferCount > 0) {
		RootParameter(2).InitAsConstants(1, 1, D3D12_SHADER_VISIBILITY_PIXEL);
	}
}

void gs_root_signature::Build(const std::wstring &name, D3D12_ROOT_SIGNATURE_FLAGS flags)
{

	D3D12_ROOT_SIGNATURE_DESC RootDesc;
	RootDesc.NumParameters = numParameters;
	RootDesc.pParameters = (const D3D12_ROOT_PARAMETER *)paramArray.data();
	RootDesc.NumStaticSamplers = numSamplers;
	RootDesc.pStaticSamplers = (const D3D12_STATIC_SAMPLER_DESC *)samplerArray.data();
	RootDesc.Flags = flags;

	descriptorTableBitMap = 0;
	samplerTableBitMap = 0;

	for (UINT param = 0; param < numParameters; ++param) {
		const D3D12_ROOT_PARAMETER &rootParam = RootDesc.pParameters[param];
		descriptorTableSize[param] = 0;

		if (rootParam.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE) {
			if (rootParam.DescriptorTable.pDescriptorRanges->RangeType ==
			    D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER)
				samplerTableBitMap |= (1 << param);
			else
				descriptorTableBitMap |= (1 << param);

			for (UINT tableRange = 0; tableRange < rootParam.DescriptorTable.NumDescriptorRanges;
			     ++tableRange)
				descriptorTableSize[param] +=
					rootParam.DescriptorTable.pDescriptorRanges[tableRange].NumDescriptors;
		}
	}


	ComPtr<ID3DBlob> pOutBlob, pErrorBlob;

	D3D12SerializeRootSignature(&RootDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pOutBlob, &pErrorBlob);
	device->device->CreateRootSignature(
			1, pOutBlob->GetBufferPointer(), pOutBlob->GetBufferSize(), IID_PPV_ARGS(&signature));

	signature->SetName(name.c_str());
}

gs_root_signature::~gs_root_signature() {
	Release();
}

void gs_root_signature::Release() {}
