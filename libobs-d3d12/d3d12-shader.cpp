#include "d3d12-subsystem.hpp"
#include "d3d12-shaderprocessor.hpp"
#include <graphics/vec2.h>
#include <graphics/vec3.h>
#include <graphics/matrix3.h>
#include <graphics/matrix4.h>
#include <util/platform.h>
#include <util/util.hpp>

#include <filesystem>
#include <fstream>
#include <d3dcompiler.h>

void gs_vertex_shader::GetBuffersExpected(const std::vector<D3D12_INPUT_ELEMENT_DESC> &inputs)
{
	for (size_t i = 0; i < inputs.size(); i++) {
		const D3D12_INPUT_ELEMENT_DESC &input = inputs[i];
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

gs_vertex_shader::gs_vertex_shader(gs_device_t *device, const char *file, const char *shaderString)
	: gs_shader(device, gs_type::gs_vertex_shader, GS_SHADER_VERTEX),
	  hasNormals(false),
	  hasColors(false),
	  hasTangents(false),
	  nTexUnits(0)
{
	ShaderProcessor processor(device);
	ComPtr<ID3D10Blob> shaderBlob;
	std::string outputString;

	processor.Process(shaderString, file);
	processor.BuildString(outputString);
	processor.BuildParams(params);
	processor.BuildInputLayout(layoutData);
	GetBuffersExpected(layoutData);
	BuildConstantBuffer();

	Compile(outputString.c_str(), file, "vs_4_0", shaderBlob.Assign());

	data.resize(shaderBlob->GetBufferSize());
	memcpy(&data[0], shaderBlob->GetBufferPointer(), data.size());

	viewProj = gs_shader_get_param_by_name(this, "ViewProj");
	world = gs_shader_get_param_by_name(this, "World");
}

gs_pixel_shader::gs_pixel_shader(gs_device_t *device, const char *file, const char *shaderString)
	: gs_shader(device, gs_type::gs_pixel_shader, GS_SHADER_PIXEL)
{
	ShaderProcessor processor(device);
	ComPtr<ID3D10Blob> shaderBlob;
	std::string outputString;

	processor.Process(shaderString, file);
	processor.BuildString(outputString);
	processor.BuildParams(params);
	processor.BuildSamplers(samplers);

	samplerCount = samplers.size();

	BuildConstantBuffer();
	Compile(outputString.c_str(), file, "ps_4_0", shaderBlob.Assign());

	data.resize(shaderBlob->GetBufferSize());
	memcpy(&data[0], shaderBlob->GetBufferPointer(), data.size());
}

/*
 * Shader compilers will pack constants in to single registers when possible.
 * For example:
 *
 *   uniform float3 test1;
 *   uniform float  test2;
 *
 * will inhabit a single constant register (c0.xyz for 'test1', and c0.w for
 * 'test2')
 *
 * However, if two constants cannot inhabit the same register, the second one
 * must begin at a new register, for example:
 *
 *   uniform float2 test1;
 *   uniform float3 test2;
 *
 * 'test1' will inhabit register constant c0.xy.  However, because there's no
 * room for 'test2, it must use a new register constant entirely (c1.xyz).
 *
 * So if we want to calculate the position of the constants in the constant
 * buffer, we must take this in to account.
 */

void gs_shader::BuildConstantBuffer()
{
	int32_t textures = 0;
	for (size_t i = 0; i < params.size(); i++) {
		gs_shader_param &param = params[i];
		size_t size = 0;

		switch (param.type) {
		case GS_SHADER_PARAM_BOOL:
		case GS_SHADER_PARAM_INT:
		case GS_SHADER_PARAM_FLOAT:
			size = sizeof(float);
			break;
		case GS_SHADER_PARAM_INT2:
		case GS_SHADER_PARAM_VEC2:
			size = sizeof(vec2);
			break;
		case GS_SHADER_PARAM_INT3:
		case GS_SHADER_PARAM_VEC3:
			size = sizeof(float) * 3;
			break;
		case GS_SHADER_PARAM_INT4:
		case GS_SHADER_PARAM_VEC4:
			size = sizeof(vec4);
			break;
		case GS_SHADER_PARAM_MATRIX4X4:
			size = sizeof(float) * 4 * 4;
			break;
		case GS_SHADER_PARAM_TEXTURE:
			++textures;
			continue;
		case GS_SHADER_PARAM_STRING:
		case GS_SHADER_PARAM_UNKNOWN:
			continue;
		}

		if (param.arrayCount)
			size *= param.arrayCount;

		if (size && (constantSize & 15) != 0) {
			size_t alignMax = (constantSize + 15) & ~15;
			if ((size + constantSize) > alignMax) {
				constantSize = alignMax;
			}
		}

		param.pos = constantSize;
		constantSize += size;
	}

	if (constantSize > 0) {
		hasDynamicUniformConstantBuffer = true; /* align */
	}

	textureCount = textures;

	for (size_t i = 0; i < params.size(); i++)
		gs_shader_set_default(&params[i]);
}

static uint64_t fnv1a_hash(const char *str, size_t len)
{
	const uint64_t FNV_OFFSET = 14695981039346656037ULL;
	const uint64_t FNV_PRIME = 1099511628211ULL;
	uint64_t hash = FNV_OFFSET;
	for (size_t i = 0; i < len; i++) {
		hash ^= (uint64_t)str[i];
		hash *= FNV_PRIME;
	}
	return hash;
}

void gs_shader::Compile(const char *shaderString, const char *file, const char *target, ID3D10Blob **shader)
{
	ComPtr<ID3D10Blob> errorsBlob;
	HRESULT hr;

	bool is_cached = false;

	if (!shaderString)
		throw "No shader string specified";

	size_t shaderStrLen = strlen(shaderString);

	if (!is_cached) {
		hr = D3DCompile(shaderString, shaderStrLen, file, NULL, NULL, "main", target,
				D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION, 0, shader, errorsBlob.Assign());
		if (FAILED(hr)) {
			if (errorsBlob != NULL && errorsBlob->GetBufferSize())
				throw ShaderError(errorsBlob, hr);
			else
				throw HRError("Failed to compile shader", hr);
		}
	}

#ifdef DISASSEMBLE_SHADERS
	ComPtr<ID3D10Blob> asmBlob;

	hr = D3DDisassemble((*shader)->GetBufferPointer(), (*shader)->GetBufferSize(), 0, nullptr, &asmBlob);

	if (SUCCEEDED(hr) && !!asmBlob && asmBlob->GetBufferSize()) {
		blog(LOG_INFO, "=============================================");
		blog(LOG_INFO, "Disassembly output for shader '%s':\n%s", file, asmBlob->GetBufferPointer());
	}
#endif
}

inline void gs_shader::UpdateParam(std::vector<uint8_t> &constData, gs_shader_param &param, bool &upload)
{
	if (param.type != GS_SHADER_PARAM_TEXTURE) {
		if (!param.curValue.size())
			throw "Not all shader parameters were set";

		/* padding in case the constant needs to start at a new
		 * register */
		if (param.pos > constData.size()) {
			uint8_t zero = 0;

			constData.insert(constData.end(), param.pos - constData.size(), zero);
		}

		constData.insert(constData.end(), param.curValue.begin(), param.curValue.end());

		if (param.changed) {
			upload = true;
			param.changed = false;
		}

	} else if (param.curValue.size() == sizeof(struct gs_shader_texture)) {
		struct gs_shader_texture shader_tex;
		memcpy(&shader_tex, param.curValue.data(), sizeof(shader_tex));
		if (shader_tex.srgb)
			device_load_texture_srgb(device, shader_tex.tex, param.textureID);
		else
			device_load_texture(device, shader_tex.tex, param.textureID);

		if (param.nextSampler) {
			device->curContext->SetDynamicSampler(samplerRootParameterIndex, 0,
							      param.nextSampler->sampleDesc.Sampler);
			param.nextSampler = nullptr;
		}
	}
}

void gs_shader::UploadParams()
{
	std::vector<uint8_t> constData;
	bool upload = true;

	constData.reserve(constantSize);

	for (size_t i = 0; i < params.size(); i++)
		UpdateParam(constData, params[i], upload);

	if (constData.size() != constantSize)
		throw "Invalid constant data size given to shader";

	if (upload && dynamicUniformConstantBufferRootParameterIndex != -1) {
		device->curContext->SetDynamicConstantBufferView(dynamicUniformConstantBufferRootParameterIndex,
								 constData.size(), constData.data());
	}
}

void gs_shader_destroy(gs_shader_t *shader)
{
	delete shader;
}

int gs_shader_get_num_params(const gs_shader_t *shader)
{
	return (int)shader->params.size();
}

gs_sparam_t *gs_shader_get_param_by_idx(gs_shader_t *shader, uint32_t param)
{
	return &shader->params[param];
}

gs_sparam_t *gs_shader_get_param_by_name(gs_shader_t *shader, const char *name)
{
	for (size_t i = 0; i < shader->params.size(); i++) {
		gs_shader_param &param = shader->params[i];
		if (strcmp(param.name.c_str(), name) == 0)
			return &param;
	}

	return NULL;
}

gs_sparam_t *gs_shader_get_viewproj_matrix(const gs_shader_t *shader)
{
	if (shader->type != GS_SHADER_VERTEX)
		return NULL;

	return static_cast<const gs_vertex_shader *>(shader)->viewProj;
}

gs_sparam_t *gs_shader_get_world_matrix(const gs_shader_t *shader)
{
	if (shader->type != GS_SHADER_VERTEX)
		return NULL;

	return static_cast<const gs_vertex_shader *>(shader)->world;
}

void gs_shader_get_param_info(const gs_sparam_t *param, struct gs_shader_param_info *info)
{
	if (!param)
		return;

	info->name = param->name.c_str();
	info->type = param->type;
}

static inline void shader_setval_inline(gs_shader_param *param, const void *data, size_t size)
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

void gs_shader_set_bool(gs_sparam_t *param, bool val)
{
	int b_val = (int)val;
	shader_setval_inline(param, &b_val, sizeof(int));
}

void gs_shader_set_float(gs_sparam_t *param, float val)
{
	shader_setval_inline(param, &val, sizeof(float));
}

void gs_shader_set_int(gs_sparam_t *param, int val)
{
	shader_setval_inline(param, &val, sizeof(int));
}

void gs_shader_set_matrix3(gs_sparam_t *param, const struct matrix3 *val)
{
	struct matrix4 mat;
	matrix4_from_matrix3(&mat, val);
	shader_setval_inline(param, &mat, sizeof(matrix4));
}

void gs_shader_set_matrix4(gs_sparam_t *param, const struct matrix4 *val)
{
	shader_setval_inline(param, val, sizeof(matrix4));
}

void gs_shader_set_vec2(gs_sparam_t *param, const struct vec2 *val)
{
	shader_setval_inline(param, val, sizeof(vec2));
}

void gs_shader_set_vec3(gs_sparam_t *param, const struct vec3 *val)
{
	shader_setval_inline(param, val, sizeof(float) * 3);
}

void gs_shader_set_vec4(gs_sparam_t *param, const struct vec4 *val)
{
	shader_setval_inline(param, val, sizeof(vec4));
}

void gs_shader_set_texture(gs_sparam_t *param, gs_texture_t *val)
{
	shader_setval_inline(param, &val, sizeof(gs_texture_t *));
}

void gs_shader_set_val(gs_sparam_t *param, const void *val, size_t size)
{
	shader_setval_inline(param, val, size);
}

void gs_shader_set_default(gs_sparam_t *param)
{
	if (param->defaultValue.size())
		shader_setval_inline(param, param->defaultValue.data(), param->defaultValue.size());
}

void gs_shader_set_next_sampler(gs_sparam_t *param, gs_samplerstate_t *sampler)
{
	param->nextSampler = sampler;
}
