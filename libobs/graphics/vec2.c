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

#include <math.h>
#include "math-extra.h"
#include "math-defs.h"
#include "vec2.h"

void vec2_abs(struct vec2 *dst, const struct vec2 *v)
{
	vec2_set(dst, fabsf(v->x), fabsf(v->y));
}

void vec2_floor(struct vec2 *dst, const struct vec2 *v)
{
	vec2_set(dst, floorf(v->x), floorf(v->y));
}

void vec2_ceil(struct vec2 *dst, const struct vec2 *v)
{
	vec2_set(dst, ceilf(v->x), ceilf(v->y));
}

int vec2_close(const struct vec2 *v1, const struct vec2 *v2, float epsilon)
{
	return close_float(v1->x, v2->x, epsilon) &&
	       close_float(v1->y, v2->y, epsilon);
}

void vec2_norm(struct vec2 *dst, const struct vec2 *v)
{
	float len = vec2_len(v);

	if (len > 0.0f) {
		len = 1.0f / len;
		vec2_mulf(dst, v, len);
	}
}
