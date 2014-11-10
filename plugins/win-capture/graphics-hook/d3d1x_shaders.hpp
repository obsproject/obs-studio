#pragma once

static const char vertex_shader_string[] =
"struct VertData \
{ \
	float4 pos : SV_Position; \
	float2 texCoord : TexCoord0; \
}; \
VertData main(VertData input) \
{ \
	VertData output; \
	output.pos = input.pos; \
	output.texCoord = input.texCoord; \
	return output; \
}";

static const char pixel_shader_string[] =
"uniform Texture2D diffuseTexture; \
SamplerState textureSampler \
{ \
	AddressU = Clamp; \
	AddressV = Clamp; \
	Filter   = Linear; \
}; \
struct VertData \
{ \
	float4 pos      : SV_Position; \
	float2 texCoord : TexCoord0; \
}; \
float4 main(VertData input) : SV_Target \
{ \
	return diffuseTexture.Sample(textureSampler, input.texCoord); \
}";
