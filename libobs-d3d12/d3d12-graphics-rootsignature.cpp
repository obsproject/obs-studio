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

#define MAX_ROOT_SIGNATURE_PARAMETERS 64
#define VIEW_GPU_DESCRIPTOR_COUNT 65536

gs_graphics_rootsignature::gs_graphics_rootsignature(ID3D12Device *device, gs_vertex_shader *vertexShader,
						     gs_pixel_shader *pixelShader)
{
	D3D12_ROOT_PARAMETER rootParameters[MAX_ROOT_SIGNATURE_PARAMETERS];
	D3D12_DESCRIPTOR_RANGE descriptorRanges[MAX_ROOT_SIGNATURE_PARAMETERS];
	int32_t parameterCount = 0;
	int32_t rangeCount = 0;
	D3D12_DESCRIPTOR_RANGE descriptorRange;
	D3D12_ROOT_PARAMETER rootParameter;
	HRESULT hr = S_FALSE;

	memset(&rootParameters, 0, sizeof(rootParameters));
	memset(&descriptorRanges, 0, sizeof(descriptorRanges));
	memset(&descriptorRange, 0, sizeof(descriptorRange));
	memset(&rootParameter, 0, sizeof(rootParameter));

	if (vertexShader->uniform32BitBufferCount > 0) {
		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParameter.Constants.Num32BitValues = vertexShader->uniform32BitBufferCount;
		rootParameter.Constants.ShaderRegister = 0;
		rootParameter.Constants.RegisterSpace = 0;
		rootParameters[parameterCount] = rootParameter;
		vertexUniform32BitBufferRootIndex = parameterCount;
		parameterCount += 1;
	}

	if (pixelShader->samplerCount > 0) {
		// Fragment Samplers
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
		descriptorRange.NumDescriptors = pixelShader->samplerCount;
		descriptorRange.BaseShaderRegister = 0;
		descriptorRange.RegisterSpace = 0;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		descriptorRanges[rangeCount] = descriptorRange;

		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameter.DescriptorTable.NumDescriptorRanges = 1;
		rootParameter.DescriptorTable.pDescriptorRanges = &descriptorRanges[rangeCount];
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[parameterCount] = rootParameter;
		pixelSamplerRootIndex = parameterCount;
		rangeCount += 1;
		parameterCount += 1;
	}

	if (pixelShader->textureCount > 0) {
		// Fragment Storage Textures
		descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptorRange.NumDescriptors = pixelShader->textureCount;
		descriptorRange.BaseShaderRegister = 0;
		descriptorRange.RegisterSpace = 0;
		descriptorRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		descriptorRanges[rangeCount] = descriptorRange;

		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParameter.DescriptorTable.NumDescriptorRanges = 1;
		rootParameter.DescriptorTable.pDescriptorRanges = &descriptorRanges[rangeCount];
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameters[parameterCount] = rootParameter;
		pixelTextureRootIndex = parameterCount;
		rangeCount += 1;
		parameterCount += 1;
	}

	if (pixelShader->uniform32BitBufferCount > 0) {
		rootParameter.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		rootParameter.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParameter.Constants.Num32BitValues = pixelShader->uniform32BitBufferCount;
		rootParameter.Constants.ShaderRegister = 0;
		rootParameter.Constants.RegisterSpace = 0;
		rootParameters[parameterCount] = rootParameter;
		pixelUniform32BitBufferRootIndex = parameterCount;
		parameterCount += 1;
	}

	if (parameterCount > MAX_ROOT_SIGNATURE_PARAMETERS)
		throw HRError("Failed to create rootSignature, parameterCount is too long", hr);

	if (rangeCount > MAX_ROOT_SIGNATURE_PARAMETERS)
		throw HRError("Failed to create rootSignature, rangeCount is too long", hr);

	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.NumParameters = parameterCount;
	rootSignatureDesc.pParameters = rootParameters;
	rootSignatureDesc.NumStaticSamplers = 0;
	rootSignatureDesc.pStaticSamplers = NULL;
	rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

	ID3DBlob *serializedRootSignature;
	ID3DBlob *errorBlob;
	hr = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &serializedRootSignature,
					 &errorBlob);

	if (FAILED(hr)) {
		std::string throwStr = "Failed SerializeRootSignature errStr:";
		if (errorBlob) {
			std::string errStr((const char *)errorBlob->GetBufferPointer(), errorBlob->GetBufferSize());
			throwStr += errStr;
		}
		throw HRError(throwStr.c_str(), hr);
	}

	hr = device->CreateRootSignature(0, serializedRootSignature->GetBufferPointer(),
					 serializedRootSignature->GetBufferSize(), IID_PPV_ARGS(&rootSignature));

	if (FAILED(hr))
		throw HRError("Failed to create rootSignature", hr);
}
