#include "ramp-stream.h"
#include "obs-output-ver.h"

#ifndef SEC_TO_NSEC
#define SEC_TO_NSEC 1000000000ULL
#endif

#ifndef MSEC_TO_USEC
#define MSEC_TO_USEC 1000ULL
#endif

#ifndef MSEC_TO_NSEC
#define MSEC_TO_NSEC 1000000ULL
#endif

/* dynamic bitrate coefficients */
#define DBR_INC_TIMER (30ULL * SEC_TO_NSEC)
#define DBR_TRIGGER_USEC (200ULL * MSEC_TO_USEC)
#define MIN_ESTIMATE_DURATION_MS 1000
#define MAX_ESTIMATE_DURATION_MS 2000


static const char *ramp_stream_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("RAMPStream");
}

static void log_ramp(int level, const char *format, va_list args)
{
	if (level > RTMP_LOGINFO)
		return;

	blogva(LOG_INFO, format, args);
}

static inline size_t num_buffered_packets(struct ramp_stream *stream);

static inline void free_packets(struct ramp_stream *stream)
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

static inline bool stopping(struct ramp_stream *stream)
{
	return os_event_try(stream->stop_event) != EAGAIN;
}

static inline bool connecting(struct ramp_stream *stream)
{
	return os_atomic_load_bool(&stream->connecting);
}

static inline bool active(struct ramp_stream *stream)
{
	return os_atomic_load_bool(&stream->active);
}

static inline bool disconnected(struct ramp_stream *stream)
{
	return os_atomic_load_bool(&stream->disconnected);
}

static void ramp_stream_destroy(void *data)
{
	struct ramp_stream *stream = data;
	obs_output_end_data_capture(stream->output);

	free_packets(stream);
	os_event_destroy(stream->stop_event);
	os_sem_destroy(stream->send_sem);
	pthread_mutex_destroy(&stream->packets_mutex);
	circlebuf_free(&stream->packets);

#ifdef TEST_FRAMEDROPS
	circlebuf_free(&stream->droptest_info);
#endif

	circlebuf_free(&stream->dbr_frames);
	pthread_mutex_destroy(&stream->dbr_mutex);

	os_event_destroy(stream->buffer_space_available_event);
	os_event_destroy(stream->buffer_has_data_event);
	os_event_destroy(stream->socket_available_event);
	os_event_destroy(stream->send_thread_signaled_exit);
	pthread_mutex_destroy(&stream->write_buf_mutex);

	if (stream->write_buf)
		bfree(stream->write_buf);

	bfree(stream);
}

static void *ramp_stream_create(obs_data_t *settings, obs_output_t *output)
{
	struct ramp_stream *stream = bzalloc(sizeof(struct ramp_stream));
	stream->output = output;
	pthread_mutex_init_value(&stream->packets_mutex);

	if (pthread_mutex_init(&stream->packets_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&stream->stop_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;

	if (pthread_mutex_init(&stream->write_buf_mutex, NULL) != 0) {
		warn("Failed to initialize write buffer mutex");
		goto fail;
	}

	if (pthread_mutex_init(&stream->dbr_mutex, NULL) != 0) {
		warn("Failed to initialize dbr mutex");
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
	if (os_event_init(&stream->socket_available_event,
			  OS_EVENT_TYPE_AUTO) != 0) {
		warn("Failed to initialize socket buffer event");
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
	ramp_stream_destroy(stream);
	return NULL;
}

static void ramp_stream_stop(void *data, uint64_t ts)
{
	struct ramp_stream *stream = data;

	stream->stop_ts = ts / 1000ULL;

	if (ts)
		stream->shutdown_timeout_ts =
			ts +
			(uint64_t)stream->max_shutdown_time_sec * 1000000000ULL;

	RAMP_Disconnect(stream->sdk);

	if (connecting(stream))
		pthread_join(stream->connect_thread, NULL);

	if (active(stream)) {
		os_event_signal(stream->stop_event);
		os_sem_post(stream->send_sem);
		pthread_join(stream->send_thread, NULL);
	} else
		obs_output_signal_stop(stream->output, OBS_OUTPUT_SUCCESS);

	RAMP_Stop(&stream->sdk);
}

static inline void set_ramp_str(AVal *val, const char *str)
{
	bool valid = (str && *str);
	val->av_val = valid ? (char *)str : NULL;
	val->av_len = valid ? (int)strlen(str) : 0;
}

static inline void set_ramp_dstr(AVal *val, struct dstr *str)
{
	bool valid = !dstr_is_empty(str);
	val->av_val = valid ? str->array : NULL;
	val->av_len = valid ? (int)str->len : 0;
}

static inline bool get_next_packet(struct ramp_stream *stream,
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

static int send_packet(struct ramp_stream *stream,
		       struct encoder_packet *packet, bool is_header,
		       size_t idx)
{
	if (!stream->sdk)
		return -1;

	size_t size;
	int ret = 0;
	char buf[8192];

	while (true) {
		ret = RAMP_Receive(stream->sdk, buf, sizeof(buf));
		if (ret < 0)
			return -1;
		if (!ret)
			break;
	}

	if (packet->type == OBS_ENCODER_VIDEO) {
		int32_t timestamp = get_ms_time(packet, packet->dts) -
				    (is_header ? 0 : stream->start_dts_offset);
		int32_t pts_offset =
			get_ms_time(packet, packet->pts - packet->dts);
		ret = RAMP_SendVideo(stream->sdk, idx, is_header,
				     packet->keyframe, timestamp, pts_offset,
				     packet->data, packet->size);
		size = ret >= 0 ? ret : 0;
	} else {
		int32_t timestamp = get_ms_time(packet, packet->dts) -
				    (is_header ? 0 : stream->start_dts_offset);
		ret = RAMP_SendAudio(stream->sdk, idx, is_header, timestamp,
				     packet->data, packet->size);
		size = ret >= 0 ? ret : 0;
	}

	if (is_header)
		bfree(packet->data);
	else
		obs_encoder_packet_release(packet);

	stream->total_bytes_sent += size;
	return ret;
}

static inline bool send_headers(struct ramp_stream *stream);

static inline bool can_shutdown_stream(struct ramp_stream *stream,
				       struct encoder_packet *packet)
{
	uint64_t cur_time = os_gettime_ns();
	bool timeout = cur_time >= stream->shutdown_timeout_ts;

	if (timeout)
		info("Stream shutdown timeout reached (%d second(s))",
		     stream->max_shutdown_time_sec);

	return timeout || packet->sys_dts_usec >= (int64_t)stream->stop_ts;
}

static void set_output_error(struct ramp_stream *stream)
{
	const char *msg = NULL;
	if (stream->sdk && stream->sdk->rtmp) {
#ifdef _WIN32
		switch (stream->sdk->rtmp->last_error_code) {
		case WSAETIMEDOUT:
			msg = obs_module_text("ConnectionTimedOut");
			break;
		case WSAEACCES:
			msg = obs_module_text("PermissionDenied");
			break;
		case WSAECONNABORTED:
			msg = obs_module_text("ConnectionAborted");
			break;
		case WSAECONNRESET:
			msg = obs_module_text("ConnectionReset");
			break;
		case WSAHOST_NOT_FOUND:
			msg = obs_module_text("HostNotFound");
			break;
		case WSANO_DATA:
			msg = obs_module_text("NoData");
			break;
		case WSAEADDRNOTAVAIL:
			msg = obs_module_text("AddressNotAvailable");
			break;
		}
#else
		switch (stream->sdk->rtmp->last_error_code) {
		case ETIMEDOUT:
			msg = obs_module_text("ConnectionTimedOut");
			break;
		case EACCES:
			msg = obs_module_text("PermissionDenied");
			break;
		case ECONNABORTED:
			msg = obs_module_text("ConnectionAborted");
			break;
		case ECONNRESET:
			msg = obs_module_text("ConnectionReset");
			break;
		case HOST_NOT_FOUND:
			msg = obs_module_text("HostNotFound");
			break;
		case NO_DATA:
			msg = obs_module_text("NoData");
			break;
		case EADDRNOTAVAIL:
			msg = obs_module_text("AddressNotAvailable");
			break;
		}
#endif
	}

	obs_output_set_last_error(stream->output, msg);
}

static void dbr_add_frame(struct ramp_stream *stream, struct dbr_frame *back)
{
	struct dbr_frame front;
	uint64_t dur;

	circlebuf_push_back(&stream->dbr_frames, back, sizeof(*back));
	circlebuf_peek_front(&stream->dbr_frames, &front, sizeof(front));

	stream->dbr_data_size += back->size;

	dur = (back->send_end - front.send_beg) / 1000000;

	if (dur >= MAX_ESTIMATE_DURATION_MS) {
		stream->dbr_data_size -= front.size;
		circlebuf_pop_front(&stream->dbr_frames, NULL, sizeof(front));
	}

	stream->dbr_est_bitrate =
		(dur >= MIN_ESTIMATE_DURATION_MS)
			? (long)(stream->dbr_data_size * 1000 / dur)
			: 0;
	stream->dbr_est_bitrate *= 8;
	stream->dbr_est_bitrate /= 1000;

	if (stream->dbr_est_bitrate) {
		stream->dbr_est_bitrate -= stream->audio_bitrate;
		if (stream->dbr_est_bitrate < 50)
			stream->dbr_est_bitrate = 50;
	}
}

static void dbr_set_bitrate(struct ramp_stream *stream);

static void *send_thread(void *data)
{
	struct ramp_stream *stream = data;

	os_set_thread_name("rtmp-tun-stream: send_thread");

	while (os_sem_wait(stream->send_sem) == 0) {
		struct encoder_packet packet;
		struct dbr_frame dbr_frame;

		if (stopping(stream) && stream->stop_ts == 0) {
			break;
		}

		if (!get_next_packet(stream, &packet))
			continue;

		if (stopping(stream)) {
			if (can_shutdown_stream(stream, &packet)) {
				obs_encoder_packet_release(&packet);
				break;
			}
		}

		if (!stream->sent_headers) {
			if (!send_headers(stream)) {
				os_atomic_set_bool(&stream->disconnected, true);
				break;
			}
		}

		if (stream->dbr_enabled) {
			dbr_frame.send_beg = os_gettime_ns();
			dbr_frame.size = packet.size;
		}

		if (send_packet(stream, &packet, false, packet.track_idx) < 0) {
			os_atomic_set_bool(&stream->disconnected, true);
			break;
		}

		if (stream->dbr_enabled) {
			dbr_frame.send_end = os_gettime_ns();

			pthread_mutex_lock(&stream->dbr_mutex);
			dbr_add_frame(stream, &dbr_frame);
			pthread_mutex_unlock(&stream->dbr_mutex);
		}
	}

	bool encode_error = os_atomic_load_bool(&stream->encode_error);

	if (disconnected(stream)) {
		info("Disconnected");
	} else if (encode_error) {
		info("Encoder error, disconnecting");
	} else {
		info("User stopped the stream");
	}

	set_output_error(stream);
	//RAMP_Close(stream->sdk->rtmp);

	if (!stopping(stream)) {
		pthread_detach(stream->send_thread);
		obs_output_signal_stop(stream->output, OBS_OUTPUT_DISCONNECTED);
	} else if (encode_error) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_ENCODE_ERROR);
	} else {
		obs_output_end_data_capture(stream->output);
	}

	free_packets(stream);
	os_event_reset(stream->stop_event);
	os_atomic_set_bool(&stream->active, false);
	stream->sent_headers = false;

	/* reset bitrate on stop */
	if (stream->dbr_enabled) {
		if (stream->dbr_cur_bitrate != stream->dbr_orig_bitrate) {
			stream->dbr_cur_bitrate = stream->dbr_orig_bitrate;
			dbr_set_bitrate(stream);
		}
	}

	return NULL;
}

static inline double encoder_bitrate(obs_encoder_t *encoder)
{
	obs_data_t *settings = obs_encoder_get_settings(encoder);
	double bitrate = obs_data_get_double(settings, "bitrate");

	obs_data_release(settings);
	return bitrate;
}

static bool send_meta_data(struct ramp_stream *stream, size_t idx, bool *next)
{
	bool success = true;

	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	obs_encoder_t *aencoder =
		obs_output_get_audio_encoder(stream->output, idx);
	video_t *video = obs_encoder_video(vencoder);
	audio_t *audio = obs_encoder_audio(aencoder);

	if (idx > 0 && !aencoder) {
		*next = false;
		return true;
	}

	struct dstr encoder_name = {0};
	dstr_printf(&encoder_name, "%s (libobs version ", MODULE_NAME);

#ifdef HAVE_OBSCONFIG_H
	dstr_cat(&encoder_name, OBS_VERSION);
#else
	dstr_catf(&encoder_name, "%d.%d.%d", LIBOBS_API_MAJOR_VER,
		  LIBOBS_API_MINOR_VER, LIBOBS_API_PATCH_VER);
#endif

	dstr_cat(&encoder_name, ")");

	RAMP_SendMeta(stream->sdk, idx, "avc1", obs_encoder_get_width(vencoder),
		      obs_encoder_get_height(vencoder),
		      encoder_bitrate(vencoder),
		      video_output_get_frame_rate(video), "mp4a",
		      encoder_bitrate(aencoder),
		      obs_encoder_get_sample_rate(aencoder), 16,
		      audio_output_get_channels(audio), encoder_name.array);

	dstr_free(&encoder_name);

	*next = true;
	return success;
}

static bool send_audio_header(struct ramp_stream *stream, size_t idx,
			      bool *next)
{
	obs_output_t *context = stream->output;
	obs_encoder_t *aencoder = obs_output_get_audio_encoder(context, idx);
	uint8_t *header;

	struct encoder_packet packet = {.type = OBS_ENCODER_AUDIO,
					.timebase_den = 1};

	if (!aencoder) {
		*next = false;
		return true;
	}

	obs_encoder_get_extra_data(aencoder, &header, &packet.size);
	packet.data = bmemdup(header, packet.size);
	return send_packet(stream, &packet, true, idx) >= 0;
}

static bool send_video_header(struct ramp_stream *stream)
{
	obs_output_t *context = stream->output;
	obs_encoder_t *vencoder = obs_output_get_video_encoder(context);
	uint8_t *header;
	size_t size;

	struct encoder_packet packet = {
		.type = OBS_ENCODER_VIDEO, .timebase_den = 1, .keyframe = true};

	obs_encoder_get_extra_data(vencoder, &header, &size);
	packet.size = obs_parse_avc_header(&packet.data, header, size);
	return send_packet(stream, &packet, true, 0) >= 0;
}

static inline bool send_headers(struct ramp_stream *stream)
{
	stream->sent_headers = true;
	size_t i = 0;
	bool next = true;

	if (!send_audio_header(stream, i++, &next))
		return false;
	if (!send_video_header(stream))
		return false;

	while (next) {
		if (!send_audio_header(stream, i++, &next))
			return false;
	}

	return true;
}

static inline bool reset_semaphore(struct ramp_stream *stream)
{
	os_sem_destroy(stream->send_sem);
	return os_sem_init(&stream->send_sem, 0) == 0;
}

static int init_send(struct ramp_stream *stream)
{
	int ret;
	size_t idx = 0;
	bool next = true;

	reset_semaphore(stream);
	ret = pthread_create(&stream->send_thread, NULL, send_thread, stream);
	if (ret != 0) {
		//RAMP_Close(stream->sdk->rtmp);
		warn("Failed to create send thread");
		return OBS_OUTPUT_ERROR;
	}

	os_atomic_set_bool(&stream->active, true);
	while (next) {
		if (!send_meta_data(stream, idx++, &next)) {
			warn("Disconnected while attempting to connect to "
			     "server.");
			set_output_error(stream);
			return OBS_OUTPUT_DISCONNECTED;
		}
	}
	obs_output_begin_data_capture(stream->output, 0);

	return OBS_OUTPUT_SUCCESS;
}

static bool init_connect(struct ramp_stream *stream)
{
	obs_service_t *service;
	obs_data_t *settings;
	//const char *bind_ip;
	int64_t drop_p;
	int64_t drop_b;
	uint32_t caps;

	if (stopping(stream)) {
		pthread_join(stream->send_thread, NULL);
	}

	free_packets(stream);

	service = obs_output_get_service(stream->output);
	if (!service)
		return false;

	os_atomic_set_bool(&stream->disconnected, false);
	os_atomic_set_bool(&stream->encode_error, false);
	stream->total_bytes_sent = 0;
	stream->dropped_frames = 0;
	stream->min_priority = 0;
	stream->got_first_video = false;

	settings = obs_output_get_settings(stream->output);

	drop_b = (int64_t)obs_data_get_int(settings, OPT_DROP_THRESHOLD);
	drop_p = (int64_t)obs_data_get_int(settings, OPT_PFRAME_DROP_THRESHOLD);
	stream->max_shutdown_time_sec =
		(int)obs_data_get_int(settings, OPT_MAX_SHUTDOWN_TIME_SEC);

	obs_encoder_t *venc = obs_output_get_video_encoder(stream->output);
	obs_encoder_t *aenc = obs_output_get_audio_encoder(stream->output, 0);
	obs_data_t *vsettings = obs_encoder_get_settings(venc);
	obs_data_t *asettings = obs_encoder_get_settings(aenc);

	circlebuf_free(&stream->dbr_frames);
	stream->audio_bitrate = (long)obs_data_get_int(asettings, "bitrate");
	stream->dbr_data_size = 0;
	stream->dbr_orig_bitrate = (long)obs_data_get_int(vsettings, "bitrate");
	stream->dbr_cur_bitrate = stream->dbr_orig_bitrate;
	stream->dbr_est_bitrate = 0;
	stream->dbr_inc_bitrate = stream->dbr_orig_bitrate / 10;
	stream->dbr_inc_timeout = 0;
	stream->dbr_enabled = obs_data_get_bool(settings, OPT_DYN_BITRATE);

	caps = obs_encoder_get_caps(venc);
	if ((caps & OBS_ENCODER_CAP_DYN_BITRATE) == 0) {
		stream->dbr_enabled = false;
	}

	if (obs_output_get_delay(stream->output) != 0) {
		stream->dbr_enabled = false;
	}

	if (stream->dbr_enabled) {
		info("Dynamic bitrate enabled.  Dropped frames begone!");
	}

	obs_data_release(vsettings);
	obs_data_release(asettings);

	if (drop_p < (drop_b + 200))
		drop_p = drop_b + 200;

	stream->drop_threshold_usec = 1000 * drop_b;
	stream->pframe_drop_threshold_usec = 1000 * drop_p;

	//bind_ip = obs_data_get_string(settings, OPT_BIND_IP);
	//dstr_copy(&stream->bind_ip, bind_ip);

	stream->low_latency_mode =
		obs_data_get_bool(settings, OPT_LOWLATENCY_ENABLED);

	obs_data_release(settings);
	return true;
}

static void *connect_thread(void *data)
{
	struct ramp_stream *stream = data;
	int ret;

	os_set_thread_name("ramp-stream: connect_thread");

	if (!init_connect(stream)) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_ERROR);
		return NULL;
	}

	obs_service_t *service = obs_output_get_service(stream->output);
	if (!service) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_ERROR);
		return NULL;
	}

	const char *url = obs_service_get_url(service);
	const char *key = obs_service_get_key(service);
	const char *username = obs_service_get_username(service);
	const char *password = obs_service_get_password(service);

	if (!url || !*url) {
		warn("URL is empty");
		obs_output_signal_stop(stream->output, OBS_OUTPUT_BAD_PATH);
		return NULL;
	}

	info("Connecting to RAMP URL %s...", url);

	stream->sdk = RAMP_Start(url, key, username, password, log_ramp,
				 RTMP_LOGINFO);
	if (!stream->sdk) {
		obs_output_signal_stop(stream->output, -1);
		info("Connection failed, can't init sdk");
		obs_output_signal_stop(stream->output, OBS_OUTPUT_ERROR);
	} else {
		ret = init_send(stream);
		if (ret != OBS_OUTPUT_SUCCESS) {
			obs_output_signal_stop(stream->output, ret);
			info("Connection to %s failed: %d", stream->sdk->path,
			     ret);
		}
	}

	os_atomic_set_bool(&stream->connecting, false);
	return NULL;
}

static bool ramp_stream_start(void *data)
{
	struct ramp_stream *stream = data;

	RAMP_Stop(&stream->sdk);

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;

	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	reset_semaphore(stream);
	os_atomic_set_bool(&stream->connecting, true);

	return pthread_create(&stream->connect_thread, NULL, connect_thread,
			      stream) == 0;
}

static inline bool add_packet(struct ramp_stream *stream,
			      struct encoder_packet *packet)
{
	circlebuf_push_back(&stream->packets, packet,
			    sizeof(struct encoder_packet));
	return true;
}

static inline size_t num_buffered_packets(struct ramp_stream *stream)
{
	return stream->packets.size / sizeof(struct encoder_packet);
}

static void drop_frames(struct ramp_stream *stream, const char *name,
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

static bool find_first_video_packet(struct ramp_stream *stream,
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

static bool dbr_bitrate_lowered(struct ramp_stream *stream)
{
	long prev_bitrate = stream->dbr_prev_bitrate;
	long est_bitrate = 0;
	long new_bitrate;

	if (stream->dbr_est_bitrate &&
	    stream->dbr_est_bitrate < stream->dbr_cur_bitrate) {
		stream->dbr_data_size = 0;
		circlebuf_pop_front(&stream->dbr_frames, NULL,
				    stream->dbr_frames.size);
		est_bitrate = stream->dbr_est_bitrate / 100 * 100;
		if (est_bitrate < 50) {
			est_bitrate = 50;
		}
	}

	if (est_bitrate) {
		new_bitrate = est_bitrate;

	} else if (prev_bitrate) {
		new_bitrate = prev_bitrate;
		info("going back to prev bitrate");

	} else {
		return false;
	}

	if (new_bitrate == stream->dbr_cur_bitrate) {
		return false;
	}

	stream->dbr_prev_bitrate = 0;
	stream->dbr_cur_bitrate = new_bitrate;
	stream->dbr_inc_timeout = os_gettime_ns() + DBR_INC_TIMER;
	info("bitrate decreased to: %ld", stream->dbr_cur_bitrate);
	return true;
}

static void dbr_set_bitrate(struct ramp_stream *stream)
{
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	obs_data_t *settings = obs_encoder_get_settings(vencoder);

	obs_data_set_int(settings, "bitrate", stream->dbr_cur_bitrate);
	obs_encoder_update(vencoder, settings);

	obs_data_release(settings);
}

static void dbr_inc_bitrate(struct ramp_stream *stream)
{
	stream->dbr_prev_bitrate = stream->dbr_cur_bitrate;
	stream->dbr_cur_bitrate += stream->dbr_inc_bitrate;

	if (stream->dbr_cur_bitrate >= stream->dbr_orig_bitrate) {
		stream->dbr_cur_bitrate = stream->dbr_orig_bitrate;
		info("bitrate increased to: %ld, done",
		     stream->dbr_cur_bitrate);
	} else if (stream->dbr_cur_bitrate < stream->dbr_orig_bitrate) {
		stream->dbr_inc_timeout = os_gettime_ns() + DBR_INC_TIMER;
		info("bitrate increased to: %ld, waiting",
		     stream->dbr_cur_bitrate);
	}
}

static void check_to_drop_frames(struct ramp_stream *stream, bool pframes)
{
	struct encoder_packet first;
	int64_t buffer_duration_usec;
	size_t num_packets = num_buffered_packets(stream);
	const char *name = pframes ? "p-frames" : "b-frames";
	int priority = pframes ? OBS_NAL_PRIORITY_HIGHEST
			       : OBS_NAL_PRIORITY_HIGH;
	int64_t drop_threshold = pframes ? stream->pframe_drop_threshold_usec
					 : stream->drop_threshold_usec;

	if (!pframes && stream->dbr_enabled) {
		if (stream->dbr_inc_timeout) {
			uint64_t t = os_gettime_ns();

			if (t >= stream->dbr_inc_timeout) {
				stream->dbr_inc_timeout = 0;
				dbr_inc_bitrate(stream);
				dbr_set_bitrate(stream);
			}
		}
	}

	if (num_packets < 5) {
		if (!pframes)
			stream->congestion = 0.0f;
		return;
	}

	if (!find_first_video_packet(stream, &first))
		return;

	buffer_duration_usec = stream->last_dts_usec - first.dts_usec;
	if (!pframes) {
		stream->congestion =
			(float)buffer_duration_usec / (float)drop_threshold;
	}

	if (stream->dbr_enabled) {
		bool bitrate_changed = false;

		if (pframes) {
			return;
		}

		if (buffer_duration_usec >= DBR_TRIGGER_USEC) {
			pthread_mutex_lock(&stream->dbr_mutex);
			bitrate_changed = dbr_bitrate_lowered(stream);
			pthread_mutex_unlock(&stream->dbr_mutex);
		}

		if (bitrate_changed) {
			debug("buffer_duration_msec: %" PRId64,
			      buffer_duration_usec / 1000);
			dbr_set_bitrate(stream);
		}
		return;
	}

	if (buffer_duration_usec > drop_threshold) {
		debug("buffer_duration_usec: %" PRId64, buffer_duration_usec);
		drop_frames(stream, name, priority, pframes);
	}
}

static bool add_video_packet(struct ramp_stream *stream,
			     struct encoder_packet *packet)
{
	check_to_drop_frames(stream, false);
	check_to_drop_frames(stream, true);

	if (packet->drop_priority < stream->min_priority) {
		stream->dropped_frames++;
		return false;
	} else {
		stream->min_priority = 0;
	}

	stream->last_dts_usec = packet->dts_usec;
	return add_packet(stream, packet);
}

static void ramp_stream_data(void *data, struct encoder_packet *packet)
{
	struct ramp_stream *stream = data;
	struct encoder_packet new_packet;
	bool added_packet = false;

	if (disconnected(stream) || !active(stream))
		return;

	/* encoder fail */
	if (!packet) {
		os_atomic_set_bool(&stream->encode_error, true);
		os_sem_post(stream->send_sem);
		return;
	}

	if (packet->type == OBS_ENCODER_VIDEO) {
		if (!stream->got_first_video) {
			stream->start_dts_offset =
				get_ms_time(packet, packet->dts);
			stream->got_first_video = true;
		}

		obs_parse_avc_packet(&new_packet, packet);
	} else {
		obs_encoder_packet_ref(&new_packet, packet);
	}

	pthread_mutex_lock(&stream->packets_mutex);

	if (!disconnected(stream)) {
		added_packet = (packet->type == OBS_ENCODER_VIDEO)
				       ? add_video_packet(stream, &new_packet)
				       : add_packet(stream, &new_packet);
	}

	pthread_mutex_unlock(&stream->packets_mutex);

	if (added_packet)
		os_sem_post(stream->send_sem);
	else
		obs_encoder_packet_release(&new_packet);
}

static void ramp_stream_defaults(obs_data_t *defaults)
{
	obs_data_set_default_int(defaults, OPT_DROP_THRESHOLD, 700);
	obs_data_set_default_int(defaults, OPT_PFRAME_DROP_THRESHOLD, 900);
	obs_data_set_default_int(defaults, OPT_MAX_SHUTDOWN_TIME_SEC, 30);
	obs_data_set_default_string(defaults, OPT_BIND_IP, "default");
	obs_data_set_default_bool(defaults, OPT_NEWSOCKETLOOP_ENABLED, false);
	obs_data_set_default_bool(defaults, OPT_LOWLATENCY_ENABLED, false);
}

static obs_properties_t *ramp_stream_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();
	struct netif_saddr_data addrs = {0};
	obs_property_t *p;

	obs_properties_add_int(props, OPT_DROP_THRESHOLD,
			       obs_module_text("RTMPStream.DropThreshold"), 200,
			       10000, 100);

	p = obs_properties_add_list(props, OPT_BIND_IP,
				    obs_module_text("RTMPStream.BindIP"),
				    OBS_COMBO_TYPE_LIST,
				    OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(p, obs_module_text("Default"), "default");

	netif_get_addrs(&addrs);
	for (size_t i = 0; i < addrs.addrs.num; i++) {
		struct netif_saddr_item item = addrs.addrs.array[i];
		obs_property_list_add_string(p, item.name, item.addr);
	}
	netif_saddr_data_free(&addrs);

	obs_properties_add_bool(props, OPT_NEWSOCKETLOOP_ENABLED,
				obs_module_text("RTMPStream.NewSocketLoop"));
	obs_properties_add_bool(props, OPT_LOWLATENCY_ENABLED,
				obs_module_text("RTMPStream.LowLatencyMode"));

	return props;
}

static uint64_t ramp_stream_total_bytes_sent(void *data)
{
	struct ramp_stream *stream = data;
	return stream->total_bytes_sent;
}

static int ramp_stream_dropped_frames(void *data)
{
	struct ramp_stream *stream = data;
	return stream->dropped_frames;
}

static float ramp_stream_congestion(void *data)
{
	struct ramp_stream *stream = data;
	return stream->min_priority > 0 ? 1.0f : stream->congestion;
}

static int ramp_stream_connect_time(void *data)
{
	struct ramp_stream *stream = data;
	return stream->sdk->rtmp->connect_time_ms;
}

struct obs_output_info ramp_output_info = {
	.id = "ramp_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_SERVICE,
//		 | OBS_OUTPUT_MULTI_TRACK,
	.encoded_video_codecs = "h264",
	.encoded_audio_codecs = "aac",
	.get_name = ramp_stream_getname,
	.create = ramp_stream_create,
	.destroy = ramp_stream_destroy,
	.start = ramp_stream_start,
	.stop = ramp_stream_stop,
	.encoded_packet = ramp_stream_data,
	.get_defaults = ramp_stream_defaults,
	.get_properties = ramp_stream_properties,
	.get_total_bytes = ramp_stream_total_bytes_sent,
	.get_congestion = ramp_stream_congestion,
	.get_connect_time_ms = ramp_stream_connect_time,
	.get_dropped_frames = ramp_stream_dropped_frames,
};
