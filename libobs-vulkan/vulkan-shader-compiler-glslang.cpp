/*******************************************************************************
    Copyright (C) 2023-2024 by OBS Project

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
*******************************************************************************/

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <vector>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

/* Stage mapping for glslang */
EShLanguage stage_to_esh_language(int stage)
{
	switch (stage) {
	case 0: /* GLSLANG_STAGE_VERTEX */
		return EShLangVertex;
	case 1: /* GLSLANG_STAGE_TESS_CONTROL */
		return EShLangTessControl;
	case 2: /* GLSLANG_STAGE_TESS_EVAL */
		return EShLangTessEvaluation;
	case 3: /* GLSLANG_STAGE_GEOMETRY */
		return EShLangGeometry;
	case 4: /* GLSLANG_STAGE_FRAGMENT */
		return EShLangFragment;
	case 5: /* GLSLANG_STAGE_COMPUTE */
		return EShLangCompute;
	default:
		return EShLangCount;
	}
}

/* Compiler class wrapper */
class GLSLCompiler {
public:
	GLSLCompiler() {}
	~GLSLCompiler() {}

	bool compile(int lang, int stage, const char *source, size_t source_len,
		     std::vector<uint32_t> &spirv, std::string &error_msg)
	{
		if (lang != 0) { /* GLSLANG_LANG_GLSL */
			error_msg = "Only GLSL is supported";
			return false;
		}

		EShLanguage esh_stage = stage_to_esh_language(stage);
		if (esh_stage == EShLangCount) {
			error_msg = "Invalid shader stage";
			return false;
		}

		glslang::TShader shader(esh_stage);

		const char *shader_strings[] = {source};
		const int shader_lengths[] = {(int)source_len};
		shader.setStringsWithLengths(shader_strings, shader_lengths, 1);

		shader.setEnvInput(glslang::EShSourceGlsl, esh_stage, glslang::EShClientVulkan,
				   glslang::EShTargetVulkan_1_2);
		shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_2);
		shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_5);

		const TBuiltInResource *resources = GetDefaultResources();

		std::string preprocessed;
		if (!shader.preprocess(resources, 450, ENoProfile, false, false, EShMsgDefault,
				       preprocessed, nullptr)) {
			error_msg = "Preprocessing failed: ";
			error_msg += shader.getInfoLog();
			error_msg += " ";
			error_msg += shader.getInfoDebugLog();
			return false;
		}

		const char *preprocessed_str = preprocessed.c_str();
		shader.resetStrings();
		shader.setStringsWithLengths(&preprocessed_str, &shader_lengths[0], 1);

		if (!shader.parse(resources, 450, false, EShMsgDefault)) {
			error_msg = "Parsing failed: ";
			error_msg += shader.getInfoLog();
			error_msg += " ";
			error_msg += shader.getInfoDebugLog();
			return false;
		}

		glslang::TProgram program;
		program.addShader(&shader);

		if (!program.link(EShMsgDefault)) {
			error_msg = "Linking failed: ";
			error_msg += program.getInfoLog();
			error_msg += " ";
			error_msg += program.getInfoDebugLog();
			return false;
		}

		glslang::SpvOptions spv_options;
		spv_options.generateDebugInfo = false;
		spv_options.disableOptimizer = false;
		spv_options.optimizeSize = false;

		std::vector<uint32_t> local_spirv;
		glslang::GlslangToSpv(*program.getIntermediate(esh_stage), local_spirv, &spv_options);

		spirv = local_spirv;
		return true;
	}
};

/* C Interface Functions */
void glslang_lib_init(void)
{
	glslang::InitializeProcess();
}

void glslang_lib_finalize(void)
{
	glslang::FinalizeProcess();
}

void *glslang_compiler_new(void)
{
	return new GLSLCompiler();
}

void glslang_compiler_delete(void *compiler)
{
	delete (GLSLCompiler *)compiler;
}

int glslang_compile(void *compiler, int lang, int stage, const char *source,
		    size_t source_len, uint32_t **out_spirv, size_t *out_size, char **out_error)
{
	if (!compiler || !source || !out_spirv || !out_size) {
		return -1;
	}

	GLSLCompiler *glsl_compiler = (GLSLCompiler *)compiler;
	std::vector<uint32_t> spirv;
	std::string error_msg;

	if (!glsl_compiler->compile(lang, stage, source, source_len, spirv, error_msg)) {
		if (out_error) {
			*out_error = (char *)malloc(error_msg.size() + 1);
			strcpy(*out_error, error_msg.c_str());
		}
		return -1;
	}

	*out_size = spirv.size() * sizeof(uint32_t);
	*out_spirv = (uint32_t *)malloc(*out_size);
	memcpy(*out_spirv, spirv.data(), *out_size);

	if (out_error) {
		*out_error = nullptr;
	}

	return 0;
}

#ifdef __cplusplus
}
#endif
