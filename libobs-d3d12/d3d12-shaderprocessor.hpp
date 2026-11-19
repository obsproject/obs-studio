#pragma once

#include <graphics/shader-parser.h>

struct ShaderParser : shader_parser {
	inline ShaderParser() { shader_parser_init(this); }
	inline ~ShaderParser() { shader_parser_free(this); }
};

struct ShaderProcessor {
	gs_device_t *device;
	ShaderParser parser;

	void BuildInputLayout(std::vector<D3D12_INPUT_ELEMENT_DESC> &inputs);
	void BuildParams(std::vector<gs_shader_param> &params);
	void BuildSamplers(std::vector<std::unique_ptr<ShaderSampler>> &samplers);
	void BuildString(std::string &outputString);
	void Process(const char *shader_string, const char *file);

	inline ShaderProcessor(gs_device_t *device) : device(device) {}
};
