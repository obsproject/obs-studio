/******************************************************************************
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

#include <obs.h>
#include <obs-avc.h>
#include <util/circlebuf.h>
#include <util/dstr.h>
#include <util/threading.h>
#include "librtmp/rtmp.h"
#include "librtmp/log.h"
#include "flv-mux.h"

struct rtmp_stream {
	obs_output_t     output;

	pthread_mutex_t  packets_mutex;
	struct circlebuf packets;

	bool             connecting;
	pthread_t        connect_thread;

	bool             active;
	pthread_t        send_thread;

	os_sem_t         send_sem;
	os_event_t       stop_event;

	struct dstr      path, key;
	struct dstr      username, password;

	RTMP             rtmp;
};

static const char *rtmp_stream_getname(const char *locale)
{
	/* TODO: locale stuff */
	UNUSED_PARAMETER(locale);
	return "RTMP Stream";
}

static void log_rtmp(int level, const char *format, va_list args)
{
	blogva(LOG_DEBUG, format, args);

	UNUSED_PARAMETER(level);
}

static inline void free_packets(struct rtmp_stream *stream)
{
	while (stream->packets.size) {
		struct encoder_packet packet;
		circlebuf_pop_front(&stream->packets, &packet, sizeof(packet));
		obs_free_encoder_packet(&packet);
	}
}

static void rtmp_stream_stop(void *data);

static void rtmp_stream_destroy(void *data)
{
	struct rtmp_stream *stream = data;

	if (stream) {
		rtmp_stream_stop(stream);

		free_packets(stream);
		dstr_free(&stream->path);
		dstr_free(&stream->key);
		dstr_free(&stream->username);
		dstr_free(&stream->password);
		RTMP_Close(&stream->rtmp);
		os_event_destroy(stream->stop_event);
		os_sem_destroy(stream->send_sem);
		pthread_mutex_destroy(&stream->packets_mutex);
		bfree(stream);
	}
}

static void *rtmp_stream_create(obs_data_t settings, obs_output_t output)
{
	struct rtmp_stream *stream = bzalloc(sizeof(struct rtmp_stream));
	stream->output = output;
	pthread_mutex_init_value(&stream->packets_mutex);

	RTMP_Init(&stream->rtmp);
	RTMP_LogSetCallback(log_rtmp);

	if (pthread_mutex_init(&stream->packets_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&stream->stop_event, OS_EVENT_TYPE_MANUAL) != 0)
		goto fail;

	UNUSED_PARAMETER(settings);
	return stream;

fail:
	rtmp_stream_destroy(stream);
	return NULL;
}

static void rtmp_stream_stop(void *data)
{
	struct rtmp_stream *stream = data;
	void *ret;

	os_event_signal(stream->stop_event);

	if (stream->connecting)
		pthread_join(stream->connect_thread, &ret);

	if (stream->active) {
		obs_output_end_data_capture(stream->output);
		os_sem_post(stream->send_sem);
		pthread_join(stream->send_thread, &ret);
	}

	os_event_reset(stream->stop_event);
}

static inline void set_rtmp_str(AVal *val, const char *str)
{
	bool valid  = (!str || !*str);
	val->av_val = valid ? (char*)str       : NULL;
	val->av_len = valid ? (int)strlen(str) : 0;
}

static inline void set_rtmp_dstr(AVal *val, struct dstr *str)
{
	bool valid  = !dstr_isempty(str);
	val->av_val = valid ? str->array    : NULL;
	val->av_len = valid ? (int)str->len : 0;
}

static inline bool get_next_packet(struct rtmp_stream *stream,
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

static int send_packet(struct rtmp_stream *stream,
		struct encoder_packet *packet, bool is_header)
{
	uint8_t *data;
	size_t  size;
	int     ret;

	flv_packet_mux(packet, &data, &size, is_header);
	ret = RTMP_Write(&stream->rtmp, (char*)data, (int)size);
	bfree(data);

	obs_free_encoder_packet(packet);
	return ret;
}

static bool send_remaining_packets(struct rtmp_stream *stream)
{
	struct encoder_packet packet;

	while (get_next_packet(stream, &packet))
		if (send_packet(stream, &packet, false) < 0)
			return false;

	return true;
}

static void *send_thread(void *data)
{
	struct rtmp_stream *stream = data;
	bool disconnected = false;

	while (os_sem_wait(stream->send_sem) == 0) {
		struct encoder_packet packet;

		if (os_event_try(stream->stop_event) != EAGAIN)
			break;
		if (!get_next_packet(stream, &packet))
			continue;
		if (send_packet(stream, &packet, false) < 0) {
			disconnected = true;
			break;
		}
	}

	if (!disconnected && !send_remaining_packets(stream))
		disconnected = true;

	if (disconnected)
		free_packets(stream);

	if (os_event_try(stream->stop_event) == EAGAIN)
		pthread_detach(stream->send_thread);

	stream->active = false;
	return NULL;
}

#define MIN_SENDBUF_SIZE 65535

static void send_meta_data(struct rtmp_stream *stream)
{
	uint8_t *meta_data;
	size_t  meta_data_size;

	flv_meta_data(stream->output, &meta_data, &meta_data_size);
	RTMP_Write(&stream->rtmp, (char*)meta_data, (int)meta_data_size);
	bfree(meta_data);
}

static void send_audio_header(struct rtmp_stream *stream)
{
	obs_output_t  context  = stream->output;
	obs_encoder_t aencoder = obs_output_get_audio_encoder(context);

	struct encoder_packet packet   = {
		.type = OBS_ENCODER_AUDIO,
		.timebase_den = 1
	};

	obs_encoder_get_extra_data(aencoder, &packet.data, &packet.size);
	send_packet(stream, &packet, true);
}

static void send_video_header(struct rtmp_stream *stream)
{
	obs_output_t  context  = stream->output;
	obs_encoder_t vencoder = obs_output_get_video_encoder(context);
	uint8_t       *header;
	size_t        size;

	struct encoder_packet packet   = {
		.type = OBS_ENCODER_VIDEO,
		.timebase_den = 1
	};

	obs_encoder_get_extra_data(vencoder, &header, &size);
	packet.size = obs_parse_avc_header(&packet.data, header, size);
	send_packet(stream, &packet, true);
	obs_free_encoder_packet(&packet);
}

static void send_headers(struct rtmp_stream *stream)
{
	send_meta_data(stream);
	send_audio_header(stream);
	send_video_header(stream);
}

static inline bool reset_semaphore(struct rtmp_stream *stream)
{
	os_sem_destroy(stream->send_sem);
	return os_sem_init(&stream->send_sem, 0) == 0;
}

#ifdef _WIN32
#define socklen_t int
#endif

static int init_send(struct rtmp_stream *stream)
{
	int cur_sendbuf_size = MIN_SENDBUF_SIZE;
	socklen_t size = sizeof(int);
	int ret;

	getsockopt(stream->rtmp.m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF,
			(char*)&cur_sendbuf_size, &size);

	if (cur_sendbuf_size < MIN_SENDBUF_SIZE) {
		cur_sendbuf_size = 65535;
		setsockopt(stream->rtmp.m_sb.sb_socket, SOL_SOCKET, SO_SNDBUF,
				(const char*)&cur_sendbuf_size, size);
	}

	reset_semaphore(stream);

	ret = pthread_create(&stream->send_thread, NULL, send_thread, stream);
	if (ret != 0) {
		RTMP_Close(&stream->rtmp);
		return OBS_OUTPUT_FAIL;
	}

	send_headers(stream);
	obs_output_begin_data_capture(stream->output, 0);
	return OBS_OUTPUT_SUCCESS;
}

static int try_connect(struct rtmp_stream *stream)
{
	if (!RTMP_SetupURL2(&stream->rtmp, stream->path.array,
				stream->key.array))
		return OBS_OUTPUT_BAD_PATH;

	set_rtmp_dstr(&stream->rtmp.Link.pubUser,   &stream->username);
	set_rtmp_dstr(&stream->rtmp.Link.pubPasswd, &stream->password);
	stream->rtmp.Link.swfUrl = stream->rtmp.Link.tcUrl;
	set_rtmp_str(&stream->rtmp.Link.flashVer,
			"FMLE/3.0 (compatible; FMSc/1.0)");

	stream->rtmp.m_outChunkSize       = 4096;
	stream->rtmp.m_bSendChunkSizeInfo = true;
	stream->rtmp.m_bUseNagle          = true;

	if (!RTMP_Connect(&stream->rtmp, NULL))
		return OBS_OUTPUT_CONNECT_FAILED;
	if (!RTMP_ConnectStream(&stream->rtmp, 0))
		return OBS_OUTPUT_INVALID_STREAM;

	return init_send(stream);
}

static void *connect_thread(void *data)
{
	struct rtmp_stream *stream = data;
	int ret = try_connect(stream);

	if (ret != OBS_OUTPUT_SUCCESS)
		obs_output_signal_start_fail(stream->output, ret);

	if (os_event_try(stream->stop_event) == EAGAIN)
		pthread_detach(stream->connect_thread);

	stream->connecting = false;
	return NULL;
}

static bool rtmp_stream_start(void *data)
{
	struct rtmp_stream *stream = data;
	obs_data_t settings;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;

	settings = obs_output_get_settings(stream->output);
	dstr_copy(&stream->path,     obs_data_getstring(settings, "path"));
	dstr_copy(&stream->key,      obs_data_getstring(settings, "key"));
	dstr_copy(&stream->username, obs_data_getstring(settings, "username"));
	dstr_copy(&stream->password, obs_data_getstring(settings, "password"));
	obs_data_release(settings);

	return pthread_create(&stream->connect_thread, NULL, connect_thread,
			stream) != 0;
}

static void rtmp_stream_data(void *data, struct encoder_packet *packet)
{
	struct rtmp_stream *stream = data;
	struct encoder_packet new_packet;

	if (packet->type == OBS_ENCODER_AUDIO)
		obs_duplicate_encoder_packet(&new_packet, packet);
	else if (packet->type == OBS_ENCODER_VIDEO)
		obs_parse_avc_packet(&new_packet, packet);

	pthread_mutex_lock(&stream->packets_mutex);
	circlebuf_push_back(&stream->packets, &new_packet, sizeof(new_packet));
	pthread_mutex_unlock(&stream->packets_mutex);
	os_sem_post(stream->send_sem);
}

static obs_properties_t rtmp_stream_properties(const char *locale)
{
	obs_properties_t props = obs_properties_create(locale);

	/* TODO: locale */
	obs_properties_add_text(props, "path", "Stream URL", OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "key", "Stream Key", OBS_TEXT_PASSWORD);
	obs_properties_add_text(props, "username", "User Name",
			OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "password", "Password",
			OBS_TEXT_PASSWORD);
	return props;
}

struct obs_output_info rtmp_output_info = {
	.id             = "rtmp_output",
	.flags          = OBS_OUTPUT_AV      |
	                  OBS_OUTPUT_ENCODED,
	.getname        = rtmp_stream_getname,
	.create         = rtmp_stream_create,
	.destroy        = rtmp_stream_destroy,
	.start          = rtmp_stream_start,
	.stop           = rtmp_stream_stop,
	.encoded_packet = rtmp_stream_data,
	.properties   = rtmp_stream_properties
};
