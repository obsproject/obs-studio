/*
 * Copyright (c) 2026
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "timecode.h"

#include <string.h>

#define S12M_TIMECODE_WORDS 4
#define NSEC_PER_SEC 1000000000LL

static bool bcd_value(uint32_t tens, uint32_t units, uint32_t limit, uint32_t *value)
{
	if (tens > 9 || units > 9)
		return false;

	*value = tens * 10 + units;
	return *value <= limit;
}

static uint32_t timecode_frame_count(AVRational frame_rate)
{
	int64_t frames = 0;

	if (frame_rate.num <= 0 || frame_rate.den <= 0)
		return 0;

	frames = ((int64_t)frame_rate.num + frame_rate.den / 2) / frame_rate.den;
	return frames > 0 && frames <= UINT32_MAX ? (uint32_t)frames : 0;
}

static int64_t timecode_frame_number(uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t frames,
				     uint32_t frame)
{
	return ((hours * 60LL + minutes) * 60LL + seconds) * frames + frame;
}

static bool read_s12m_timecode(uint32_t timecode, AVRational frame_rate, int64_t frame_duration, int64_t *timestamp)
{
	uint32_t hours = 0;
	uint32_t minutes = 0;
	uint32_t seconds = 0;
	uint32_t frame = 0;
	const uint32_t frames = timecode_frame_count(frame_rate);
	const bool drop_frame = !!(timecode & (1U << 30));

	if (frame_duration <= 0 || !frames)
		return false;

	if (!bcd_value((timecode >> 4) & 0x3, timecode & 0xf, 23, &hours))
		return false;
	if (!bcd_value((timecode >> 12) & 0x7, (timecode >> 8) & 0xf, 59, &minutes))
		return false;
	if (!bcd_value((timecode >> 20) & 0x7, (timecode >> 16) & 0xf, 59, &seconds))
		return false;
	if (!bcd_value((timecode >> 28) & 0x3, (timecode >> 24) & 0xf, 39, &frame))
		return false;

	if (frame_rate.num > 0 && frame_rate.den > 0 && av_cmp_q(frame_rate, (AVRational){30, 1}) > 0) {
		const bool field = av_cmp_q(frame_rate, (AVRational){50, 1}) == 0 ? !!(timecode & (1U << 7))
										  : !!(timecode & (1U << 23));
		frame = frame * 2 + field;
	}

	if (frame >= frames)
		return false;

	if (drop_frame) {
		const uint32_t drop_frames = frames / 30 * 2;
		const uint32_t total_minutes = hours * 60 + minutes;
		const int64_t frame_count = timecode_frame_number(hours, minutes, seconds, frames, frame);

		if (!drop_frames || frames % 30)
			return false;
		if (seconds == 0 && minutes % 10 && frame < drop_frames)
			return false;

		*timestamp = (frame_count - drop_frames * (total_minutes - total_minutes / 10)) * frame_duration;
	} else if (av_cmp_q(frame_rate, (AVRational){frames, 1}) != 0) {
		*timestamp = timecode_frame_number(hours, minutes, seconds, frames, frame) * frame_duration;
	} else {
		*timestamp = ((hours * 60LL + minutes) * 60LL + seconds) * NSEC_PER_SEC + frame * frame_duration;
	}

	return true;
}

bool mp_s12m_timecode_parse(const uint8_t *data, size_t size, AVRational frame_rate, int64_t frame_duration,
			    int64_t *timestamp)
{
	uint32_t values[S12M_TIMECODE_WORDS] = {0};
	uint32_t count = 0;

	if (!data || !timestamp || size < sizeof(uint32_t) * 2)
		return false;

	if (size > sizeof(values))
		size = sizeof(values);

	memcpy(values, data, size);

	count = values[0];
	if (count < 1 || count > 3 || size < sizeof(uint32_t) * (count + 1))
		return false;

	return read_s12m_timecode(values[1], frame_rate, frame_duration, timestamp);
}
