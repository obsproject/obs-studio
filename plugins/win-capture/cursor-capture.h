#pragma once

#include <stdint.h>

struct cursor_data {
	gs_texture_t                   *texture;
	HCURSOR                        current_cursor;
	POINT                          cursor_pos;
	long                           x_hotspot;
	long                           y_hotspot;
	bool                           visible;
};

extern void cursor_capture(struct cursor_data *data);
extern void cursor_draw(struct cursor_data *data, long x_offset, long y_offset,
		float x_scale, float y_scale, long width, long height);
extern void cursor_data_free(struct cursor_data *data);
