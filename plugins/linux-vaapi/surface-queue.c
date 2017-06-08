#include "surface-queue.h"

#include <util/platform.h>

struct surface_queue {
	VADisplay display;
	VAContextID context;
	DARRAY(VASurfaceID) available;
	DARRAY(surface_entry_t) rendering;
	DARRAY(coded_block_entry_t) finished;
	size_t size;
	size_t width;
	size_t height;
	size_t output_size;
};

surface_queue_t *surface_queue_create(VADisplay display, VAContextID context,
		size_t size, size_t width, size_t height)
{
	surface_queue_t *q = bzalloc(sizeof(surface_queue_t));

	q->display = display;
	q->context = context;
	q->size = size;
	q->width = width;
	q->height = height;

	q->output_size = round_up_to_power_of_2(width * height * 4, 16);

	da_reserve(q->available, size);
	da_reserve(q->rendering, size);
	da_reserve(q->finished, size);

	return q;
}

static void reset_entry(surface_queue_t *q, surface_entry_t *s)
{
	for(size_t i = 0; i < s->list.num; i++) {
		VABufferID *buffer = darray_item(sizeof(VABufferID),
				&s->list, i);
		vaDestroyBuffer(q->display, *buffer);
	}
	vaDestroyBuffer(q->display, s->output);
	darray_free(&s->list);
}

void surface_queue_destroy(surface_queue_t *q)
{
	if (q != NULL) {
		vaDestroySurfaces(q->display, q->available.array,
				q->available.num);
		da_free(q->available);
		for (size_t i = 0; i < q->rendering.num; i++) {
			reset_entry(q, &q->rendering.array[i]);
		}
		da_free(q->rendering);
		for(size_t i = 0; i < q->finished.num; i++) {
			coded_block_entry_t *c = &q->finished.array[i];
			da_free(c->data);

		}
		da_free(q->finished);
		bfree(q);
	}
}

static bool make_finished(surface_queue_t *q, surface_entry_t *e,
		coded_block_entry_t *c)
{

	VACodedBufferSegment *segment;
	VAStatus status;

	CHECK_STATUS_FALSE(vaSyncSurface(q->display, e->surface));

	CHECK_STATUS_FAIL(vaMapBuffer(q->display, e->output,
			(void **)&segment));

	coded_block_entry_t cb = {
		.pts = e->pts,
		.type = e->type
	};

	while(segment != NULL) {
		if (segment->status & VA_CODED_BUF_STATUS_SLICE_OVERFLOW_MASK) {
			VA_LOG(LOG_ERROR, "surface output buffer too small");
			goto fail;
		}

		da_push_back_array(cb.data, segment->buf, segment->size);

		segment = segment->next;
	}

	CHECK_STATUS_FAILN(vaUnmapBuffer(q->display, e->output), 1);

	*c = cb;

	return true;

fail1:
	da_free(cb.data);
	return false;

fail:
	vaUnmapBuffer(q->display, e->output);

	return false;

}

bool create_input_surface(surface_queue_t *q, VASurfaceID *input)
{
	VAStatus status;
	VASurfaceID surface;

	CHECK_STATUS_FALSE(vaCreateSurfaces(q->display, VA_RT_FORMAT_YUV420,
			q->width, q->height, &surface, 1, NULL, 0));

	*input = surface;
	return true;
}

bool surface_queue_pop_available(surface_queue_t *q, VASurfaceID *surface)
{
	if (q->available.num == 0) {
		if (q->size - q->rendering.num > 0) {
			return create_input_surface(q, surface);
		}
		assert(q->rendering.num > 0);
		surface_entry_t *e = &q->rendering.array[0];

		coded_block_entry_t c;
		if (!make_finished(q, e, &c))
			return false;
		da_push_back(q->finished, &c);

		reset_entry(q, e);
		*surface = e->surface;
		da_erase(q->rendering, 0);

		return true;
	} else {
		*surface = q->available.array[0];
		da_erase(q->available, 0);

		return true;
	}
}

static bool render_surface(surface_queue_t *q, surface_entry_t *e)
{
	VAStatus status;

	CHECK_STATUS_FALSE(vaBeginPicture(q->display, q->context, e->surface));
	CHECK_STATUS_FALSE(vaRenderPicture(q->display, q->context,
			(VABufferID *)e->list.array, e->list.num));
	CHECK_STATUS_FALSE(vaEndPicture(q->display, q->context));

	return true;
}

bool surface_queue_push_and_render(surface_queue_t *q, surface_entry_t *e)
{
	if (!render_surface(q, e))
		return false;

	da_push_back(q->rendering, e);

	return true;
}

bool surface_queue_pop_finished(surface_queue_t *q, coded_block_entry_t *c,
		bool *success)
{
 	if (q->finished.num > 0) {
		*c = q->finished.array[0];
		da_erase(q->finished, 0);
		*success = true;
		return true;
	} else if (q->rendering.num > 0) {
		surface_entry_t *e = &q->rendering.array[0];

		VAStatus status;
		VASurfaceStatus surface_status;

		CHECK_STATUS_FALSE(vaQuerySurfaceStatus(q->display, e->surface,
				&surface_status));

		// next surface in rendering queue isn't finished
		if (surface_status != VASurfaceReady) {
			*success = false;
			return true;
		}

		if (!make_finished(q, e, c)) {
			return false;
		}
		da_push_back(q->available, &e->surface);

		reset_entry(q, e);
		da_erase(q->rendering, 0);

		*success = true;
		return true;
	}
	// no rendering or finished surfaces available
	*success = false;
	return true;
}
