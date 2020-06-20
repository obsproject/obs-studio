#include <string.h>
#include "tiny-nv12-scale.h"

/* TODO: optimize this stuff later, or replace with something better.  it's
 * kind of garbage.  although normally it shouldn't be called that often.  plus
 * it's nearest neighbor so not really a huge deal.  at the very least it
 * should be sse2 at some point. */

void nv12_scale_init(nv12_scale_t *s, bool convert_to_i420, int dst_cx,
		     int dst_cy, int src_cx, int src_cy)
{
	s->convert_to_i420 = convert_to_i420;

	s->src_cx = src_cx;
	s->src_cy = src_cy;

	s->dst_cx = dst_cx;
	s->dst_cy = dst_cy;
}

static void nv12_scale_nearest(nv12_scale_t *s, uint8_t *dst_start,
			       const uint8_t *src)
{
	register uint8_t *dst = dst_start;
	const int src_cx = s->src_cx;
	const int src_cy = s->src_cy;
	const int dst_cx = s->dst_cx;
	const int dst_cy = s->dst_cy;

	/* lum */
	for (int y = 0; y < dst_cy; y++) {
		const int src_line = y * src_cy / dst_cy * s->src_cx;

		for (int x = 0; x < dst_cx; x++) {
			const int src_x = x * src_cx / dst_cx;

			*(dst++) = src[src_line + src_x];
		}
	}

	src += src_cx * src_cy;

	/* uv */
	const int dst_cx_d2 = dst_cx / 2;
	const int dst_cy_d2 = dst_cy / 2;

	for (int y = 0; y < dst_cy_d2; y++) {
		const int src_line = y * src_cy / dst_cy * src_cx;

		for (int x = 0; x < dst_cx_d2; x++) {
			const int src_x = x * src_cx / dst_cx * 2;
			const int pos = src_line + src_x;

			*(dst++) = src[pos];
			*(dst++) = src[pos + 1];
		}
	}
}

static void nv12_scale_nearest_to_i420(nv12_scale_t *s, uint8_t *dst_start,
				       const uint8_t *src)
{
	register uint8_t *dst = dst_start;
	const int src_cx = s->src_cx;
	const int src_cy = s->src_cy;
	const int dst_cx = s->dst_cx;
	const int dst_cy = s->dst_cy;
	const int size = src_cx * src_cy;

	/* lum */
	for (int y = 0; y < dst_cy; y++) {
		const int src_line = y * src_cy / dst_cy * s->src_cx;

		for (int x = 0; x < dst_cx; x++) {
			const int src_x = x * src_cx / dst_cx;

			*(dst++) = src[src_line + src_x];
		}
	}

	src += size;

	/* uv */
	const int dst_cx_d2 = dst_cx / 2;
	const int dst_cy_d2 = dst_cy / 2;

	register uint8_t *dst2 = dst + dst_cx * dst_cy / 4;

	for (int y = 0; y < dst_cy_d2; y++) {
		const int src_line = y * src_cy / dst_cy * src_cx;

		for (int x = 0; x < dst_cx_d2; x++) {
			const int src_x = x * src_cx / dst_cx * 2;
			const int pos = src_line + src_x;

			*(dst++) = src[pos];
			*(dst2++) = src[pos + 1];
		}
	}
}

static void nv12_convert_to_i420(nv12_scale_t *s, uint8_t *dst_start,
				 const uint8_t *src_start)
{
	const int size = s->src_cx * s->src_cy;
	const int size_d4 = size / 4;

	memcpy(dst_start, src_start, size);

	register uint8_t *dst1 = dst_start + size;
	register uint8_t *dst2 = dst1 + size_d4;
	register uint8_t *dst_end = dst2 + size_d4;
	register const uint8_t *src = src_start + size;

	while (dst2 < dst_end) {
		*(dst1++) = *(src++);
		*(dst2++) = *(src++);
	}
}

void nv12_do_scale(nv12_scale_t *s, uint8_t *dst, const uint8_t *src)
{
	if (s->src_cx == s->dst_cx && s->src_cy == s->dst_cy) {
		if (s->convert_to_i420)
			nv12_convert_to_i420(s, dst, src);
		else
			memcpy(dst, src, s->src_cx * s->src_cy * 3 / 2);
	} else {
		if (s->convert_to_i420)
			nv12_scale_nearest_to_i420(s, dst, src);
		else
			nv12_scale_nearest(s, dst, src);
	}
}
