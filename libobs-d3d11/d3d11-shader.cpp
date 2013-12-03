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

#include "d3d11-subsystem.hpp"
#include "d3d11-shaderprocessor.hpp"
#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/matrix3.h>
#include <graphics/matrix4.h>

void gs_vertex_shader::GetBuffersExpected(
		const vector<D3D11_INPUT_ELEMENT_DESC> &inputs)
{
	for (size_t i = 0; i < inputs.size(); i++) {
		const D3D11_INPUT_ELEMENT_DESC &input = inputs[i];
		if (strcmp(input.SemanticName, "NORMAL") == 0)
			hasNormals = true;
		else if (strcmp(input.SemanticName, "TANGENT") == 0)
			hasTangents = true;
		else if (strcmp(input.SemanticName, "COLOR") == 0)
			hasColors = true;
		else if (strcmp(input.SemanticName, "TEXCOORD") == 0)
			nTexUnits++;
	}
}

gs_vertex_shader::gs_vertex_shader(device_t device, const char *file,
		const char *shaderString)
	: gs_shader   (device, SHADER_VERTEX),
	  hasNormals  (false),
	  hasColors   (false),
	  hasTangents (false),
	  nTexUnits   (0)
{
	vector<D3D11_INPUT_ELEMENT_DESC> inputs;
	ShaderProcessor    processor(device);
	ComPtr<ID3D10Blob> shaderBlob;
	string             outputString;
	HRESULT            hr;

	processor.Process(shaderString, file);
	processor.BuildString(outputString);
	processor.BuildParams(params);
	processor.BuildInputLayout(inputs);
	GetBuffersExpected(inputs);
	BuildConstantBuffer();

	Compile(outputString.c_str(), file, "vs_4_0", shaderBlob.Assign());

	hr = device->device->CreateVertexShader(shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize(), NULL, shader.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create vertex shader", hr);

	hr = device->device->CreateInputLayout(inputs.data(),
			(UINT)inputs.size(), shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize(), layout.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create input layout", hr);

	viewProj = shader_getparambyname(this, "ViewProj");
	world    = shader_getparambyname(this, "World");
}

gs_pixel_shader::gs_pixel_shader(device_t device, const char *file,
		const char *shaderString)
	: gs_shader(device, SHADER_PIXEL)
{
	ShaderProcessor    processor(device);
	ComPtr<ID3D10Blob> shaderBlob;
	string             outputString;
	HRESULT            hr;

	processor.Process(shaderString, file);
	processor.BuildString(outputString);
	processor.BuildParams(params);
	processor.BuildSamplers(samplers);
	BuildConstantBuffer();

	Compile(outputString.c_str(), file, "ps_4_0", shaderBlob.Assign());

	hr = device->device->CreatePixelShader(shaderBlob->GetBufferPointer(),
			shaderBlob->GetBufferSize(), NULL, shader.Assign());
	if (FAILED(hr))
		throw HRError("Failed to create vertex shader", hr);
}

void gs_shader::BuildConstantBuffer()
{
	for (size_t i = 0; i < params.size(); i++) {
		shader_param &param = params[i];
		switch (param.type) {
		case SHADER_PARAM_BOOL:
		case SHADER_PARAM_INT:
		case SHADER_PARAM_FLOAT: constantSize += sizeof(float); break;
		case SHADER_PARAM_VEC2:  constantSize += sizeof(vec2); break;
		case SHADER_PARAM_VEC3:  constantSize += sizeof(float)*3; break;
		case SHADER_PARAM_VEC4:  constantSize += sizeof(vec4); break;
		case SHADER_PARAM_MATRIX4X4:
			constantSize += sizeof(float)*4*4;
		}
	}

	if (constantSize) {
		D3D11_BUFFER_DESC bd;
		HRESULT hr;

		memset(&bd, 0, sizeof(bd));
		bd.ByteWidth      = (constantSize+15)&0xFFFFFFF0; /* align */
		bd.Usage          = D3D11_USAGE_DYNAMIC;
		bd.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		hr = device->device->CreateBuffer(&bd, NULL,
				constants.Assign());
		if (FAILED(hr))
			throw HRError("Failed to create constant buffer", hr);
	}

	for (size_t i = 0; i < params.size(); i++)
		shader_setdefault(this, &params[i]);
}

void gs_shader::Compile(const char *shaderString, const char *file,
		const char *target, ID3D10Blob **shader)
{
	ComPtr<ID3D10Blob> errorsBlob;
	HRESULT hr;

	if (!shaderString)
		throw "No shader string specified";

	hr = D3DCompile(shaderString, strlen(shaderString), file, NULL, NULL,
			"main", target,
			D3D10_SHADER_OPTIMIZATION_LEVEL1, 0,
			shader, errorsBlob.Assign());
	if (FAILED(hr)) {
		if (errorsBlob != NULL && errorsBlob->GetBufferSize())
			throw ShaderError(errorsBlob, hr);
		else
			throw HRError("Failed to compile shader", hr);
	}
}

inline void gs_shader::UpdateParam(vector<uint8_t> &constData,
		shader_param &param, bool &upload)
{
	if (param.type != SHADER_PARAM_TEXTURE) {
		if (!param.curValue.size())
			throw "Not all shader parameters were set";

		constData.insert(constData.end(),
				param.curValue.begin(),
				param.curValue.end());

		if (param.changed) {
			upload = true;
			param.changed = false;
		}
	} else if (param.curValue.size() == sizeof(texture_t)) {
		texture_t tex;
		memcpy(&tex, param.curValue.data(), sizeof(texture_t));
		device_load_texture(device, tex, param.textureID);
	}
}

void gs_shader::UploadParams()
{
	vector<uint8_t> constData;
	bool            upload = false;

	constData.reserve(constantSize);

	for (size_t i = 0; i < params.size(); i++)
		UpdateParam(constData, params[i], upload);

	if (constData.size() != constantSize)
		throw "Invalid constant data size given to shader";

	if (upload) {
		D3D11_MAPPED_SUBRESOURCE map;
		HRESULT hr;

		hr = device->context->Map(constants, 0, D3D11_MAP_WRITE_DISCARD,
				0, &map);
		if (FAILED(hr))
			throw HRError("Could not lock constant buffer", hr);

		memcpy(map.pData, constData.data(), constData.size());
		device->context->Unmap(constants, 0);
	}
}

void shader_destroy(shader_t shader)
{
	delete shader;
}

int shader_numparams(shader_t shader)
{
	return (int)shader->params.size();
}

sparam_t shader_getparambyidx(shader_t shader, uint32_t param)
{
	return &shader->params[param];
}

sparam_t shader_getparambyname(shader_t shader, const char *name)
{
	for (size_t i = 0; i < shader->params.size(); i++) {
		shader_param &param = shader->params[i];
		if (strcmp(param.name.c_str(), name) == 0)
			return &param;
	}

	return NULL;
}

void shader_getparaminfo(shader_t shader, sparam_t param,
		struct shader_param_info *info)
{
	if (!param || !shader)
		return;

	info->name = param->name.c_str();
	info->type = param->type;
}

sparam_t shader_getviewprojmatrix(shader_t shader)
{
	if (shader->type != SHADER_VERTEX)
		return NULL;

	return static_cast<gs_vertex_shader*>(shader)->viewProj;
}

sparam_t shader_getworldmatrix(shader_t shader)
{
	if (shader->type != SHADER_VERTEX)
		return NULL;

	return static_cast<gs_vertex_shader*>(shader)->world;
}

static inline void shader_setval_inline(gs_shader *shader, shader_param *param,
		const void *data, size_t size)
{
	assert(param);
	if (!param)
		return;

	bool size_changed = param->curValue.size() != size;
	if (size_changed)
		param->curValue.resize(size);

	if (size_changed || memcmp(param->curValue.data(), data, size) != 0) {
		memcpy(param->curValue.data(), data, size);
		param->changed = true;
	}
}

void shader_setbool(shader_t shader, sparam_t param, bool val)
{
	shader_setval_inline(shader, param, &val, sizeof(bool));
}

void shader_setfloat(shader_t shader, sparam_t param, float val)
{
	shader_setval_inline(shader, param, &val, sizeof(float));
}

void shader_setint(shader_t shader, sparam_t param, int val)
{
	shader_setval_inline(shader, param, &val, sizeof(int));
}

void shader_setmatrix3(shader_t shader, sparam_t param,
		const struct matrix3 *val)
{
	struct matrix4 mat;
	matrix4_from_matrix3(&mat, val);
	shader_setval_inline(shader, param, &mat, sizeof(matrix4));
}

void shader_setmatrix4(shader_t shader, sparam_t param,
		const struct matrix4 *val)
{
	shader_setval_inline(shader, param, val, sizeof(matrix4));
}

void shader_setvec2(shader_t shader, sparam_t param, const struct vec2 *val)
{
	shader_setval_inline(shader, param, val, sizeof(vec2));
}

void shader_setvec3(shader_t shader, sparam_t param, const struct vec3 *val)
{
	shader_setval_inline(shader, param, val, sizeof(float) * 3);
}

void shader_setvec4(shader_t shader, sparam_t param, const struct vec4 *val)
{
	shader_setval_inline(shader, param, val, sizeof(vec4));
}

void shader_settexture(shader_t shader, sparam_t param, texture_t val)
{
	shader_setval_inline(shader, param, &val, sizeof(texture_t));
}

void shader_setval(shader_t shader, sparam_t param, const void *val,
		size_t size)
{
	shader_setval_inline(shader, param, val, size);
}

void shader_setdefault(shader_t shader, sparam_t param)
{
	if (param->defaultValue.size())
		shader_setval_inline(shader, param, param->defaultValue.data(),
				param->defaultValue.size());
}
