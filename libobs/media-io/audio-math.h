/******************************************************************************
    Copyright (C) 2015 by Hugh Bailey <obs.jim@gmail.com>

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

#include "../util/c99defs.h"
#include <math.h>

#ifdef _MSC_VER
#include <float.h>

#pragma warning(push)
#pragma warning(disable : 4056)
#pragma warning(disable : 4756)
#endif

static inline float mul_to_db(const float mul)
{
	return (mul == 0.0f) ? -INFINITY : (20.0f * log10f(mul));
}

static inline float db_to_mul(const float db)
{
	return isfinite((double)db) ? powf(10.0f, db / 20.0f) : 0.0f;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif
