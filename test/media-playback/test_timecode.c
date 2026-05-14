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

#include <stdint.h>
#include <stdio.h>

#define NSEC_PER_SEC 1000000000LL

static uint32_t s12m_timecode(uint32_t hours, uint32_t minutes, uint32_t seconds, uint32_t frame)
{
	return ((frame / 10) << 28) | ((frame % 10) << 24) | ((seconds / 10) << 20) | ((seconds % 10) << 16) |
	       ((minutes / 10) << 12) | ((minutes % 10) << 8) | ((hours / 10) << 4) | (hours % 10);
}

static bool parses_s12m_timecode(void)
{
	uint32_t data[] = {
		1,
		s12m_timecode(12, 34, 56, 7),
		0,
		0,
	};
	int64_t timestamp = 0;
	const int64_t frame_duration = 33333333;
	const int64_t expected = ((12 * 60 + 34) * 60 + 56) * NSEC_PER_SEC + 7 * frame_duration;

	return mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){30, 1}, frame_duration,
				      &timestamp) &&
	       timestamp == expected;
}

static bool parses_high_frame_rate_s12m_timecode(void)
{
	uint32_t data[] = {
		1,
		s12m_timecode(1, 2, 3, 5) | (1U << 7),
		0,
		0,
	};
	int64_t timestamp = 0;
	const int64_t frame_duration = 20000000;
	const int64_t expected = ((1 * 60 + 2) * 60 + 3) * NSEC_PER_SEC + 11 * frame_duration;

	return mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){50, 1}, frame_duration,
				      &timestamp) &&
	       timestamp == expected;
}

static bool parses_60hz_s12m_timecode(void)
{
	uint32_t data[] = {
		1,
		s12m_timecode(1, 2, 3, 5) | (1U << 23),
		0,
		0,
	};
	int64_t timestamp = 0;
	const int64_t frame_duration = 16683333;
	const int64_t expected = (((1 * 60 + 2) * 60 + 3) * 60 + 11) * frame_duration;

	return mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){60000, 1001}, frame_duration,
				      &timestamp) &&
	       timestamp == expected;
}

static bool parses_drop_frame_s12m_timecode(void)
{
	uint32_t data[] = {
		1,
		s12m_timecode(1, 2, 3, 4) | (1U << 30),
		0,
		0,
	};
	int64_t timestamp = 0;
	const int64_t frame_duration = 33366667;
	const int64_t dropped_frames = 2 * (62 - 6);
	const int64_t expected = (((1 * 60 + 2) * 60 + 3) * 30 + 4 - dropped_frames) * frame_duration;

	return mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){30000, 1001}, frame_duration,
				      &timestamp) &&
	       timestamp == expected;
}

static bool keeps_fractional_s12m_frame_cadence(void)
{
	uint32_t frame29[] = {
		1,
		s12m_timecode(0, 0, 0, 29),
		0,
		0,
	};
	uint32_t frame30[] = {
		1,
		s12m_timecode(0, 0, 1, 0),
		0,
		0,
	};
	int64_t first = 0;
	int64_t second = 0;
	const int64_t frame_duration = 33366667;

	if (!mp_s12m_timecode_parse((const uint8_t *)frame29, sizeof(frame29), (AVRational){30000, 1001},
				    frame_duration, &first))
		return false;
	if (!mp_s12m_timecode_parse((const uint8_t *)frame30, sizeof(frame30), (AVRational){30000, 1001},
				    frame_duration, &second))
		return false;

	return second - first == frame_duration;
}

static bool rejects_invalid_s12m_count(void)
{
	uint32_t data[] = {
		4,
		s12m_timecode(12, 34, 56, 7),
		0,
		0,
	};
	int64_t timestamp = 0;

	return !mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){30, 1}, 33333333, &timestamp);
}

static bool rejects_truncated_s12m_timecode(void)
{
	uint32_t data[] = {
		2,
		s12m_timecode(12, 34, 56, 7),
	};
	int64_t timestamp = 0;

	return !mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){30, 1}, 33333333, &timestamp);
}

static bool rejects_invalid_s12m_value(void)
{
	uint32_t data[] = {
		1,
		s12m_timecode(29, 34, 56, 7),
		0,
		0,
	};
	int64_t timestamp = 0;

	return !mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){30, 1}, 33333333, &timestamp);
}

static bool rejects_out_of_range_s12m_frame(void)
{
	uint32_t data[] = {
		1,
		s12m_timecode(12, 34, 56, 35),
		0,
		0,
	};
	int64_t timestamp = 0;

	return !mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){30, 1}, 33333333, &timestamp);
}

static bool rejects_invalid_drop_frame_s12m_timecode(void)
{
	uint32_t data[] = {
		1,
		s12m_timecode(1, 2, 0, 1) | (1U << 30),
		0,
		0,
	};
	int64_t timestamp = 0;

	return !mp_s12m_timecode_parse((const uint8_t *)data, sizeof(data), (AVRational){30000, 1001}, 33366667,
				       &timestamp);
}

int main(void)
{
	if (!parses_s12m_timecode()) {
		fprintf(stderr, "parses_s12m_timecode failed\n");
		return 1;
	}

	if (!parses_high_frame_rate_s12m_timecode()) {
		fprintf(stderr, "parses_high_frame_rate_s12m_timecode failed\n");
		return 1;
	}

	if (!parses_60hz_s12m_timecode()) {
		fprintf(stderr, "parses_60hz_s12m_timecode failed\n");
		return 1;
	}

	if (!parses_drop_frame_s12m_timecode()) {
		fprintf(stderr, "parses_drop_frame_s12m_timecode failed\n");
		return 1;
	}

	if (!keeps_fractional_s12m_frame_cadence()) {
		fprintf(stderr, "keeps_fractional_s12m_frame_cadence failed\n");
		return 1;
	}

	if (!rejects_invalid_s12m_count()) {
		fprintf(stderr, "rejects_invalid_s12m_count failed\n");
		return 1;
	}

	if (!rejects_truncated_s12m_timecode()) {
		fprintf(stderr, "rejects_truncated_s12m_timecode failed\n");
		return 1;
	}

	if (!rejects_invalid_s12m_value()) {
		fprintf(stderr, "rejects_invalid_s12m_value failed\n");
		return 1;
	}

	if (!rejects_out_of_range_s12m_frame()) {
		fprintf(stderr, "rejects_out_of_range_s12m_frame failed\n");
		return 1;
	}

	if (!rejects_invalid_drop_frame_s12m_timecode()) {
		fprintf(stderr, "rejects_invalid_drop_frame_s12m_timecode failed\n");
		return 1;
	}

	return 0;
}
