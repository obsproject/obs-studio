#pragma once

#include <stdint.h>

/* This defines an interface between obs-drmsend and obs. */

#define OBS_DRMSEND_MAX_FRAMEBUFFERS 16
#define OBS_DRMSEND_TAG 0x0b500001u

typedef struct {
	uint32_t fb_id;
	int width, height;
	uint32_t fourcc;
	int offset, pitch;
	/* fds are delivered OOB using control msg */
} drmsend_framebuffer_t;

typedef struct {
	unsigned tag;
	int num_framebuffers;
	drmsend_framebuffer_t framebuffers[OBS_DRMSEND_MAX_FRAMEBUFFERS];
} drmsend_response_t;
