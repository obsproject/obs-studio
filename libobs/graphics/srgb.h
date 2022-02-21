/******************************************************************************
    Copyright (C) 2021 by Hugh Bailey <obs.jim@gmail.com>

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
******************************************************************************/

#pragma once

#include <math.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

static inline float gs_srgb_nonlinear_to_linear(float u)
{
	return (u <= 0.04045f) ? (u / 12.92f)
			       : powf((u + 0.055f) / 1.055f, 2.4f);
}

static inline float gs_srgb_linear_to_nonlinear(float u)
{
	return (u <= 0.0031308f) ? (12.92f * u)
				 : ((1.055f * powf(u, 1.0f / 2.4f)) - 0.055f);
}

static inline float gs_u8_to_float(uint8_t u)
{
	return (float)u / 255.0f;
}

static inline void gs_u8x4_to_float4(float *f, const uint8_t *u)
{
	f[0] = gs_u8_to_float(u[0]);
	f[1] = gs_u8_to_float(u[1]);
	f[2] = gs_u8_to_float(u[2]);
	f[3] = gs_u8_to_float(u[3]);
}

static inline uint8_t gs_float_to_u8(float f)
{
	return (uint8_t)(f * 255.0f + 0.5f);
}

static inline void gs_premultiply_float4(float *f)
{
	f[0] *= f[3];
	f[1] *= f[3];
	f[2] *= f[3];
}

static inline void gs_float3_to_u8x3(uint8_t *u, const float *f)
{
	u[0] = gs_float_to_u8(f[0]);
	u[1] = gs_float_to_u8(f[1]);
	u[2] = gs_float_to_u8(f[2]);
}

static inline void gs_float4_to_u8x4(uint8_t *u, const float *f)
{
	u[0] = gs_float_to_u8(f[0]);
	u[1] = gs_float_to_u8(f[1]);
	u[2] = gs_float_to_u8(f[2]);
	u[3] = gs_float_to_u8(f[3]);
}

static inline void gs_float3_srgb_nonlinear_to_linear(float *f)
{
	f[0] = gs_srgb_nonlinear_to_linear(f[0]);
	f[1] = gs_srgb_nonlinear_to_linear(f[1]);
	f[2] = gs_srgb_nonlinear_to_linear(f[2]);
}

static inline void gs_float3_srgb_linear_to_nonlinear(float *f)
{
	f[0] = gs_srgb_linear_to_nonlinear(f[0]);
	f[1] = gs_srgb_linear_to_nonlinear(f[1]);
	f[2] = gs_srgb_linear_to_nonlinear(f[2]);
}

static inline void gs_premultiply_xyza(uint8_t *data)
{
	uint8_t u[4];
	float f[4];
	memcpy(&u, data, sizeof(u));
	gs_u8x4_to_float4(f, u);
	gs_premultiply_float4(f);
	gs_float3_to_u8x3(u, f);
	memcpy(data, &u, sizeof(u));
}

static inline void gs_premultiply_xyza_srgb(uint8_t *data)
{
	uint8_t u[4];
	float f[4];
	memcpy(&u, data, sizeof(u));
	gs_u8x4_to_float4(f, u);
	gs_float3_srgb_nonlinear_to_linear(f);
	gs_premultiply_float4(f);
	gs_float3_srgb_linear_to_nonlinear(f);
	gs_float3_to_u8x3(u, f);
	memcpy(data, &u, sizeof(u));
}

static inline void gs_premultiply_xyza_restrict(uint8_t *__restrict dst,
						const uint8_t *__restrict src)
{
	uint8_t u[4];
	float f[4];
	memcpy(&u, src, sizeof(u));
	gs_u8x4_to_float4(f, u);
	gs_premultiply_float4(f);
	gs_float3_to_u8x3(u, f);
	memcpy(dst, &u, sizeof(u));
}

static inline void
gs_premultiply_xyza_srgb_restrict(uint8_t *__restrict dst,
				  const uint8_t *__restrict src)
{
	uint8_t u[4];
	float f[4];
	memcpy(&u, src, sizeof(u));
	gs_u8x4_to_float4(f, u);
	gs_float3_srgb_nonlinear_to_linear(f);
	gs_premultiply_float4(f);
	gs_float3_srgb_linear_to_nonlinear(f);
	gs_float3_to_u8x3(u, f);
	memcpy(dst, &u, sizeof(u));
}

static inline void gs_premultiply_xyza_loop(uint8_t *data, size_t texel_count)
{
	for (size_t i = 0; i < texel_count; ++i) {
		gs_premultiply_xyza(data);
		data += 4;
	}
}

static inline void gs_premultiply_xyza_srgb_loop(uint8_t *data,
						 size_t texel_count)
{
	for (size_t i = 0; i < texel_count; ++i) {
		gs_premultiply_xyza_srgb(data);
		data += 4;
	}
}

static inline void
gs_premultiply_xyza_loop_restrict(uint8_t *__restrict dst,
				  const uint8_t *__restrict src,
				  size_t texel_count)
{
	for (size_t i = 0; i < texel_count; ++i) {
		gs_premultiply_xyza_restrict(dst, src);
		dst += 4;
		src += 4;
	}
}

static inline void
gs_premultiply_xyza_srgb_loop_restrict(uint8_t *__restrict dst,
				       const uint8_t *__restrict src,
				       size_t texel_count)
{
	for (size_t i = 0; i < texel_count; ++i) {
		gs_premultiply_xyza_srgb_restrict(dst, src);
		dst += 4;
		src += 4;
	}
}

#ifdef __cplusplus
}
#endif
