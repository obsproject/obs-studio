/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

#include "math-defs.h"
#include "srgb.h"

#include "../util/sse-intrin.h"

#ifdef __cplusplus
extern "C" {
#endif

struct vec3;
struct matrix4;

struct vec4 {
	union {
		struct {
			float x, y, z, w;
		};
		float ptr[4];
		__m128 m;
	};
};

static inline void vec4_zero(struct vec4 *v)
{
	v->m = _mm_setzero_ps();
}

static inline void vec4_set(struct vec4 *dst, float x, float y, float z,
			    float w)
{
	dst->m = _mm_set_ps(w, z, y, x);
}

static inline void vec4_copy(struct vec4 *dst, const struct vec4 *v)
{
	dst->m = v->m;
}

EXPORT void vec4_from_vec3(struct vec4 *dst, const struct vec3 *v);

static inline void vec4_add(struct vec4 *dst, const struct vec4 *v1,
			    const struct vec4 *v2)
{
	dst->m = _mm_add_ps(v1->m, v2->m);
}

static inline void vec4_sub(struct vec4 *dst, const struct vec4 *v1,
			    const struct vec4 *v2)
{
	dst->m = _mm_sub_ps(v1->m, v2->m);
}

static inline void vec4_mul(struct vec4 *dst, const struct vec4 *v1,
			    const struct vec4 *v2)
{
	dst->m = _mm_mul_ps(v1->m, v2->m);
}

static inline void vec4_div(struct vec4 *dst, const struct vec4 *v1,
			    const struct vec4 *v2)
{
	dst->m = _mm_div_ps(v1->m, v2->m);
}

static inline void vec4_addf(struct vec4 *dst, const struct vec4 *v, float f)
{
	dst->m = _mm_add_ps(v->m, _mm_set1_ps(f));
}

static inline void vec4_subf(struct vec4 *dst, const struct vec4 *v, float f)
{
	dst->m = _mm_sub_ps(v->m, _mm_set1_ps(f));
}

static inline void vec4_mulf(struct vec4 *dst, const struct vec4 *v, float f)
{
	dst->m = _mm_mul_ps(v->m, _mm_set1_ps(f));
}

static inline void vec4_divf(struct vec4 *dst, const struct vec4 *v, float f)
{
	dst->m = _mm_div_ps(v->m, _mm_set1_ps(f));
}

static inline float vec4_dot(const struct vec4 *v1, const struct vec4 *v2)
{
	struct vec4 add;
	__m128 mul = _mm_mul_ps(v1->m, v2->m);
	add.m = _mm_add_ps(_mm_movehl_ps(mul, mul), mul);
	add.m = _mm_add_ps(_mm_shuffle_ps(add.m, add.m, 0x55), add.m);
	return add.x;
}

static inline void vec4_neg(struct vec4 *dst, const struct vec4 *v)
{
	dst->x = -v->x;
	dst->y = -v->y;
	dst->z = -v->z;
	dst->w = -v->w;
}

static inline float vec4_len(const struct vec4 *v)
{
	float dot_val = vec4_dot(v, v);
	return (dot_val > 0.0f) ? sqrtf(dot_val) : 0.0f;
}

static inline float vec4_dist(const struct vec4 *v1, const struct vec4 *v2)
{
	struct vec4 temp;
	float dot_val;

	vec4_sub(&temp, v1, v2);
	dot_val = vec4_dot(&temp, &temp);
	return (dot_val > 0.0f) ? sqrtf(dot_val) : 0.0f;
}

static inline void vec4_norm(struct vec4 *dst, const struct vec4 *v)
{
	float dot_val = vec4_dot(v, v);
	dst->m = (dot_val > 0.0f)
			 ? _mm_mul_ps(v->m, _mm_set1_ps(1.0f / sqrtf(dot_val)))
			 : _mm_setzero_ps();
}

static inline int vec4_close(const struct vec4 *v1, const struct vec4 *v2,
			     float epsilon)
{
	struct vec4 test;
	vec4_sub(&test, v1, v2);
	return test.x < epsilon && test.y < epsilon && test.z < epsilon &&
	       test.w < epsilon;
}

static inline void vec4_min(struct vec4 *dst, const struct vec4 *v1,
			    const struct vec4 *v2)
{
	dst->m = _mm_min_ps(v1->m, v2->m);
}

static inline void vec4_minf(struct vec4 *dst, const struct vec4 *v, float f)
{
	dst->m = _mm_min_ps(v->m, _mm_set1_ps(f));
}

static inline void vec4_max(struct vec4 *dst, const struct vec4 *v1,
			    const struct vec4 *v2)
{
	dst->m = _mm_max_ps(v1->m, v2->m);
}

static inline void vec4_maxf(struct vec4 *dst, const struct vec4 *v, float f)
{
	dst->m = _mm_max_ps(v->m, _mm_set1_ps(f));
}

static inline void vec4_abs(struct vec4 *dst, const struct vec4 *v)
{
	dst->x = fabsf(v->x);
	dst->y = fabsf(v->y);
	dst->z = fabsf(v->z);
	dst->w = fabsf(v->w);
}

static inline void vec4_floor(struct vec4 *dst, const struct vec4 *v)
{
	dst->x = floorf(v->x);
	dst->y = floorf(v->y);
	dst->z = floorf(v->z);
	dst->w = floorf(v->w);
}

static inline void vec4_ceil(struct vec4 *dst, const struct vec4 *v)
{
	dst->x = ceilf(v->x);
	dst->y = ceilf(v->y);
	dst->z = ceilf(v->z);
	dst->w = ceilf(v->w);
}

static inline uint32_t vec4_to_rgba(const struct vec4 *src)
{
	float f[4];
	memcpy(f, src->ptr, sizeof(f));
	uint8_t u[4];
	gs_float4_to_u8x4(u, f);
	uint32_t val;
	memcpy(&val, u, sizeof(val));
	return val;
}

static inline uint32_t vec4_to_bgra(const struct vec4 *src)
{
	float f[4];
	memcpy(f, src->ptr, sizeof(f));
	uint8_t u[4];
	gs_float4_to_u8x4(u, f);
	uint8_t temp = u[0];
	u[0] = u[2];
	u[2] = temp;
	uint32_t val;
	memcpy(&val, u, sizeof(val));
	return val;
}

static inline void vec4_from_rgba(struct vec4 *dst, uint32_t rgba)
{
	uint8_t u[4];
	memcpy(u, &rgba, sizeof(u));
	gs_u8x4_to_float4(dst->ptr, u);
}

static inline void vec4_from_bgra(struct vec4 *dst, uint32_t bgra)
{
	uint8_t u[4];
	memcpy(u, &bgra, sizeof(u));
	uint8_t temp = u[0];
	u[0] = u[2];
	u[2] = temp;
	gs_u8x4_to_float4(dst->ptr, u);
}

static inline void vec4_from_rgba_srgb(struct vec4 *dst, uint32_t rgba)
{
	vec4_from_rgba(dst, rgba);
	gs_float3_srgb_nonlinear_to_linear(dst->ptr);
}

EXPORT void vec4_transform(struct vec4 *dst, const struct vec4 *v,
			   const struct matrix4 *m);

#ifdef __cplusplus
}
#endif
