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

#include "ffmpeg-encoded-output.h"

static int proto_init(struct ffmpeg_encoded_output *stream);
static int proto_try_connect(struct ffmpeg_encoded_output *stream);
static int proto_connect_time(struct ffmpeg_encoded_output *stream);
static void proto_close(struct ffmpeg_encoded_output *stream);
static void proto_set_output_error(struct ffmpeg_encoded_output *stream);
static int proto_send_packet(struct ffmpeg_encoded_output *stream,
			     struct encoder_packet *packet, bool is_header);

static inline size_t num_buffered_packets(struct ffmpeg_encoded_output *stream);
static inline void free_packets(struct ffmpeg_encoded_output *stream);
static inline bool stopping(struct ffmpeg_encoded_output *stream);
static inline bool connecting(struct ffmpeg_encoded_output *stream);
static inline bool active(struct ffmpeg_encoded_output *stream);
static inline bool disconnected(struct ffmpeg_encoded_output *stream);
static inline bool get_next_packet(struct ffmpeg_encoded_output *stream,
				   struct encoder_packet *packet);
#ifdef TEST_FRAMEDROPS
static void droptest_cap_data_rate(struct ffmpeg_encoded_output *stream,
				   size_t size);
#endif
static inline bool can_shutdown_stream(struct ffmpeg_encoded_output *stream,
				       struct encoder_packet *packet);
static void *send_thread(void *data);
static bool send_sps_pps(struct ffmpeg_encoded_output *stream);
static inline bool reset_semaphore(struct ffmpeg_encoded_output *stream);
static bool init_connect(struct ffmpeg_encoded_output *stream);
static int init_send(struct ffmpeg_encoded_output *stream);
static void *connect_thread(void *data);
static inline bool add_packet(struct ffmpeg_encoded_output *stream,
			      struct encoder_packet *packet);
static inline size_t num_buffered_packets(struct ffmpeg_encoded_output *stream);
static void drop_frames(struct ffmpeg_encoded_output *stream, const char *name,
			int highest_priority, bool pframes);
static bool find_first_video_packet(struct ffmpeg_encoded_output *stream,
				    struct encoder_packet *first);
static void check_to_drop_frames(struct ffmpeg_encoded_output *stream,
				 bool pframes);
static bool add_video_packet(struct ffmpeg_encoded_output *stream,
			     struct encoder_packet *packet);

static const char *ffmpeg_encoded_output_getname(void *unused)
{
	UNUSED_PARAMETER(unused);

	return obs_module_text("FFmpegEncodedOutput");
}

static inline bool add_packet(struct ffmpeg_encoded_output *stream,
			      struct encoder_packet *packet)
{
	circlebuf_push_back(&stream->packets, packet,
			    sizeof(struct encoder_packet));

	return true;
}

static bool add_video_packet(struct ffmpeg_encoded_output *stream,
			     struct encoder_packet *packet)
{
	check_to_drop_frames(stream, false);
	check_to_drop_frames(stream, true);

	/* if currently dropping frames, drop packets until it reaches the
	 * desired priority */
	if (packet->drop_priority < stream->min_priority) {
		stream->dropped_frames++;
		return false;
	} else {
		stream->min_priority = 0;
	}

	stream->last_dts_usec = packet->dts_usec;

	return add_packet(stream, packet);
}

static inline bool get_next_packet(struct ffmpeg_encoded_output *stream,
				   struct encoder_packet *packet)
{
	bool new_packet = false;

	pthread_mutex_lock(&stream->packets_mutex);
	if (stream->packets.size) {
		circlebuf_pop_front(&stream->packets, packet,
				    sizeof(struct encoder_packet));
		new_packet = true;
	}
	pthread_mutex_unlock(&stream->packets_mutex);

	return new_packet;
}

static inline void free_packets(struct ffmpeg_encoded_output *stream)
{
	size_t num_packets;

	pthread_mutex_lock(&stream->packets_mutex);

	num_packets = num_buffered_packets(stream);
	if (num_packets)
		info("Freeing %d remaining packets", (int)num_packets);

	while (stream->packets.size) {
		struct encoder_packet packet;
		circlebuf_pop_front(&stream->packets, &packet, sizeof(packet));
		obs_encoder_packet_release(&packet);
	}
	pthread_mutex_unlock(&stream->packets_mutex);
}

static inline bool stopping(struct ffmpeg_encoded_output *stream)
{
	return os_event_try(stream->stop_event) != EAGAIN;
}

static inline bool connecting(struct ffmpeg_encoded_output *stream)
{
	return os_atomic_load_bool(&stream->connecting);
}

static inline bool active(struct ffmpeg_encoded_output *stream)
{
	return os_atomic_load_bool(&stream->active);
}

static inline bool disconnected(struct ffmpeg_encoded_output *stream)
{
	return os_atomic_load_bool(&stream->disconnected);
}

#ifdef TEST_FRAMEDROPS
static void droptest_cap_data_rate(struct ffmpeg_encoded_output *stream,
				   size_t size)
{
	uint64_t ts = os_gettime_ns();
	struct droptest_info info;

	info.ts = ts;
	info.size = size;

	circlebuf_push_back(&stream->droptest_info, &info, sizeof(info));
	stream->droptest_size += size;

	if (stream->droptest_info.size) {
		circlebuf_peek_front(&stream->droptest_info, &info,
				     sizeof(info));

		if (stream->droptest_size > DROPTEST_MAX_BYTES) {
			uint64_t elapsed = ts - info.ts;

			if (elapsed < 1000000000ULL) {
				elapsed = 1000000000ULL - elapsed;
				os_sleepto_ns(ts + elapsed);
			}

			while (stream->droptest_size > DROPTEST_MAX_BYTES) {
				circlebuf_pop_front(&stream->droptest_info,
						    &info, sizeof(info));
				stream->droptest_size -= info.size;
			}
		}
	}
}
#endif

static void *send_thread(void *data)
{
	struct ffmpeg_encoded_output *stream = data;

	os_set_thread_name("ffmpeg-stream: send_thread");

	while (os_sem_wait(stream->send_sem) == 0) {
		struct encoder_packet packet;

		if (stopping(stream) && stream->stop_ts == 0)
			break;

		if (!get_next_packet(stream, &packet))
			continue;

		if (stopping(stream)) {
			if (can_shutdown_stream(stream, &packet)) {
				obs_encoder_packet_release(&packet);
				break;
			}
		}
		if (!stream->sent_sps_pps) {
			if (!send_sps_pps(stream)) {
				os_atomic_set_bool(&stream->disconnected, true);
				break;
			}
		}
		if (proto_send_packet(stream, &packet, false) < 0) {
			os_atomic_set_bool(&stream->disconnected, true);
			break;
		}
	}

	if (disconnected(stream))
		info("Disconnected from %s", stream->path.array);
	else
		info("User stopped the stream");

	proto_set_output_error(stream);
	proto_close(stream);

	if (!stopping(stream)) {
		pthread_detach(stream->send_thread);
		obs_output_signal_stop(stream->output, OBS_OUTPUT_DISCONNECTED);
	} else {
		obs_output_end_data_capture(stream->output);
	}

	free_packets(stream);
	os_event_reset(stream->stop_event);
	os_atomic_set_bool(&stream->active, false);
	stream->sent_sps_pps = false;

	return NULL;
}

static bool send_sps_pps(struct ffmpeg_encoded_output *stream)
{
	obs_output_t *context = stream->output;
	obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
	uint8_t *header;
	struct encoder_packet packet = {
		.type = OBS_ENCODER_VIDEO, .timebase_den = 1, .keyframe = true};

	if (obs_encoder_get_extra_data(vencoder, &header, &packet.size)) {
		packet.data = bmemdup(header, packet.size);
		stream->sent_sps_pps =
			proto_send_packet(stream, &packet, true) >= 0;
	}
	return stream->sent_sps_pps;
}

static inline bool reset_semaphore(struct ffmpeg_encoded_output *stream)
{
	os_sem_destroy(stream->send_sem);

	return os_sem_init(&stream->send_sem, 0) == 0;
}

static bool init_connect(struct ffmpeg_encoded_output *stream)
{
	obs_service_t *service;
	obs_data_t *settings;
	int64_t drop_p;
	int64_t drop_b;

	if (stopping(stream))
		pthread_join(stream->send_thread, NULL);

	free_packets(stream);

	service = obs_output_get_service(stream->output);
	if (!service)
		return false;

	os_atomic_set_bool(&stream->disconnected, false);
	stream->total_bytes_sent = 0;
	stream->dropped_frames = 0;
	stream->min_priority = 0;
	stream->got_first_video = false;

	settings = obs_output_get_settings(stream->output);
	dstr_copy(&stream->path, obs_service_get_url(service));
	dstr_copy(&stream->key, obs_service_get_key(service));
	dstr_copy(&stream->username, obs_service_get_username(service));
	dstr_copy(&stream->password, obs_service_get_password(service));
	dstr_depad(&stream->path);
	dstr_depad(&stream->key);
	drop_b = (int64_t)obs_data_get_int(settings, OPT_DROP_THRESHOLD);
	drop_p = (int64_t)obs_data_get_int(settings, OPT_PFRAME_DROP_THRESHOLD);
	stream->max_shutdown_time_sec =
		(int)obs_data_get_int(settings, OPT_MAX_SHUTDOWN_TIME_SEC);

	if (drop_p < (drop_b + 200))
		drop_p = drop_b + 200;

	stream->drop_threshold_usec = 1000 * drop_b;
	stream->pframe_drop_threshold_usec = 1000 * drop_p;

	obs_data_release(settings);

	return true;
}

static void *connect_thread(void *data)
{
	struct ffmpeg_encoded_output *stream = data;
	int ret;

	os_set_thread_name("ffmpeg-stream: connect_thread");

	if (!init_connect(stream)) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_BAD_PATH);
		return NULL;
	}

	ret = proto_try_connect(stream);
	if (ret == OBS_OUTPUT_SUCCESS)
		ret = init_send(stream);

	if (ret != OBS_OUTPUT_SUCCESS) {
		obs_output_signal_stop(stream->output, ret);
		info("Connection to %s failed: %d", stream->path.array, ret);
	}

	if (!stopping(stream))
		pthread_detach(stream->connect_thread);

	os_atomic_set_bool(&stream->connecting, false);

	return NULL;
}

static inline size_t num_buffered_packets(struct ffmpeg_encoded_output *stream)
{
	return stream->packets.size / sizeof(struct encoder_packet);
}

static void drop_frames(struct ffmpeg_encoded_output *stream, const char *name,
			int highest_priority, bool pframes)
{
	UNUSED_PARAMETER(pframes);
	struct circlebuf new_buf = {0};
	int num_frames_dropped = 0;

#ifdef _DEBUG
	int start_packets = (int)num_buffered_packets(stream);
#else
	UNUSED_PARAMETER(name);
#endif

	circlebuf_reserve(&new_buf, sizeof(struct encoder_packet) * 8);

	while (stream->packets.size) {
		struct encoder_packet packet;
		circlebuf_pop_front(&stream->packets, &packet, sizeof(packet));

		/* do not drop audio data or video keyframes */
		if (packet.type == OBS_ENCODER_AUDIO ||
		    packet.drop_priority >= highest_priority) {
			circlebuf_push_back(&new_buf, &packet, sizeof(packet));

		} else {
			num_frames_dropped++;
			obs_encoder_packet_release(&packet);
		}
	}

	circlebuf_free(&stream->packets);
	stream->packets = new_buf;

	if (stream->min_priority < highest_priority)
		stream->min_priority = highest_priority;
	if (!num_frames_dropped)
		return;

	stream->dropped_frames += num_frames_dropped;
#ifdef _DEBUG
	debug("Dropped %s, prev packet count: %d, new packet count: %d", name,
	      start_packets, (int)num_buffered_packets(stream));
#endif
}

static bool find_first_video_packet(struct ffmpeg_encoded_output *stream,
				    struct encoder_packet *first)
{
	size_t count = stream->packets.size / sizeof(*first);

	for (size_t i = 0; i < count; i++) {
		struct encoder_packet *cur =
			circlebuf_data(&stream->packets, i * sizeof(*first));
		if (cur->type == OBS_ENCODER_VIDEO && !cur->keyframe) {
			*first = *cur;
			return true;
		}
	}

	return false;
}

static void check_to_drop_frames(struct ffmpeg_encoded_output *stream,
				 bool pframes)
{
	struct encoder_packet first;
	int64_t buffer_duration_usec;
	size_t num_packets = num_buffered_packets(stream);
	const char *name = pframes ? "p-frames" : "b-frames";
	int priority = pframes ? OBS_NAL_PRIORITY_HIGHEST
			       : OBS_NAL_PRIORITY_HIGH;
	int64_t drop_threshold = pframes ? stream->pframe_drop_threshold_usec
					 : stream->drop_threshold_usec;

	if (num_packets < 5) {
		if (!pframes)
			stream->congestion = 0.0f;
		return;
	}

	if (!find_first_video_packet(stream, &first))
		return;

	/* if the amount of time stored in the buffered packets waiting to be
	 * sent is higher than threshold, drop frames */
	buffer_duration_usec = stream->last_dts_usec - first.dts_usec;

	if (!pframes) {
		stream->congestion =
			(float)buffer_duration_usec / (float)drop_threshold;
	}

	if (buffer_duration_usec > drop_threshold) {
		debug("buffer_duration_usec: %" PRId64, buffer_duration_usec);
		drop_frames(stream, name, priority, pframes);
	}
}

static int init_send(struct ffmpeg_encoded_output *stream)
{
	int ret;
	reset_semaphore(stream);

	ret = pthread_create(&stream->send_thread, NULL, send_thread, stream);
	if (ret != 0) {
		proto_close(stream);
		warn("Failed to create send thread");
		return OBS_OUTPUT_ERROR;
	}

	os_atomic_set_bool(&stream->active, true);
	obs_output_begin_data_capture(stream->output, 0);

	return OBS_OUTPUT_SUCCESS;
}

static inline bool can_shutdown_stream(struct ffmpeg_encoded_output *stream,
				       struct encoder_packet *packet)
{
	uint64_t cur_time = os_gettime_ns();
	bool timeout = cur_time >= stream->shutdown_timeout_ts;

	if (timeout)
		info("Stream shutdown timeout reached (%d second(s))",
		     stream->max_shutdown_time_sec);

	return timeout || packet->sys_dts_usec >= (int64_t)stream->stop_ts;
}

static int proto_init(struct ffmpeg_encoded_output *stream)
{
	AVOutputFormat *outfmt = NULL;
	int ret = 0;

	//1. set up output format
	outfmt = av_guess_format("mpegts", NULL, "video/MP2T");
	if (outfmt == NULL) {
		ret = -1;
	} else {
		stream->ff_data.output = avformat_alloc_context();
		if (stream->ff_data.output)
			stream->ff_data.output->oformat = outfmt;
		else
			ret = -1;
	}

	return ret;
}

static int get_audio_mix_count(int audio_mix_mask)
{
	int mix_count = 0;

	for (int i = 0; i < MAX_AUDIO_MIXES; i++) {
		if ((audio_mix_mask & (1 << i)) != 0)
			mix_count++;
	}

	return mix_count;
}

static inline int encoder_bitrate(obs_encoder_t *encoder)
{
	obs_data_t *settings = obs_encoder_get_settings(encoder);
	const int bitrate = (int)obs_data_get_int(settings, "bitrate");

	obs_data_release(settings);

	return bitrate;
}

static int proto_try_connect(struct ffmpeg_encoded_output *stream)
{
	int ret = 0;
	const char *url = stream->path.array;
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	obs_encoder_t *aencoder =
		obs_output_get_audio_encoder(stream->output, 0);
	video_t *video = obs_encoder_video(vencoder);
	audio_t *audio = obs_encoder_audio(aencoder);
	const struct video_output_info *voi = video_output_get_info(video);
	struct ffmpeg_cfg config;
	bool success;

	config.url = url;
	config.format_name = "mpegts";
	config.format_mime_type = "video/MP2T";
	config.muxer_settings = "";
	config.video_bitrate = encoder_bitrate(vencoder);
	config.audio_bitrate = encoder_bitrate(aencoder);
	config.gop_size = 250;
	config.video_encoder = "";
	config.video_encoder_id = AV_CODEC_ID_H264;
	config.audio_encoder = "";
	config.audio_encoder_id = AV_CODEC_ID_AAC;
	config.video_settings = "";
	config.audio_settings = "";
	config.scale_width = 0;
	config.scale_height = 0;
	config.width = obs_encoder_get_width(vencoder);
	config.height = obs_encoder_get_height(vencoder);
	config.audio_tracks = (int)obs_output_get_mixer(stream->output);
	config.audio_mix_count = 1;
	config.format =
		obs_to_ffmpeg_video_format(video_output_get_format(video));

	if (format_is_yuv(voi->format)) {
		config.color_range = voi->range == VIDEO_RANGE_FULL
					     ? AVCOL_RANGE_JPEG
					     : AVCOL_RANGE_MPEG;
		config.color_space = voi->colorspace == VIDEO_CS_709
					     ? AVCOL_SPC_BT709
					     : AVCOL_SPC_BT470BG;
	} else {
		config.color_range = AVCOL_RANGE_UNSPECIFIED;
		config.color_space = AVCOL_SPC_RGB;
	}

	if (config.format == AV_PIX_FMT_NONE) {
		blog(LOG_DEBUG, "invalid pixel format used for FFmpeg output");
		return false;
	}

	if (!config.scale_width)
		config.scale_width = config.width;
	if (!config.scale_height)
		config.scale_height = config.height;

	success = ffmpeg_data_init(&stream->ff_data, &config);
	if (!success)
		return -1;

	return ret;
}

static int proto_connect_time(struct ffmpeg_encoded_output *stream)
{
	UNUSED_PARAMETER(stream);

	return 0;
}

static void proto_close(struct ffmpeg_encoded_output *stream)
{
	ffmpeg_data_free(&stream->ff_data);
}

static inline int64_t _rescale_ts(struct ffmpeg_encoded_output *stream,
				  int64_t val, int idx)
{
	AVStream *avstream = stream->ff_data.output->streams[idx];

	return av_rescale_q_rnd(val / avstream->codec->time_base.num,
				avstream->codec->time_base, avstream->time_base,
				AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
}

static int proto_send_packet(struct ffmpeg_encoded_output *stream,
			     struct encoder_packet *obs_packet, bool is_header)
{
	AVPacket av_packet = {0};
	int ret = 0;
	int streamIdx = (obs_packet->type == OBS_ENCODER_VIDEO) ? 0 : 1;

	//2. send
	av_init_packet(&av_packet);

	av_packet.data = obs_packet->data;
	av_packet.size = (int)obs_packet->size;
	av_packet.stream_index = streamIdx;
	av_packet.pts = _rescale_ts(stream, obs_packet->pts, streamIdx);
	av_packet.dts = _rescale_ts(stream, obs_packet->dts, streamIdx);

	if (obs_packet->keyframe)
		av_packet.flags = AV_PKT_FLAG_KEY;

	ret = av_interleaved_write_frame(stream->ff_data.output, &av_packet);
	stream->total_bytes_sent += obs_packet->size;
	if (is_header)
		bfree(obs_packet->data);
	else
		obs_encoder_packet_release(obs_packet);

	return ret;
}

static void proto_set_output_error(struct ffmpeg_encoded_output *stream)
{
	UNUSED_PARAMETER(stream);
}

static void ffmpeg_encoded_output_destroy(void *data)
{
	struct ffmpeg_encoded_output *stream = data;

	if (stopping(stream) && !connecting(stream)) {
		pthread_join(stream->send_thread, NULL);

	} else if (connecting(stream) || active(stream)) {
		if (stream->connecting)
			pthread_join(stream->connect_thread, NULL);

		stream->stop_ts = 0;
		os_event_signal(stream->stop_event);

		if (active(stream)) {
			os_sem_post(stream->send_sem);
			obs_output_end_data_capture(stream->output);
			pthread_join(stream->send_thread, NULL);
		}
	}

	free_packets(stream);
	dstr_free(&stream->path);
	dstr_free(&stream->key);
	dstr_free(&stream->username);
	dstr_free(&stream->password);
	dstr_free(&stream->encoder_name);
	os_event_destroy(stream->stop_event);
	os_sem_destroy(stream->send_sem);
	pthread_mutex_destroy(&stream->packets_mutex);
	circlebuf_free(&stream->packets);
#ifdef TEST_FRAMEDROPS
	circlebuf_free(&stream->droptest_info);
#endif

	os_event_destroy(stream->buffer_space_available_event);
	os_event_destroy(stream->buffer_has_data_event);
	os_event_destroy(stream->send_thread_signaled_exit);
	pthread_mutex_destroy(&stream->write_buf_mutex);
	ffmpeg_data_free(&stream->ff_data);
	if (stream->write_buf)
		bfree(stream->write_buf);
	bfree(stream);
}

static void *ffmpeg_encoded_output_create(obs_data_t *settings,
					  obs_output_t *output)
{
	struct ffmpeg_encoded_output *stream =
		bzalloc(sizeof(struct ffmpeg_encoded_output));

	stream->output = output;
	pthread_mutex_init_value(&stream->packets_mutex);

	proto_init(stream);

	if (pthread_mutex_init(&stream->packets_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&stream->stop_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;

	if (pthread_mutex_init(&stream->write_buf_mutex, NULL) != 0) {
		warn("Failed to initialize write buffer mutex");
		goto fail;
	}

	if (os_event_init(&stream->buffer_space_available_event,
			  OS_EVENT_TYPE_AUTO) != 0) {
		warn("Failed to initialize write buffer event");
		goto fail;
	}
	if (os_event_init(&stream->buffer_has_data_event, OS_EVENT_TYPE_AUTO) !=
	    0) {
		warn("Failed to initialize data buffer event");
		goto fail;
	}
	if (os_event_init(&stream->send_thread_signaled_exit,
			  OS_EVENT_TYPE_MANUAL) != 0) {
		warn("Failed to initialize socket exit event");
		goto fail;
	}

	UNUSED_PARAMETER(settings);

	return stream;

fail:
	ffmpeg_encoded_output_destroy(stream);
	return NULL;
}

static bool ffmpeg_encoded_output_start(void *data)
{
	struct ffmpeg_encoded_output *stream = data;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;
	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	os_atomic_set_bool(&stream->connecting, true);

	return pthread_create(&stream->connect_thread, NULL, connect_thread,
			      stream) == 0;
}

#define MILLISECOND_DEN 1000

static int32_t get_ms_time(struct encoder_packet *packet, int64_t val)
{
	return (int32_t)(val * MILLISECOND_DEN / packet->timebase_den);
}

static void ffmpeg_encoded_output_data(void *data,
				       struct encoder_packet *packet)
{
	struct ffmpeg_encoded_output *stream = data;
	struct encoder_packet new_packet;
	bool added_packet = false;

	if (disconnected(stream) || !active(stream))
		return;

	if (packet->type == OBS_ENCODER_VIDEO) {
		if (!stream->got_first_video) {
			stream->start_dts_offset =
				get_ms_time(packet, packet->dts);
			stream->got_first_video = true;
		}
	}
	obs_encoder_packet_ref(&new_packet, packet);

	pthread_mutex_lock(&stream->packets_mutex);

	if (!disconnected(stream))
		added_packet = (packet->type == OBS_ENCODER_VIDEO)
				       ? add_video_packet(stream, &new_packet)
				       : add_packet(stream, &new_packet);

	pthread_mutex_unlock(&stream->packets_mutex);

	if (added_packet)
		os_sem_post(stream->send_sem);
	else
		obs_encoder_packet_release(&new_packet);
}

static void ffmpeg_encoded_output_stop(void *data, uint64_t ts)
{
	struct ffmpeg_encoded_output *stream = data;

	if (stopping(stream) && ts != 0)
		return;

	if (connecting(stream))
		pthread_join(stream->connect_thread, NULL);

	stream->stop_ts = ts / 1000ULL;

	if (ts)
		stream->shutdown_timeout_ts =
			ts +
			(uint64_t)stream->max_shutdown_time_sec * 1000000000ULL;

	if (active(stream)) {
		os_event_signal(stream->stop_event);
		if (stream->stop_ts == 0)
			os_sem_post(stream->send_sem);
	} else {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_SUCCESS);
	}
}

static void ffmpeg_encoded_output_defaults(obs_data_t *defaults)
{
	obs_data_set_default_int(defaults, OPT_DROP_THRESHOLD, 700);
	obs_data_set_default_int(defaults, OPT_PFRAME_DROP_THRESHOLD, 900);
	obs_data_set_default_int(defaults, OPT_MAX_SHUTDOWN_TIME_SEC, 30);
}

static obs_properties_t *ffmpeg_encoded_output_properties(void *unused)
{
	UNUSED_PARAMETER(unused);
	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(
		props, OPT_DROP_THRESHOLD,
		obs_module_text("FFmpegEncodedOutput.DropThreshold"), 200,
		10000, 100);

	return props;
}

static uint64_t ffmpeg_encoded_output_total_bytes_sent(void *data)
{
	struct ffmpeg_encoded_output *stream = data;

	return stream->total_bytes_sent;
}

static int ffmpeg_encoded_output_dropped_frames(void *data)
{
	struct ffmpeg_encoded_output *stream = data;

	return stream->dropped_frames;
}

static float ffmpeg_encoded_output_congestion(void *data)
{
	struct ffmpeg_encoded_output *stream = data;

	return stream->min_priority > 0 ? 1.0f : stream->congestion;
}

static int ffmpeg_encoded_output_connect_time(void *data)
{
	struct ffmpeg_encoded_output *stream = data;

	return proto_connect_time(stream);
}

struct obs_output_info ffmpeg_encoded_output_info = {
	.id = "ffmpeg_encoded_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_SERVICE |
		 OBS_OUTPUT_MULTI_TRACK,
	.encoded_video_codecs = "h264",
	.encoded_audio_codecs = "aac",
	.get_name = ffmpeg_encoded_output_getname,
	.create = ffmpeg_encoded_output_create,
	.destroy = ffmpeg_encoded_output_destroy,
	.start = ffmpeg_encoded_output_start,
	.stop = ffmpeg_encoded_output_stop,
	.encoded_packet = ffmpeg_encoded_output_data,
	.get_defaults = ffmpeg_encoded_output_defaults,
	.get_properties = ffmpeg_encoded_output_properties,
	.get_total_bytes = ffmpeg_encoded_output_total_bytes_sent,
	.get_congestion = ffmpeg_encoded_output_congestion,
	.get_connect_time_ms = ffmpeg_encoded_output_connect_time,
	.get_dropped_frames = ffmpeg_encoded_output_dropped_frames};
