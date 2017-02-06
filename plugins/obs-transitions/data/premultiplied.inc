float4 convert_pmalpha(float4 color)
{
	float4 ret = color;
	if (color.a >= 0.001)
		ret.xyz /= color.a;
	else
		ret = float4(0.0, 0.0, 0.0, 0.0);
	return ret;
}
