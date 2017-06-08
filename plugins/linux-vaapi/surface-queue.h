#pragma once

#include <va/va.h>
#include <va/va_x11.h>
#include "vaapi-common.h"

struct surface_queue;
typedef struct surface_queue surface_queue_t;

struct surface_entry {
	VASurfaceID surface;
	VABufferID output;
	buffer_list_t list;
	uint64_t pts;
	vaapi_slice_type_t type;
};
typedef struct surface_entry surface_entry_t;

surface_queue_t *surface_queue_create(VADisplay display, VAContextID context,
		size_t size, size_t width, size_t height);
void surface_queue_destroy(surface_queue_t *q);
bool surface_queue_pop_available(surface_queue_t *q, VASurfaceID *surface);
bool surface_queue_push_and_render(surface_queue_t *q, surface_entry_t *e);
bool surface_queue_pop_finished(surface_queue_t *q, coded_block_entry_t *c,
		bool *success);
