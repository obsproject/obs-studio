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

#include "math-defs.h"
#include "matrix4.h"
#include "matrix3.h"

void matrix4_from_matrix3(struct matrix4 *dst, const struct matrix3 *m)
{
	dst->x.m = m->x.m;
	dst->y.m = m->y.m;
	dst->z.m = m->z.m;
	dst->t.m = m->t.m;
	dst->t.w = 1.0f;
}

void matrix4_mul(struct matrix4 *dst, const struct matrix4 *m1,
		const struct matrix4 *m2)
{
	const struct vec4 *m1v = (const struct vec4*)m1;
	const float *m2f = (const float*)m2;
	struct vec4 out[4];
	int i, j;

	for (i = 0; i < 4; i++) {
		for (j=0; j<4; j++) {
			struct vec4 temp;
			vec4_set(&temp, m2f[j], m2f[j+4], m2f[j+8], m2f[j+12]);
			out[i].ptr[j] = vec4_dot(&m1v[i], &temp);
		}
	}

	matrix4_copy(dst, (struct matrix4*)out);
}

static inline void get_3x3_submatrix(float *dst, const struct matrix4 *m,
		int i, int j)
{
	const float *mf = (const float *)m;
	int ti, tj, idst, jdst;

	for (ti = 0; ti < 4; ti++) {
		if (ti < i)
			idst = ti;
		else if (ti > i)
			idst = ti-1;
		else
			continue;

		for (tj = 0; tj < 4; tj++) {
			if (tj < j)
				jdst = tj;
			else if (tj > j)
				jdst = tj-1;
			else
				continue;

			dst[(idst*3) + jdst] = mf[(ti*4) + tj];
		}
	}
}

static inline float get_3x3_determinant(const float *m)
{
	return (m[0] * ((m[4]*m[8]) - (m[7]*m[5]))) -
	       (m[1] * ((m[3]*m[8]) - (m[6]*m[5]))) +
	       (m[2] * ((m[3]*m[7]) - (m[6]*m[4])));
}

float matrix4_determinant(const struct matrix4 *m)
{
	const float *mf = (const float *)m;
	float det, result = 0.0f, i = 1.0f;
	float m3x3[9];
	int n;

	for (n = 0; n < 4; n++, i *= -1.0f) {
		get_3x3_submatrix(m3x3, m, 0, n);

		det     = get_3x3_determinant(m3x3);
		result += mf[n] * det * i;
	}

	return result;
}

bool matrix4_inv(struct matrix4 *dst, const struct matrix4 *m)
{
	struct vec4 *dstv = (struct vec4 *)dst;
	float det = matrix4_determinant(m);
	float m3x3[9];
	int   i, j, sign;

	if (fabs(det) < 0.0005f)
		return false;

	for (i = 0; i < 4; i++) {
		for (j = 0; j < 4; j++) {
			sign = 1 - ((i+j) % 2) * 2;
			get_3x3_submatrix(m3x3, m, i, j);
			dstv[j].ptr[i] = get_3x3_determinant(m3x3) *
			                  (float)sign / det;
		}
	}

	return true;
}

void matrix4_transpose(struct matrix4 *dst, const struct matrix4 *m)
{
	struct matrix4 temp;

	/* TODO: Add SSE */
	temp.x.x = m->x.x;
	temp.x.y = m->y.x;
	temp.x.z = m->z.x;
	temp.x.w = m->t.x;
	temp.y.x = m->x.y;
	temp.y.y = m->y.y;
	temp.y.z = m->z.y;
	temp.y.w = m->t.y;
	temp.z.x = m->x.z;
	temp.z.y = m->y.z;
	temp.z.z = m->z.z;
	temp.z.w = m->t.z;
	temp.t.x = m->x.w;
	temp.t.y = m->y.w;
	temp.t.z = m->z.w;
	temp.t.w = m->t.w;

	matrix4_copy(dst, &temp);
}
