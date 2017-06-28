#include "metal-shaderprocessor.hpp"

#include <strstream>
#include <vector>
#include <map>
#include <set>

using namespace std;

#define METAL_VERSION_1_1      ((1 << 16) | 1)
#define METAL_VERSION_1_2      ((1 << 16) | 2)
#define COMPILE_METAL_VERSION  METAL_VERSION_1_2

#define USE_PROGRAMMABLE_SAMPLER 1

constexpr const char *UNIFORM_DATA_NAME = "UniformData";

enum class ShaderTextureCallType
{
	Sample,
	SampleBias,
	SampleGrad,
	SampleLevel,
	Load
};

struct ShaderFunctionInfo
{
	bool           useUniform;
	vector<string> useTextures;
#if USE_PROGRAMMABLE_SAMPLER
	vector<string> useSamplers;
#endif
};

struct ShaderBuilder
{
	const gs_shader_type            type;

	ShaderParser                    *parser;
	ostrstream                      output;

	set<string>                     constantNames;
	vector<struct shader_var*>      textureVars;
	map<string, ShaderFunctionInfo> functionInfo;

	void Build(string &outputString);

	inline ShaderBuilder(gs_shader_type type, ShaderParser *parser)
		: type(type),
		  parser(parser)
	{
	}

	bool isVertexShader() const {return type == GS_SHADER_VERTEX;}
	bool isPixelShader() const {return type == GS_SHADER_PIXEL;}

private:
	struct shader_var *GetVariable(struct cf_token *token);

	bool IsNextCompareOperator(struct cf_token *&token);
	void AnalysisFunction(struct cf_token *&token, const char *end,
			ShaderFunctionInfo &info);

	void WriteType(const char *type);
	bool WriteTypeToken(struct cf_token *token);
	bool WriteMul(struct cf_token *&token);
	bool WriteConstantVariable(struct cf_token *token);
	bool WriteTextureCall(struct cf_token *&token,
			ShaderTextureCallType type);
	bool WriteTextureCode(struct cf_token *&token, struct shader_var *var);
	bool WriteIntrinsic(struct cf_token *&token);
	void WriteFunctionAdditionalParam(string funcionName);
	void WriteFunctionContent(struct cf_token *&token, const char *end);
	void WriteSamplerParamDelimitter(bool &first);
	void WriteSamplerFilter(enum gs_sample_filter filter, bool &first);
	void WriteSamplerAddress(enum gs_address_mode address,
			const char key, bool &first);
	void WriteSamplerMaxAnisotropy(int maxAnisotropy, bool &first);
	void WriteSamplerBorderColor(uint32_t borderColor, bool &first);

	void WriteVariable(const shader_var *var);
	void WriteSampler(struct shader_sampler *sampler);
	void WriteStruct(const shader_struct *str);
	void WriteFunction(const shader_func *func);

	void WriteInclude();
	void WriteVariables();
	void WriteSamplers();
	void WriteStructs();
	void WriteFunctions();
};

static inline const char *GetType(const string &type)
{
	if (type == "texture2d")
		return "texture2d<float>";
	else if (type == "texture3d")
		return "texture3d<float>";
	else if (type == "texture_cube")
		return "texturecube<float>";
	else if (type == "texture_rect")
		throw "texture_rect is not supported in Metal";
	else if (type.compare(0, 4, "half") == 0) {
		switch (*(type.end() - 1)) {
			case '2': return "float2";
			case '3': return "float3";
			case '4': return "float4";
			case 'f': return "float";
		}
		throw "Unknown type";
	} else if (type.compare(0, 10, "min16float") == 0) {
		switch (*(type.end() - 1)) {
			case '2': return "half2";
			case '3': return "half3";
			case '4': return "half4";
			case 'f': return "half";
		}
		throw "Unknown type";
	} else if (type.compare(0, 10, "min10float") == 0)
		throw "min10float* is not supported in Metal";
	else if (type.compare(0, 6, "double") == 0)
		throw "double* is not supported in Metal";
	else if (type.compare(0, 8, "min16int") == 0) {
		switch (*(type.end() - 1)) {
			case '2': return "short2";
			case '3': return "short3";
			case '4': return "short4";
			case 't': return "short";
		}
		throw "Unknown type";
	} else if (type.compare(0, 9, "min16uint") == 0) {
		switch (*(type.end() - 1)) {
			case '2': return "ushort2";
			case '3': return "ushort3";
			case '4': return "ushort4";
			case 't': return "ushort";
		}
		throw "Unknown type";
	} else if (type.compare(0, 8, "min12int") == 0)
		throw "min12int* is not supported in Metal";

	return nullptr;
}

inline void ShaderBuilder::WriteType(const char *rawType)
{
	string type(rawType);
	const char *newType = GetType(string(rawType));
	output << (newType != nullptr ? newType : type);
}

inline bool ShaderBuilder::WriteTypeToken(struct cf_token *token)
{
	string type(token->str.array, token->str.len);
	const char *newType = GetType(type);
	if (newType == nullptr)
		return false;

	output << newType;
	return true;
}

inline void ShaderBuilder::WriteInclude()
{
	output << "#include <metal_stdlib>" << endl
	       << "using namespace metal;" << endl
	       << endl;
}

inline void ShaderBuilder::WriteVariable(const shader_var *var)
{
	if (var->var_type == SHADER_VAR_CONST)
		output << "constant ";

	WriteType(var->type);

	output << ' ' << var->name;
}

static inline const char *GetMapping(const char *rawMapping)
{
	if (rawMapping == nullptr)
		return nullptr;

	string mapping(rawMapping);
	if (mapping == "POSITION")
		return "position";
	if (mapping == "COLOR")
		return "color(0)";

	return nullptr;
}

inline void ShaderBuilder::WriteVariables()
{
	if (parser->params.num == 0)
		return;

	bool isFirst = true;
	for (struct shader_var *var = parser->params.array;
	     var != parser->params.array + parser->params.num;
	     var++) {
		if (isPixelShader() &&
		    astrcmp_n("texture", var->type, 7) == 0) {
			textureVars.push_back(var);

		} else {
			if (isFirst) {
				output << "struct " << UNIFORM_DATA_NAME
				<< " {" << endl;
				isFirst = false;
			}

			output << '\t';
			WriteVariable(var);

			const char* mapping = GetMapping(var->mapping);
			if (mapping != nullptr)
				output << " [[" << mapping << "]]";

			output << ';' << endl;

			constantNames.insert(var->name);
		}
	}
	if (!isFirst)
		output << "};" << endl << endl;
}

inline void ShaderBuilder::WriteSamplerParamDelimitter(bool &first)
{
	if (!first)
		output << "," << endl;
	else
		first = false;
}

inline void ShaderBuilder::WriteSamplerFilter(enum gs_sample_filter filter,
		bool &first)
{
	if (filter != GS_FILTER_POINT) {
		WriteSamplerParamDelimitter(first);

		switch (filter) {
		case GS_FILTER_LINEAR:
		case GS_FILTER_ANISOTROPIC:
			output << "\tfilter::linear";
			break;
		case GS_FILTER_MIN_MAG_POINT_MIP_LINEAR:
			output << "\tmag_filter::nearest," << endl
			       << "\tmin_filter::nearest," << endl
			       << "\tmip_filter::linear";
			break;
		case GS_FILTER_MIN_POINT_MAG_LINEAR_MIP_POINT:
			output << "\tmag_filter::nearest," << endl
			       << "\tmin_filter::nearest," << endl
			       << "\tmip_filter::linear";
			break;
		case GS_FILTER_MIN_POINT_MAG_MIP_LINEAR:
			output << "\tmag_filter::linear," << endl
			       << "\tmin_filter::nearest," << endl
			       << "\tmip_filter::linear";
			break;
		case GS_FILTER_MIN_LINEAR_MAG_MIP_POINT:
			output << "\tmag_filter::nearest," << endl
			       << "\tmin_filter::linear," << endl
			       << "\tmip_filter::nearest";
			break;
		case GS_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR:
			output << "\tmag_filter::nearest," << endl
			       << "\tmin_filter::linear," << endl
			       << "\tmip_filter::linear";
			break;
		case GS_FILTER_MIN_MAG_LINEAR_MIP_POINT:
			output << "\tmag_filter::linear," << endl
			       << "\tmin_filter::linear," << endl
			       << "\tmip_filter::nearest";
			break;
		case GS_FILTER_POINT:
		default:
			throw "Unknown error";
		}
	}
}

inline void ShaderBuilder::WriteSamplerAddress(enum gs_address_mode address,
		const char key, bool &first)
{
	if (address != GS_ADDRESS_CLAMP) {
		WriteSamplerParamDelimitter(first);

		output << "\t" << key << "_address::";
		switch (address)
		{
		case GS_ADDRESS_WRAP:
			output << "repeat";
			break;
		case GS_ADDRESS_MIRROR:
			output << "mirrored_repeat";
			break;
		case GS_ADDRESS_BORDER:
#if COMPILE_METAL_VERSION >= METAL_VERSION_1_2
			output << "clamp_to_border";
			break;
#else
			throw "GS_ADDRESS_BORDER is not supported in MSL 1.1";
#endif
		case GS_ADDRESS_MIRRORONCE:
			throw "GS_ADDRESS_MIRRORONCE is not supported in Metal";
		default:
			throw "Unknown error";
		}
	}
}

inline void ShaderBuilder::WriteSamplerMaxAnisotropy(int maxAnisotropy,
		bool &first)
{
	if (maxAnisotropy >= 2 && maxAnisotropy <= 16) {
		WriteSamplerParamDelimitter(first);

		output << "\tmax_anisotropy(" << maxAnisotropy << ")";
	}
}

inline void ShaderBuilder::WriteSamplerBorderColor(uint32_t borderColor,
		bool &first)
{
	const bool isNotTransBlack = (borderColor & 0x000000FF) != 0;
	const bool isOpaqueWhite = borderColor == 0xFFFFFFFF;
	if (isNotTransBlack || isOpaqueWhite) {
		WriteSamplerParamDelimitter(first);

		output << "\tborder_color::";

		if (isOpaqueWhite)
			output << "opaque_white";
		else if (isNotTransBlack)
			output << "opaque_black";
	}
}

inline void ShaderBuilder::WriteSampler(struct shader_sampler *sampler)
{
	gs_sampler_info si;
	shader_sampler_convert(sampler, &si);

	output << "constexpr sampler " << sampler->name << "(" << endl;

	bool isFirst = true;
	WriteSamplerFilter(si.filter, isFirst);
	WriteSamplerAddress(si.address_u, 's', isFirst);
	WriteSamplerAddress(si.address_v, 't', isFirst);
	WriteSamplerAddress(si.address_w, 'r', isFirst);
	WriteSamplerMaxAnisotropy(si.max_anisotropy, isFirst);
	WriteSamplerBorderColor(si.border_color, isFirst);

	output << ");" << endl << endl;
}

inline void ShaderBuilder::WriteSamplers()
{
	if (isPixelShader()) {
		for (struct shader_sampler *sampler = parser->samplers.array;
		     sampler != parser->samplers.array + parser->samplers.num;
		     sampler++)
			WriteSampler(sampler);
	}
}

inline void ShaderBuilder::WriteStruct(const shader_struct *str)
{
	output << "struct " << str->name << " {" << endl;

	size_t attributeId = 0;
	for (struct shader_var *var = str->vars.array;
	     var != str->vars.array + str->vars.num;
	     var++) {
		output << '\t';
		WriteVariable(var);

		const char* mapping = GetMapping(var->mapping);
		if (isVertexShader()) {
			output << " [[attribute(" << attributeId++ << ")";
			if (mapping != nullptr)
				output << ", " << mapping;
			output << "]]";
		}

		output << ';' << endl;
	}

	output << "};" << endl << endl;
}

inline void ShaderBuilder::WriteStructs()
{
	for (struct shader_struct *str = parser->structs.array;
	     str != parser->structs.array + parser->structs.num;
	     str++)
		WriteStruct(str);
}

/*
 * NOTE: HLSL -> MSL intrinsic conversions
 *   clip        -> (unsupported)
 *   ddx         -> dfdx
 *   ddy         -> dfdy
 *   frac        -> fract
 *   lerp        -> mix
 *   mul         -> (change to operator)
 *   Sample      -> sample
 *   SampleBias  -> sample(.., bias(..))
 *   SampleGrad  -> sample(.., gradient2d(..))
 *   SampleLevel -> sample(.., level(..))
 *   Load        -> read
 *   A cmp B     -> all(A cmp B)
 *
 *   All else can be left as-is
 */

inline bool ShaderBuilder::WriteMul(struct cf_token *&token)
{
	struct cf_parser *cfp = &parser->cfp;
	cfp->cur_token = token;

	if (!cf_next_token(cfp))    return false;
	if (!cf_token_is(cfp, "(")) return false;

	output << '(';
	WriteFunctionContent(cfp->cur_token, ",");
	output << ") * (";
	cf_next_token(cfp);
	WriteFunctionContent(cfp->cur_token, ")");
	output << "))";

	token = cfp->cur_token;
	return true;
}

inline bool ShaderBuilder::WriteConstantVariable(struct cf_token *token)
{
	string str(token->str.array, token->str.len);
	if (constantNames.find(str) != constantNames.end()) {
		output << "uniforms." << str;
		return true;
	}
	return false;
}

inline bool ShaderBuilder::WriteTextureCall(struct cf_token *&token,
		ShaderTextureCallType type)
{
	struct cf_parser *cfp = &parser->cfp;
	cfp->cur_token = token;

	/* ( */
	if (!cf_next_token(cfp))    return false;
	if (!cf_token_is(cfp, "(")) return false;

	/* sampler */
	if (type != ShaderTextureCallType::Load) {
		output << "sample(";

		if (!cf_next_token(cfp))    return false;
		if (cfp->cur_token->type != CFTOKEN_NAME) return false;
		output.write(cfp->cur_token->str.array,
				cfp->cur_token->str.len);

		if (!cf_next_token(cfp))    return false;
		if (!cf_token_is(cfp, ",")) return false;
		output << ", ";
	} else
		output << "read((u";

	/* location */
	if (!cf_next_token(cfp))    return false;
	if (type != ShaderTextureCallType::Sample &&
	    type != ShaderTextureCallType::Load) {
		WriteFunctionContent(cfp->cur_token, ",");

		/* bias, gradient2d, level */
		switch (type)
		{
		case ShaderTextureCallType::SampleBias:
			output << "bias(";
			if (!cf_next_token(cfp))    return false;
			WriteFunctionContent(cfp->cur_token, ")");
			output << ')';
			break;

		case ShaderTextureCallType::SampleGrad:
			output << "gradient2d(";
			if (!cf_next_token(cfp))    return false;
			WriteFunctionContent(cfp->cur_token, ",");
			if (!cf_next_token(cfp))    return false;
			WriteFunctionContent(cfp->cur_token, ")");
			output << ')';
			break;

		case ShaderTextureCallType::SampleLevel:
			output << "level(";
			if (!cf_next_token(cfp))    return false;
			WriteFunctionContent(cfp->cur_token, ")");
			output << ')';
			break;

		default:
			break;
		}
	} else
		WriteFunctionContent(cfp->cur_token, ")");

	/* ) */
	if (type == ShaderTextureCallType::Load)
		output << ").xy)";
	else
		output << ')';

	return true;
}

inline bool ShaderBuilder::WriteTextureCode(struct cf_token *&token,
		struct shader_var *var)
{
	struct cf_parser *cfp = &parser->cfp;
	bool succeeded = false;
	cfp->cur_token = token;

	if (!cf_next_token(cfp))    return false;
	if (!cf_token_is(cfp, ".")) return false;
	output << var->name << ".";

	if (!cf_next_token(cfp))    return false;
	if (cf_token_is(cfp, "Sample"))
		succeeded = WriteTextureCall(cfp->cur_token,
				ShaderTextureCallType::Sample);
	else if (cf_token_is(cfp, "SampleBias"))
		succeeded = WriteTextureCall(cfp->cur_token,
				ShaderTextureCallType::SampleBias);
	else if (cf_token_is(cfp, "SampleGrad"))
		succeeded = WriteTextureCall(cfp->cur_token,
				ShaderTextureCallType::SampleGrad);
	else if (cf_token_is(cfp, "SampleLevel"))
		succeeded = WriteTextureCall(cfp->cur_token,
				ShaderTextureCallType::SampleLevel);
	else if (cf_token_is(cfp, "Load"))
		succeeded = WriteTextureCall(cfp->cur_token,
				ShaderTextureCallType::Load);

	if (!succeeded)
		throw "Failed to write texture code";

	token = cfp->cur_token;
	return true;
}

inline struct shader_var *ShaderBuilder::GetVariable(struct cf_token *token)
{
	for (struct shader_var *var = parser->params.array;
	     var != parser->params.array + parser->params.num;
	     var++) {
		if (strref_cmp(&token->str, var->name) == 0)
			return var;
	}

	return nullptr;
}

inline bool ShaderBuilder::WriteIntrinsic(struct cf_token *&token)
{
	bool written = true;

	if (strref_cmp(&token->str, "clip") == 0)
		throw "clip is not supported in Metal";
	else if (strref_cmp(&token->str, "ddx") == 0)
		output << "dfdx";
	else if (strref_cmp(&token->str, "ddy") == 0)
		output << "dfdy";
	else if (strref_cmp(&token->str, "frac") == 0)
		output << "fract";
	else if (strref_cmp(&token->str, "lerp") == 0)
		output << "mix";
	else if (strref_cmp(&token->str, "mul") == 0)
		written = WriteMul(token);
	else {
		struct shader_var *var = GetVariable(token);
		if (var != nullptr && astrcmp_n(var->type, "texture", 7) == 0)
			written = WriteTextureCode(token, var);
		else
			written = false;
	}

	return written;
}

inline void ShaderBuilder::AnalysisFunction(struct cf_token *&token,
		const char *end, ShaderFunctionInfo &info)
{
	while (token->type != CFTOKEN_NONE) {
		token++;

		if (strref_cmp(&token->str, end) == 0)
			break;

		if (token->type == CFTOKEN_NAME) {
			string name(token->str.array, token->str.len);

			/* Check function */
			const auto fi = functionInfo.find(name);
			if (fi != functionInfo.end()) {
				if (fi->second.useUniform)
					info.useUniform = true;
				info.useTextures.insert(info.useTextures.end(),
						fi->second.useTextures.begin(),
						fi->second.useTextures.end());
#if USE_PROGRAMMABLE_SAMPLER
				info.useSamplers.insert(info.useSamplers.end(),
						fi->second.useSamplers.begin(),
						fi->second.useSamplers.end());
#endif
				continue;
			}

			/* Check UniformData */
			if (!info.useUniform &&
			    constantNames.find(name) != constantNames.end()) {
				info.useUniform = true;
				continue;
			}

			/* Check texture */
			if (isPixelShader()) {
				for (auto tex = textureVars.cbegin();
				     tex != textureVars.cend();
				     tex++) {
					if (name == (*tex)->name) {
						info.useTextures.emplace_back(
								name);
						break;
					}
				}
#if USE_PROGRAMMABLE_SAMPLER
				for (struct shader_sampler *sampler =
						parser->samplers.array;
				     sampler != parser->samplers.array +
						parser->samplers.num;
				     sampler++) {
					if (name == sampler->name) {
						info.useSamplers.emplace_back(
									name);
						break;
					}
				}
#endif
			}

		} else if (token->type == CFTOKEN_OTHER) {
			if (*token->str.array == '{')
				AnalysisFunction(token, "}", info);
			else if (*token->str.array == '(')
				AnalysisFunction(token, ")", info);
		}
	}
}

inline void ShaderBuilder::WriteFunctionAdditionalParam(string funcionName)
{
	auto fi = functionInfo.find(funcionName);
	if (fi != functionInfo.end()) {
		if (fi->second.useUniform)
			output << ", uniforms";

		for (auto var = textureVars.cbegin();
		     var != textureVars.cend();
		     var++) {
			for (auto tex = fi->second.useTextures.cbegin();
			     tex != fi->second.useTextures.cend();
			     tex++) {
				if (*tex == (*var)->name) {
					output << ", " << *tex;
					break;
				}
			}
		}

#if USE_PROGRAMMABLE_SAMPLER
		for (struct shader_sampler *sampler = parser->samplers.array;
		     sampler != parser->samplers.array + parser->samplers.num;
		     sampler++) {
			for (auto s = fi->second.useSamplers.cbegin();
			     s != fi->second.useSamplers.cend();
			     s++) {
				if (*s == sampler->name) {
					output << ", " << *s;
					break;
				}
			}
		}
#endif
	}
}

inline bool ShaderBuilder::IsNextCompareOperator(struct cf_token *&token)
{
	struct cf_token *token2 = token + 1;
	if (token2->type != CFTOKEN_OTHER)
		return false;

	if (astrcmp_n(token2->str.array, "==", token2->str.len) == 0 ||
	    astrcmp_n(token2->str.array, "!=", token2->str.len) == 0 ||
	    astrcmp_n(token2->str.array, "<", token2->str.len) == 0 ||
	    astrcmp_n(token2->str.array, "<=", token2->str.len) == 0 ||
	    astrcmp_n(token2->str.array, ">", token2->str.len) == 0 ||
	    astrcmp_n(token2->str.array, ">=", token2->str.len) == 0)
		return true;
	return false;
}

inline void ShaderBuilder::WriteFunctionContent(struct cf_token *&token,
		const char *end)
{
	string temp;
	if (token->type != CFTOKEN_NAME)
		output.write(token->str.array, token->str.len);

	else if ((!WriteTypeToken(token) && !WriteIntrinsic(token) &&
	     !WriteConstantVariable(token))) {
		temp = string(token->str.array, token->str.len);
		output << temp;
	}

	bool dot = false;
	bool cmp = false;
	while (token->type != CFTOKEN_NONE) {
		token++;

		if (strref_cmp(&token->str, end) == 0)
			break;

		if (token->type == CFTOKEN_NAME) {
			if (!WriteTypeToken(token) && !WriteIntrinsic(token) &&
			    (dot || !WriteConstantVariable(token))) {
				if (dot)
					dot = false;

				bool cmp2 = IsNextCompareOperator(token);
				if (cmp2)
					output << "all(";

				temp = string(token->str.array, token->str.len);
				output << temp;

				if (cmp) {
					output << ")";
					cmp = false;
				}

				cmp = cmp2;
			}

		} else if (token->type == CFTOKEN_OTHER) {
			if (*token->str.array == '{')
				WriteFunctionContent(token, "}");
			else if (*token->str.array == '(') {
				WriteFunctionContent(token, ")");
				WriteFunctionAdditionalParam(temp);
			} else if (*token->str.array == '.')
				dot = true;

			output.write(token->str.array, token->str.len);

		} else
			output.write(token->str.array, token->str.len);
	}
}

inline void ShaderBuilder::WriteFunction(const shader_func *func)
{
	string funcName(func->name);

	const bool isMain = funcName == "main";
	if (isMain) {
		if (isVertexShader())
			output << "vertex ";
		else if (isPixelShader())
			output << "fragment ";
		else
			throw "Failed to add shader prefix";

		funcName = "_main";
	}

	ShaderFunctionInfo info = {};
	struct cf_token *token = func->start;
	AnalysisFunction(token, "}", info);
	unique(info.useTextures.begin(), info.useTextures.end());
	unique(info.useSamplers.begin(), info.useSamplers.end());
	functionInfo.emplace(funcName, info);

	output << func->return_type << ' ' << funcName << '(';

	bool isFirst = true;
	for (struct shader_var *param = func->params.array;
	     param != func->params.array + func->params.num;
	     param++) {
		if (!isFirst)
			output << ", ";

		WriteVariable(param);

		if (isMain) {
			if (!isFirst)
				throw "Failed to add type";
			output << " [[stage_in]]";

		}

		if (isFirst)
			isFirst = false;
	}

	if (constantNames.size() != 0 && (isMain || info.useUniform))
	{
		if (!isFirst)
			output << ", ";

		output << "constant " << UNIFORM_DATA_NAME << " &uniforms";

		if (isMain)
			output << " [[buffer(30)]]";

		if (isFirst)
			isFirst = false;
	}

	if (isPixelShader())
	{
		size_t textureId = 0;
		for (auto var = textureVars.cbegin();
		     var != textureVars.cend();
		     var++) {
			if (!isMain) {
				bool additional = false;
				for (auto tex = info.useTextures.cbegin();
				     tex != info.useTextures.cend();
				     tex++) {
					if (*tex == (*var)->name) {
						additional = true;
						break;
					}
				}
				if (!additional)
					continue;
			}

			if (!isFirst)
				output << ", ";

			WriteVariable(*var);

			if (isMain)
				output << " [[texture(" << textureId++ << ")]]";

			if (isFirst)
				isFirst = false;
		}

#if USE_PROGRAMMABLE_SAMPLER
		size_t samplerId = 0;
		for (struct shader_sampler *sampler = parser->samplers.array;
		     sampler != parser->samplers.array + parser->samplers.num;
		     sampler++) {
			if (!isMain) {
				bool additional = false;
				for (auto s = info.useSamplers.cbegin();
				     s != info.useSamplers.cend();
				     s++) {
					if (*s == sampler->name) {
						additional = true;
						break;
					}
				}
				if (!additional)
					continue;
			}

			if (!isFirst)
				output << ", ";

			output << "sampler " << sampler->name;

			if (isMain)
				output << " [[sampler(" << samplerId++ << ")]]";

			if (isFirst)
				isFirst = false;
		}
#endif
	}

	output << ")" << endl;

	token = func->start;
	WriteFunctionContent(token, "}");

	output << '}' << endl << endl;
}

inline void ShaderBuilder::WriteFunctions()
{
	for (struct shader_func *func = parser->funcs.array;
	     func != parser->funcs.array + parser->funcs.num;
	     func++)
		WriteFunction(func);
}

void ShaderBuilder::Build(string &outputString)
{
	WriteInclude();
#if !USE_PROGRAMMABLE_SAMPLER
	WriteSamplers();
#endif
	WriteVariables();
	WriteStructs();
	WriteFunctions();
	outputString = string(output.str(), output.pcount());
	output.freeze(false);
}

string build_shader(gs_shader_type type, ShaderParser *parser)
{
	string output;
	ShaderBuilder(type, parser).Build(output);
	return output;
}
