/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "vec4.h"

/* 4x4 Matrix */

#ifdef __cplusplus
extern "C" {
#endif

struct matrix3;

struct matrix4 {
	struct vec4 x, y, z, t;
};

static inline void matrix4_copy(struct matrix4 *dst, const struct matrix4 *m)
{
	dst->x.m = m->x.m;
	dst->y.m = m->y.m;
	dst->z.m = m->z.m;
	dst->t.m = m->t.m;
}

static inline void matrix4_identity(struct matrix4 *dst)
{
	vec4_zero(&dst->x);
	vec4_zero(&dst->y);
	vec4_zero(&dst->z);
	vec4_zero(&dst->t);
	dst->x.x = 1.0f;
	dst->y.y = 1.0f;
	dst->z.z = 1.0f;
	dst->t.w = 1.0f;
}

EXPORT void matrix4_from_matrix3(struct matrix4 *dst, const struct matrix3 *m);

EXPORT void matrix4_mul(struct matrix4 *dst, const struct matrix4 *m1,
		const struct matrix4 *m2);

EXPORT float matrix4_determinant(const struct matrix4 *m);

EXPORT bool matrix4_inv(struct matrix4 *dst, const struct matrix4 *m);
EXPORT void matrix4_transpose(struct matrix4 *dst, const struct matrix4 *m);

#ifdef __cplusplus
}
#endif
