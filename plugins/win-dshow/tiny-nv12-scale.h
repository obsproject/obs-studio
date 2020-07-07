#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct nv12_scale {
	bool convert_to_i420;

	int src_cx;
	int src_cy;

	int dst_cx;
	int dst_cy;
};

typedef struct nv12_scale nv12_scale_t;

extern void nv12_scale_init(nv12_scale_t *s, bool convert_to_i420, int dst_cx,
			    int dst_cy, int src_cx, int src_cy);
extern void nv12_do_scale(nv12_scale_t *s, uint8_t *dst, const uint8_t *src);

#ifdef __cplusplus
}
#endif
