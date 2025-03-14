#include "graphics-internal.h"
#include "../obs.h"

static inline bool can_cache_verts(graphics_t *graphics, size_t size)
{
	return size <= MAX_CACHABLE_VERTS && !graphics->norms.num && !graphics->colors.num &&
	       !graphics->texverts[1].num;
}

#ifndef SEC_TO_NSEC
#define SEC_TO_NSEC 1000000000ULL
#endif

static gs_vertbuffer_t *cache_verts(graphics_t *graphics, struct gs_vertex_buffer_cache *cache, size_t size)
{
	struct gs_vertex_buffer_cache_item item;
	bool has_uvs = !!graphics->texverts[0].num;
	uint64_t ts = obs_get_video_frame_time();
	gs_vertbuffer_t *vb = NULL;

	for (size_t i = 0; i < cache->items.num; i++) {
		item = cache->items.array[i];

		/* end of unfilled cache */
		if (!item.data.size) {
			break;
		}

		if (!vb && item.data.size == size && item.data.has_uvs == has_uvs) {
			if (memcmp(graphics->verts.array, item.data.verts, size * sizeof(struct vec3)) != 0) {
				continue;
			}
			if (has_uvs &&
			    memcmp(graphics->texverts[0].array, item.data.uvs, size * sizeof(struct vec2)) != 0) {
				continue;
			}

			if (i != 0) {
				memmove(&cache->items.array[1], &cache->items.array[0], sizeof(item) * i);
				cache->items.array[0] = item;
			}

			vb = item.vb;

		} else if ((ts - item.last_used_ts) > (5 * SEC_TO_NSEC)) {
			gs_vertexbuffer_destroy(item.vb);
			memset(&cache->items.array[i], 0, sizeof(item));
		}
	}

	if (vb) {
		cache->items.array[0].last_used_ts = ts;
		return vb;
	}

	struct gs_vertex_buffer_cache_item *last = &cache->items.array[cache->items.num - 1];
	if (last->vb) {
		gs_vertexbuffer_destroy(last->vb);
	}

	memmove(&cache->items.array[1], &cache->items.array[0], sizeof(item) * (cache->items.num - 1));

	item.data.size = size;
	item.data.has_uvs = has_uvs;
	memcpy(item.data.verts, graphics->verts.array, sizeof(item.data.verts[0]) * size);
	if (has_uvs) {
		memcpy(item.data.uvs, graphics->texverts[0].array, sizeof(item.data.uvs[0]) * size);
	}

	struct gs_vb_data vbd = {0};
	struct gs_tvertarray tvs = {0};
	vbd.num = size;
	vbd.points = &item.data.verts[0];
	vbd.num_tex = has_uvs ? 1 : 0;
	if (has_uvs) {
		tvs.width = 2;
		tvs.array = &item.data.uvs[0];
		vbd.tvarray = &tvs;
	}

	item.vb = gs_vertexbuffer_create(&vbd, GS_DUP_BUFFER);
	item.last_used_ts = ts;

	memcpy(&cache->items.array[0], &item, sizeof(item));
	return item.vb;
}

bool try_cache_verts(graphics_t *graphics, struct gs_vertex_buffer_cache *cache, size_t size)
{
	if (!can_cache_verts(graphics, size)) {
		blog(LOG_ERROR, "try_cache_verts: Tried to use vertices "
				"outside of the vertex caching limitations "
				"(maximum 16 points and optionally one equal "
				"amount of texture coordinates) ");
		return false;
	}

	gs_vertbuffer_t *vb = cache_verts(graphics, cache, size);
	if (!vb) {
		blog(LOG_ERROR, "try_cache_verts: cache_verts() failed "
				"unexpectedly. File an issue on the repository "
				"if you see this.");
		return false;
	}

	gs_load_vertexbuffer(vb);
	return true;
}

gs_vertexbuffer_cache_t *gs_vertexbuffer_cache_create(size_t size)
{
	struct gs_vertex_buffer_cache *cache = bzalloc(sizeof(*cache));
	da_resize(cache->items, size);
	return cache;
}

void gs_vertexbuffer_cache_destroy(gs_vertexbuffer_cache_t *cache)
{
	if (!cache) {
		return;
	}

	for (size_t i = 0; i < cache->items.num; i++) {
		struct gs_vertex_buffer_cache_item *item = &cache->items.array[i];

		/* end of unfilled cache */
		if (!item->vb) {
			break;
		}

		gs_vertexbuffer_destroy(item->vb);
	}

	da_free(cache->items);
	bfree(cache);
}

void gs_render_stop_cached(gs_vertexbuffer_cache_t *cache, enum gs_draw_mode mode)
{
	gs_render_stop_internal(cache, mode);
}
