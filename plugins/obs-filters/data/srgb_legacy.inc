#include "colorspace.inc"

float3 ConvertToSrgbNonlinear(float3 rgb)
{
	float3 rgb_matrix0 = float3(2.02579975, -0.798917055, -0.159652337);
	float3 rgb_matrix1 = float3(0.231922448, 0.835083961, -0.0509763733);
	float3 rgb_matrix2 = float3(0.0342020467, -0.189897582, 1.07084787);
	rgb = float3(dot(rgb, rgb_matrix0), dot(rgb, rgb_matrix1), dot(rgb, rgb_matrix2));
	rgb = saturate(rgb);
	rgb = LinearToGamma(rgb, float3(1.055, 0.0, 12.92), float3(0.0031308, -0.055, 1.0 / 2.4));
	return rgb;
}

float3 ConvertFromSrgbNonlinear(float3 rgb)
{
	rgb = GammaToLinear(rgb, float3(1.0 / 1.055, 0.055 / 1.055, 1.0 / 12.92), float3(0.04045, 0.0, 2.4));
	float3 rgb_matrix0 = float3(0.441543043, 0.442176282, 0.0868787393);
	float3 rgb_matrix1 = float3(-0.124839157, 1.08557081, 0.0330650136);
	float3 rgb_matrix2 = float3(-0.0362407565, 0.178385690, 0.936928093);
	rgb = float3(dot(rgb, rgb_matrix0), dot(rgb, rgb_matrix1), dot(rgb, rgb_matrix2));
	return rgb;
}
