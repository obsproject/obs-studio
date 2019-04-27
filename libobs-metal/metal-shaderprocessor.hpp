#pragma once

#include <string>

#include <graphics/shader-parser.h>

struct ShaderBufferInfo {
	bool     normals  = false;
	bool     colors   = false;
	bool     tangents = false;
	uint32_t texUnits = 0;
};

struct ShaderParser : shader_parser {
	inline ShaderParser()  {shader_parser_init(this);}
	inline ~ShaderParser() {shader_parser_free(this);}
};

#ifdef __OBJC__
struct ShaderProcessor {
	gs_device_t *device;
	ShaderParser parser;

	void BuildVertexDesc(MTLVertexDescriptor *vertexDesc);
	void BuildParamInfo(ShaderBufferInfo &info);
	void BuildParams(std::vector<gs_shader_param> &params);
	void BuildSamplers(std::vector<std::unique_ptr<ShaderSampler>>
			&samplers);
	std::string BuildString(gs_shader_type type);
	void Process(const char *shader_string, const char *file);

	inline ShaderProcessor(gs_device_t *device) : device(device)
	{
	}
};
#endif

extern std::string build_shader(gs_shader_type type, ShaderParser *parser);
