// Keep in sync with file in plugins\obs-filters\data

float GammaToLinearChannel(float v, float3 abc, float3 def)
{
	return (v < def.x) ? (abc.z * v) : (pow(abc.x * v + abc.y, def.z) + def.y);
}

float3 GammaToLinear(float3 color, float3 abc, float3 def)
{
	return float3(GammaToLinearChannel(color.r, abc, def), GammaToLinearChannel(color.g, abc, def), GammaToLinearChannel(color.b, abc, def));
}

float LinearToGammaChannel(float l, float3 abc, float3 def)
{
	return (l < def.x) ? (abc.z * l) : ((abc.x * pow(l + abc.y, def.z)) + def.y);
}

float3 LinearToGamma(float3 color, float3 abc, float3 def)
{
	return float3(LinearToGammaChannel(color.r, abc, def), LinearToGammaChannel(color.g, abc, def), LinearToGammaChannel(color.b, abc, def));
}

float3 YuvToRgb(float3 yuv, float3 range_min, float3 range_max, float4x4 yuv_matrix)
{
	yuv = clamp(yuv, range_min, range_max);
	return saturate(mul(float4(yuv, 1.0), yuv_matrix).rgb);
}

float3 RgbToYuv(float3 rgb, float4x4 yuv_matrix)
{
	float3 yuv = mul(float4(rgb, 1.0), yuv_matrix).xyz;
	return yuv;
}

float3 ConvertFromLinear(float3 rgb, float3 abc, float3 def, float3 rgb_matrix0, float3 rgb_matrix1, float3 rgb_matrix2)
{
	rgb = float3(dot(rgb, rgb_matrix0), dot(rgb, rgb_matrix1), dot(rgb, rgb_matrix2));
	rgb = LinearToGamma(rgb, abc, def);
	return rgb;
}

float3 ConvertToLinear(float3 rgb, float3 abc, float3 def, float3 rgb_matrix0, float3 rgb_matrix1, float3 rgb_matrix2)
{
	rgb = GammaToLinear(rgb, abc, def);
	rgb = float3(dot(rgb, rgb_matrix0), dot(rgb, rgb_matrix1), dot(rgb, rgb_matrix2));
	return rgb;
}

float3 TranslateColor(float3 rgb, float3 source_abc, float3 source_def, float3 target_abc, float3 target_def, float3 rgb_matrix0, float3 rgb_matrix1, float3 rgb_matrix2)
{
	rgb = GammaToLinear(rgb, source_abc, source_def);
	rgb = float3(dot(rgb, rgb_matrix0), dot(rgb, rgb_matrix1), dot(rgb, rgb_matrix2));
	rgb = LinearToGamma(rgb, target_abc, target_def);
	return rgb;
}

float4 YuvToLinear(float3 yuv, float3 abc, float3 def, float3 range_min, float3 range_max, float4x4 yuv_matrix, float3 rgb_matrix0, float3 rgb_matrix1, float3 rgb_matrix2)
{
	return float4(ConvertToLinear(YuvToRgb(yuv, range_min, range_max, yuv_matrix), abc, def, rgb_matrix0, rgb_matrix1, rgb_matrix2), 1.0);
}

float3 LinearToYuv(float3 rgb, float3 abc, float3 def, float4x4 yuv_matrix, float3 rgb_matrix0, float3 rgb_matrix1, float3 rgb_matrix2)
{
	return RgbToYuv(ConvertFromLinear(rgb, abc, def, rgb_matrix0, rgb_matrix1, rgb_matrix2), yuv_matrix);
}

float3 TranslateMatrixColor(float3 yuv, float3 source_abc, float3 source_def, float3 target_abc, float3 target_def, float3 range_min, float3 range_max, float4x4 yuv_matrix, float3 rgb_matrix0, float3 rgb_matrix1, float3 rgb_matrix2)
{
	float3 source_rgb = YuvToRgb(yuv, range_min, range_max, yuv_matrix);
	float3 target_rgb = TranslateColor(source_rgb, source_abc, source_def, target_abc, target_def, rgb_matrix0, rgb_matrix1, rgb_matrix2);
	return target_rgb;
}

float3 TranslateMatrixColorReverse(float3 source_rgb, float3 source_abc, float3 source_def, float3 target_abc, float3 target_def, float4x4 yuv_matrix, float3 rgb_matrix0, float3 rgb_matrix1, float3 rgb_matrix2)
{
	float3 target_rgb = TranslateColor(source_rgb, source_abc, source_def, target_abc, target_def, rgb_matrix0, rgb_matrix1, rgb_matrix2);
	float3 yuv = RgbToYuv(target_rgb, yuv_matrix);
	return yuv;
}
