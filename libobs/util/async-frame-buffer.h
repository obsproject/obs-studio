#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define OBS_ASYNC_BUFFER_MAX_FRAMES 30
#define OBS_ASYNC_BUFFER_MAX_RATE_COMPONENT 1000000

struct obs_async_buffer_config {
	bool enabled;
	uint32_t target_frames;
	uint32_t max_frames;
	bool rebuild_timestamps;
	uint32_t fps_numerator;
	uint32_t fps_denominator;
};

struct async_frame_buffer {
	bool playing;
	bool arrival_valid;
	uint64_t last_arrival;
	uint64_t output_anchor;
	uint64_t capture_anchor;
	uint64_t next_timestamp;
	uint64_t remainder;
};

struct async_frame_buffer_selection {
	size_t discard;
	bool output;
};

static inline struct obs_async_buffer_config async_frame_buffer_normalize(struct obs_async_buffer_config config)
{
	if (!config.target_frames)
		config.target_frames = 1;
	if (config.target_frames > OBS_ASYNC_BUFFER_MAX_FRAMES)
		config.target_frames = OBS_ASYNC_BUFFER_MAX_FRAMES;
	if (config.max_frames < config.target_frames)
		config.max_frames = config.target_frames;
	if (config.max_frames > OBS_ASYNC_BUFFER_MAX_FRAMES)
		config.max_frames = OBS_ASYNC_BUFFER_MAX_FRAMES;
	if (!config.fps_numerator || config.fps_numerator > OBS_ASYNC_BUFFER_MAX_RATE_COMPONENT)
		config.fps_numerator = 60;
	if (!config.fps_denominator || config.fps_denominator > OBS_ASYNC_BUFFER_MAX_RATE_COMPONENT)
		config.fps_denominator = 1;
	return config;
}

static inline void async_frame_buffer_reset(struct async_frame_buffer *buffer)
{
	buffer->playing = false;
	buffer->arrival_valid = false;
	buffer->last_arrival = 0;
	buffer->output_anchor = 0;
	buffer->capture_anchor = 0;
	buffer->next_timestamp = 0;
	buffer->remainder = 0;
}

static inline bool async_frame_buffer_arrived(struct async_frame_buffer *buffer, uint64_t timestamp,
					      bool rebuild_timestamps)
{
	const bool discontinuity =
		!rebuild_timestamps && buffer->arrival_valid &&
		(timestamp < buffer->last_arrival || timestamp - buffer->last_arrival > 2000000000ULL);
	buffer->last_arrival = timestamp;
	buffer->arrival_valid = true;
	return discontinuity;
}

static inline size_t async_frame_buffer_trim(const struct obs_async_buffer_config *config, size_t count)
{
	return count > config->max_frames ? count - config->target_frames : 0;
}

static inline uint64_t async_frame_buffer_interval(const struct obs_async_buffer_config *config)
{
	return 1000000000ULL * config->fps_denominator / config->fps_numerator;
}

static inline void async_frame_buffer_advance(struct async_frame_buffer *buffer,
					      const struct obs_async_buffer_config *config)
{
	const uint64_t duration = 1000000000ULL * config->fps_denominator;
	buffer->next_timestamp += duration / config->fps_numerator;
	buffer->remainder += duration % config->fps_numerator;
	buffer->next_timestamp += buffer->remainder / config->fps_numerator;
	buffer->remainder %= config->fps_numerator;
}

static inline struct async_frame_buffer_selection
async_frame_buffer_select(struct async_frame_buffer *buffer, const struct obs_async_buffer_config *config,
			  const uint64_t *capture_timestamps, size_t count, uint64_t output_timestamp,
			  uint64_t output_interval)
{
	struct async_frame_buffer_selection result = {0, false};
	if (!count) {
		buffer->playing = false;
		return result;
	}
	if (!buffer->playing) {
		if (count < config->target_frames)
			return result;
		buffer->output_anchor = output_timestamp;
		buffer->capture_anchor = capture_timestamps[0];
		buffer->next_timestamp = output_timestamp;
		buffer->remainder = 0;
		buffer->playing = true;
	}
	const uint64_t tolerance = output_interval / 5 * 3 + output_interval % 5 * 3 / 5;
	for (size_t index = 0; index < count; index++) {
		uint64_t timestamp = buffer->next_timestamp;
		if (!config->rebuild_timestamps) {
			if (capture_timestamps[index] < buffer->capture_anchor) {
				buffer->playing = false;
				result.discard = index + 1;
				return result;
			}
			timestamp = buffer->output_anchor + capture_timestamps[index] - buffer->capture_anchor;
		}
		if (timestamp > output_timestamp && timestamp - output_timestamp > tolerance)
			return result;
		if (config->rebuild_timestamps)
			async_frame_buffer_advance(buffer, config);
		if (timestamp >= output_timestamp || output_timestamp - timestamp <= tolerance) {
			result.output = true;
			buffer->playing = index + 1 < count;
			return result;
		}
		result.discard++;
	}
	buffer->playing = false;
	return result;
}
