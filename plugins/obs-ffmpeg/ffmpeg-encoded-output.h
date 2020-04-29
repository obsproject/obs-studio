/******************************************************************************
    Copyright (C) 2019 Haivision Systems Inc.
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#include <obs-module.h>
#include <obs-avc.h>
#include <util/platform.h>
#include <util/circlebuf.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <inttypes.h>
#include "obs-ffmpeg-output.h"
#include "obs-ffmpeg-formats.h"

#define do_log(level, format, ...)                           \
	blog(level, "[ffmpeg-encoded-output: '%s'] " format, \
	     obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

#define OPT_DROP_THRESHOLD "drop_threshold_ms"
#define OPT_PFRAME_DROP_THRESHOLD "pframe_drop_threshold_ms"
#define OPT_MAX_SHUTDOWN_TIME_SEC "max_shutdown_time_sec"
#define OPT_LOWLATENCY_ENABLED "low_latency_mode_enabled"

//#define TEST_FRAMEDROPS

#ifdef TEST_FRAMEDROPS

#define DROPTEST_MAX_KBPS 3000
#define DROPTEST_MAX_BYTES (DROPTEST_MAX_KBPS * 1000 / 8)

struct droptest_info {
	uint64_t ts;
	size_t size;
};
#endif

struct ffmpeg_encoded_output {
	obs_output_t *output;

	pthread_mutex_t packets_mutex;
	struct circlebuf packets;
	bool sent_sps_pps;

	bool got_first_video;
	int64_t start_dts_offset;

	volatile bool connecting;
	pthread_t connect_thread;

	volatile bool active;
	volatile bool disconnected;
	pthread_t send_thread;

	int max_shutdown_time_sec;

	os_sem_t *send_sem;
	os_event_t *stop_event;
	uint64_t stop_ts;
	uint64_t shutdown_timeout_ts;

	struct dstr path, key;
	struct dstr username, password;
	struct dstr encoder_name;

	/* frame drop variables */
	int64_t drop_threshold_usec;
	int64_t pframe_drop_threshold_usec;
	int min_priority;
	float congestion;

	int64_t last_dts_usec;

	uint64_t total_bytes_sent;
	int dropped_frames;

#ifdef TEST_FRAMEDROPS
	struct circlebuf droptest_info;
	size_t droptest_size;
#endif

	uint8_t *write_buf;
	size_t write_buf_len;
	size_t write_buf_size;
	pthread_mutex_t write_buf_mutex;
	os_event_t *buffer_space_available_event;
	os_event_t *buffer_has_data_event;
	os_event_t *send_thread_signaled_exit;

	struct ffmpeg_data ff_data;
};
