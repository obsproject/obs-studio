#include "metal-subsystem.hpp"
#include "metal-shaderprocessor.hpp"

#include <vector>

using namespace std;

static inline void AddInputLayoutVar(shader_var *var,
		MTLVertexAttributeDescriptor *vad,
		MTLVertexBufferLayoutDescriptor *vbld)
{
	if (strcmp(var->mapping, "COLOR") == 0) {
		vad.format = MTLVertexFormatUChar4Normalized;
		vbld.stride = sizeof(vec4);

	} else if (strcmp(var->mapping, "POSITION") == 0 ||
	           strcmp(var->mapping, "NORMAL")   == 0 ||
	           strcmp(var->mapping, "TANGENT")  == 0) {
		vad.format = MTLVertexFormatFloat4;
		vbld.stride = sizeof(vec4);
		
	} else if (astrcmp_n(var->mapping, "TEXCOORD", 8) == 0) {
		/* type is always a 'float' type */
		switch (var->type[5]) {
		case 0:
			vad.format = MTLVertexFormatFloat;
			vbld.stride = sizeof(float);
			break;
		
		case '2':
			vad.format = MTLVertexFormatFloat2;
			vbld.stride = sizeof(float) * 2;
			break;
				
		case '3':
			vad.format = MTLVertexFormatFloat3;
			vbld.stride = sizeof(vec3);
			break;
				
		case '4':
			vad.format = MTLVertexFormatFloat4;
			vbld.stride = sizeof(vec4);
			break;
		}
	}
}

static inline void BuildVertexDescFromVars(shader_parser *parser, darray *vars,
		MTLVertexDescriptor *vd, size_t &index)
{
	shader_var *array = (shader_var*)vars->array;

	for (size_t i = 0; i < vars->num; i++) {
		shader_var *var = array + i;

		if (var->mapping) {
			vd.attributes[index].bufferIndex = index;
			AddInputLayoutVar(var, vd.attributes[index],
					vd.layouts[index++]);
		} else {
			shader_struct *st = shader_parser_getstruct(parser,
					var->type);
			if (st)
				BuildVertexDescFromVars(parser, &st->vars.da,
						vd, index);
		}
	}
}

void ShaderProcessor::BuildVertexDesc(MTLVertexDescriptor *vertexDesc)
{
	shader_func *func = shader_parser_getfunc(&parser, "main");
	if (!func)
		throw "Failed to find 'main' shader function";

	size_t index = 0;
	BuildVertexDescFromVars(&parser, &func->params.da, vertexDesc, index);
}

static inline void BuildParamInfoFromVars(shader_parser *parser, darray *vars,
		ShaderBufferInfo &info)
{
	shader_var *array = (shader_var*)vars->array;
	
	for (size_t i = 0; i < vars->num; i++) {
		shader_var *var = array + i;
		
		if (var->mapping) {
			if (strcmp(var->mapping, "NORMAL") == 0)
				info.normals = true;
			else if (strcmp(var->mapping, "TANGENT") == 0)
				info.tangents = true;
			else if (strcmp(var->mapping, "COLOR") == 0)
				info.colors = true;
			else if (astrcmp_n(var->mapping, "TEXCOORD", 8) == 0)
				info.texUnits++;

		} else {
			shader_struct *st = shader_parser_getstruct(parser,
					var->type);
			if (st)
				BuildParamInfoFromVars(parser, &st->vars.da,
						info);
		}
	}
}

void ShaderProcessor::BuildParamInfo(ShaderBufferInfo &info)
{
	shader_func *func = shader_parser_getfunc(&parser, "main");
	if (!func)
		throw "Failed to find 'main' shader function";
	
	BuildParamInfoFromVars(&parser, &func->params.da, info);
}

gs_shader_param::gs_shader_param(shader_var &var, uint32_t &texCounter)
	: name       (var.name),
	  type       (get_shader_param_type(var.type)),
	  arrayCount (var.array_count),
	  textureID  (texCounter),
	  changed    (false)
{
	defaultValue.resize(var.default_val.num);
	memcpy(defaultValue.data(), var.default_val.array, var.default_val.num);

	if (type == GS_SHADER_PARAM_TEXTURE)
		texCounter++;
	else
		textureID = 0;
}

static inline void AddParam(shader_var &var, vector<gs_shader_param> &params,
		uint32_t &texCounter)
{
	if (var.var_type != SHADER_VAR_UNIFORM ||
	    strcmp(var.type, "sampler") == 0)
		return;

	params.push_back(gs_shader_param(var, texCounter));
}

void ShaderProcessor::BuildParams(vector<gs_shader_param> &params)
{
	uint32_t texCounter = 0;

	for (struct shader_var *var = parser.params.array;
	     var != parser.params.array + parser.params.num;
	     var++)
		AddParam(*var, params, texCounter);
}

static inline void AddSampler(gs_device_t *device, shader_sampler &sampler,
			vector<unique_ptr<ShaderSampler>> &samplers)
{
	gs_sampler_info si;
	shader_sampler_convert(&sampler, &si);
	samplers.emplace_back(new ShaderSampler(sampler.name, device, &si));
}

void ShaderProcessor::BuildSamplers(vector<unique_ptr<ShaderSampler>> &samplers)
{
	for (struct shader_sampler *sampler = parser.samplers.array;
	     sampler != parser.samplers.array + parser.samplers.num;
	     sampler++)
		AddSampler(device, *sampler, samplers);
}

string ShaderProcessor::BuildString(gs_shader_type type)
{
	return build_shader(type, &parser);
}

void ShaderProcessor::Process(const char *shader_string, const char *file)
{
	bool success = shader_parse(&parser, shader_string, file);
	char *str = shader_parser_geterrors(&parser);
	if (str) {
		blog(LOG_WARNING, "Shader parser errors/warnings:\n%s\n", str);
		bfree(str);
	}
	
	if (!success)
		throw "Failed to parse shader";
}

