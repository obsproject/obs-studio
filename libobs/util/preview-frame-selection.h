#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct preview_frame_candidate {
	uint64_t timestamp;
	uint64_t sequence;
	bool ready;
};

static inline bool preview_frame_expired(uint64_t timestamp, uint64_t now, uint64_t refresh_interval)
{
	return timestamp <= now && now - timestamp > refresh_interval + refresh_interval / 2;
}

static inline size_t preview_frame_select(const struct preview_frame_candidate *frames, size_t count, uint64_t now,
					  uint64_t refresh_interval, uint64_t last_sequence)
{
	const uint64_t maximum_age = refresh_interval + refresh_interval / 4;
	size_t selected = SIZE_MAX;
	for (size_t index = 0; index < count; index++) {
		const struct preview_frame_candidate *frame = &frames[index];
		if (!frame->ready || frame->sequence <= last_sequence || frame->timestamp > now ||
		    now - frame->timestamp > maximum_age)
			continue;
		if (selected == SIZE_MAX || frame->timestamp < frames[selected].timestamp ||
		    (frame->timestamp == frames[selected].timestamp && frame->sequence < frames[selected].sequence))
			selected = index;
	}
	return selected;
}
