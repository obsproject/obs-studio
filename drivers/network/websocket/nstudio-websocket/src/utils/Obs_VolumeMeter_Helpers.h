/*
obs-websocket
Copyright (C) 2014 by Leonhard Oelke <leonhard@in-verted.de>
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

// All of this is literally magic

// It should probably be imported as a libobs util someday though.

#define SHIFT_RIGHT_2PS(msb, lsb)                                               \
	{                                                                       \
		__m128 tmp = _mm_shuffle_ps(lsb, msb, _MM_SHUFFLE(0, 0, 3, 3)); \
		lsb = _mm_shuffle_ps(lsb, tmp, _MM_SHUFFLE(2, 1, 2, 1));        \
		msb = _mm_shuffle_ps(msb, msb, _MM_SHUFFLE(3, 3, 2, 1));        \
	}

#define abs_ps(v) _mm_andnot_ps(_mm_set1_ps(-0.f), v)

#define VECTOR_MATRIX_CROSS_PS(out, v, m0, m1, m2, m3)    \
	{                                                 \
		out = _mm_mul_ps(v, m0);                  \
		__m128 mul1 = _mm_mul_ps(v, m1);          \
		__m128 mul2 = _mm_mul_ps(v, m2);          \
		__m128 mul3 = _mm_mul_ps(v, m3);          \
                                                          \
		_MM_TRANSPOSE4_PS(out, mul1, mul2, mul3); \
                                                          \
		out = _mm_add_ps(out, mul1);              \
		out = _mm_add_ps(out, mul2);              \
		out = _mm_add_ps(out, mul3);              \
	}

#define hmax_ps(r, x4)                     \
	do {                               \
		float x4_mem[4];           \
		_mm_storeu_ps(x4_mem, x4); \
		r = x4_mem[0];             \
		r = fmaxf(r, x4_mem[1]);   \
		r = fmaxf(r, x4_mem[2]);   \
		r = fmaxf(r, x4_mem[3]);   \
	} while (false)

static float GetSamplePeak(__m128 previousSamples, const float *samples, size_t sampleCount)
{
	__m128 peak = previousSamples;
	for (size_t i = 0; (i + 3) < sampleCount; i += 4) {
		__m128 newWork = _mm_load_ps(&samples[i]);
		peak = _mm_max_ps(peak, abs_ps(newWork));
	}

	float ret;
	hmax_ps(ret, peak);
	return ret;
}

static float GetTruePeak(__m128 previousSamples, const float *samples, size_t sampleCount)
{
	const __m128 m3 = _mm_set_ps(-0.155915f, 0.935489f, 0.233872f, -0.103943f);
	const __m128 m1 = _mm_set_ps(-0.216236f, 0.756827f, 0.504551f, -0.189207f);
	const __m128 p1 = _mm_set_ps(-0.189207f, 0.504551f, 0.756827f, -0.216236f);
	const __m128 p3 = _mm_set_ps(-0.103943f, 0.233872f, 0.935489f, -0.155915f);

	__m128 work = previousSamples;
	__m128 peak = previousSamples;
	for (size_t i = 0; (i + 3) < sampleCount; i += 4) {
		__m128 newWork = _mm_load_ps(&samples[i]);
		__m128 intrpSamples;

		__m128 absNewWork = abs_ps(newWork);
		peak = _mm_max_ps(peak, absNewWork);

		SHIFT_RIGHT_2PS(newWork, work);
		VECTOR_MATRIX_CROSS_PS(intrpSamples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrpSamples));

		SHIFT_RIGHT_2PS(newWork, work);
		VECTOR_MATRIX_CROSS_PS(intrpSamples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrpSamples));

		SHIFT_RIGHT_2PS(newWork, work);
		VECTOR_MATRIX_CROSS_PS(intrpSamples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrpSamples));

		SHIFT_RIGHT_2PS(newWork, work);
		VECTOR_MATRIX_CROSS_PS(intrpSamples, work, m3, m1, p1, p3);
		peak = _mm_max_ps(peak, abs_ps(intrpSamples));
	}

	float ret;
	hmax_ps(ret, peak);
	return ret;
}
