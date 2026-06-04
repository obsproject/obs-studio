/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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
#include <util/deque.h>
#include <util/threading.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>

#include "obs-ffmpeg-output.h"
#include "obs-ffmpeg-formats.h"
#include "obs-ffmpeg-compat.h"
#include "obs-ffmpeg-rist.h"
#include "obs-ffmpeg-srt.h"
#include <libavutil/channel_layout.h>
#include <libavutil/mastering_display_metadata.h>

/* ------------------------------------------------------------------------- */
#define do_log(level, format, ...) \
	blog(level, "[obs-ffmpeg mpegts muxer: '%s']: " format, obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)

static void ffmpeg_mpegts_set_last_error(struct ffmpeg_data *data, const char *error)
{
	if (data->last_error)
		bfree(data->last_error);

	data->last_error = bstrdup(error);
}

void ffmpeg_mpegts_log_error(int log_level, struct ffmpeg_data *data, const char *format, ...)
{
	va_list args;
	char out[4096];

	va_start(args, format);
	vsnprintf(out, sizeof(out), format, args);
	va_end(args);

	ffmpeg_mpegts_set_last_error(data, out);

	blog(log_level, "%s", out);
}

static bool is_rist(const char *url)
{
	return !strncmp(url, RIST_PROTO, sizeof(RIST_PROTO) - 1);
}

static bool is_srt(const char *url)
{
	return !strncmp(url, SRT_PROTO, sizeof(SRT_PROTO) - 1);
}

static bool proto_is_allowed(struct ffmpeg_output *stream)
{
	return !strncmp(stream->ff_data.config.url, UDP_PROTO, sizeof(UDP_PROTO) - 1) ||
	       !strncmp(stream->ff_data.config.url, TCP_PROTO, sizeof(TCP_PROTO) - 1) ||
	       !strncmp(stream->ff_data.config.url, HTTP_PROTO, sizeof(HTTP_PROTO) - 1);
}

static bool new_stream(struct ffmpeg_data *data, AVStream **stream, const char *name)
{

	*stream = avformat_new_stream(data->output, NULL);
	if (!*stream) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data, "Couldn't create stream for encoder '%s'", name);
		return false;
	}

	(*stream)->id = data->output->nb_streams - 1;
	return true;
}

static bool get_audio_headers(struct ffmpeg_output *stream, struct ffmpeg_data *data, int idx)
{
	AVCodecParameters *par = data->audio_infos[idx].stream->codecpar;
	obs_encoder_t *aencoder = obs_output_get_audio_encoder(stream->output, idx);
	struct encoder_packet packet = {.type = OBS_ENCODER_AUDIO, .timebase_den = 1, .track_idx = idx};

	if (obs_encoder_get_extra_data(aencoder, &packet.data, &packet.size)) {
		par->extradata = av_memdup(packet.data, packet.size);
		par->extradata_size = (int)packet.size;
		avcodec_parameters_to_context(data->audio_infos[idx].ctx, par);
		return 1;
	}
	return 0;
}

static bool get_video_headers(struct ffmpeg_output *stream, struct ffmpeg_data *data)
{
	AVCodecParameters *par = data->video->codecpar;
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	struct encoder_packet packet = {.type = OBS_ENCODER_VIDEO, .timebase_den = 1};

	if (obs_encoder_get_extra_data(vencoder, &packet.data, &packet.size)) {
		par->extradata = av_memdup(packet.data, packet.size);
		par->extradata_size = (int)packet.size;
		avcodec_parameters_to_context(data->video_ctx, data->video->codecpar);
		return 1;
	}
	return 0;
}

static bool create_video_stream(struct ffmpeg_output *stream, struct ffmpeg_data *data)
{
	AVCodecContext *context;
	struct obs_video_info ovi;

	if (!obs_get_video_info(&ovi)) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data, "No active video");
		return false;
	}
	const char *name = data->config.video_encoder;
	const AVCodecDescriptor *codec = avcodec_descriptor_get_by_name(name);
	if (!codec) {
		error("Couldn't find codec '%s'", name);
		return false;
	}
	if (!new_stream(data, &data->video, name))
		return false;

	context = avcodec_alloc_context3(NULL);
	context->codec_type = codec->type;
	context->codec_id = codec->id;
	context->bit_rate = (int64_t)data->config.video_bitrate * 1000;
	context->width = data->config.scale_width;
	context->height = data->config.scale_height;
	context->coded_width = data->config.scale_width;
	context->coded_height = data->config.scale_height;
	context->time_base = (AVRational){ovi.fps_den, ovi.fps_num};
	context->gop_size = data->config.gop_size;
	context->pix_fmt = data->config.format;
	context->color_range = data->config.color_range;
	context->color_primaries = data->config.color_primaries;
	context->color_trc = data->config.color_trc;
	context->colorspace = data->config.colorspace;
	context->chroma_sample_location = determine_chroma_location(data->config.format, data->config.colorspace);
	context->thread_count = 0;

	data->video->time_base = context->time_base;
	data->video->avg_frame_rate = av_inv_q(context->time_base);

	data->video_ctx = context;
	data->config.width = data->config.scale_width;
	data->config.height = data->config.scale_height;

	avcodec_parameters_from_context(data->video->codecpar, context);

	const bool pq = data->config.color_trc == AVCOL_TRC_SMPTE2084;
	const bool hlg = data->config.color_trc == AVCOL_TRC_ARIB_STD_B67;
	if (pq || hlg) {
		const int hdr_nominal_peak_level = pq ? (int)obs_get_video_hdr_nominal_peak_level() : (hlg ? 1000 : 0);

		size_t content_size;
		AVContentLightMetadata *const content = av_content_light_metadata_alloc(&content_size);
		content->MaxCLL = hdr_nominal_peak_level;
		content->MaxFALL = hdr_nominal_peak_level;
		av_packet_side_data_add(&data->video->codecpar->coded_side_data,
					&data->video->codecpar->nb_coded_side_data, AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
					(uint8_t *)content, content_size, 0);

		AVMasteringDisplayMetadata *const mastering = av_mastering_display_metadata_alloc();
		mastering->display_primaries[0][0] = av_make_q(17, 25);
		mastering->display_primaries[0][1] = av_make_q(8, 25);
		mastering->display_primaries[1][0] = av_make_q(53, 200);
		mastering->display_primaries[1][1] = av_make_q(69, 100);
		mastering->display_primaries[2][0] = av_make_q(3, 20);
		mastering->display_primaries[2][1] = av_make_q(3, 50);
		mastering->white_point[0] = av_make_q(3127, 10000);
		mastering->white_point[1] = av_make_q(329, 1000);
		mastering->min_luminance = av_make_q(0, 1);
		mastering->max_luminance = av_make_q(hdr_nominal_peak_level, 1);
		mastering->has_primaries = 1;
		mastering->has_luminance = 1;
		av_packet_side_data_add(&data->video->codecpar->coded_side_data,
					&data->video->codecpar->nb_coded_side_data,
					AV_PKT_DATA_MASTERING_DISPLAY_METADATA, (uint8_t *)mastering,
					sizeof(*mastering), 0);
	}

	return true;
}

static bool create_audio_stream(struct ffmpeg_output *stream, struct ffmpeg_data *data, int idx)
{
	AVCodecContext *context;
	AVStream *avstream;
	struct obs_audio_info aoi;
	const char *name = data->config.audio_encoder;
	int channels;

	const AVCodecDescriptor *codec = avcodec_descriptor_get_by_name(name);
	if (!codec) {
		warn("Couldn't find codec '%s'", name);
		return false;
	}

	if (!obs_get_audio_info(&aoi)) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data, "No active audio");
		return false;
	}

	if (!new_stream(data, &avstream, data->config.audio_encoder))
		return false;

	context = avcodec_alloc_context3(NULL);
	context->codec_type = codec->type;
	context->codec_id = codec->id;
	context->bit_rate = (int64_t)data->config.audio_bitrates[idx] * 1000;
	context->time_base = (AVRational){1, aoi.samples_per_sec};
	channels = get_audio_channels(aoi.speakers);
	context->sample_rate = aoi.samples_per_sec;

	av_channel_layout_default(&context->ch_layout, channels);
	if (aoi.speakers == SPEAKERS_4POINT1)
		context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_4POINT1;

	context->sample_fmt = AV_SAMPLE_FMT_S16;
	context->frame_size = data->config.frame_size;

	avstream->time_base = context->time_base;

	data->audio_samplerate = aoi.samples_per_sec;
	data->audio_format = convert_ffmpeg_sample_format(context->sample_fmt);
	data->audio_planes = get_audio_planes(data->audio_format, aoi.speakers);
	data->audio_size = get_audio_size(data->audio_format, aoi.speakers, 1);

	data->audio_infos[idx].stream = avstream;
	data->audio_infos[idx].ctx = context;
	avcodec_parameters_from_context(data->audio_infos[idx].stream->codecpar, context);
	return true;
}

static inline bool init_streams(struct ffmpeg_output *stream, struct ffmpeg_data *data)
{
	if (!create_video_stream(stream, data))
		return false;

	if (data->num_audio_streams) {
		data->audio_infos = calloc(data->num_audio_streams, sizeof(*data->audio_infos));
		for (int i = 0; i < data->num_audio_streams; i++) {
			if (!create_audio_stream(stream, data, i))
				return false;
		}
	}

	return true;
}

int ff_network_init(void)
{
#if HAVE_WINSOCK2_H
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(1, 1), &wsaData))
		return 0;
#endif
	return 1;
}

static inline int connect_mpegts_url(struct ffmpeg_output *stream, bool is_rist)
{
	int err = 0;
	const char *url = stream->ff_data.config.url;
	if (!ff_network_init()) {
		ffmpeg_mpegts_log_error(LOG_ERROR, &stream->ff_data, "Couldn't initialize network");
		return OBS_OUTPUT_ERROR;
	}

	URLContext *uc = av_mallocz(sizeof(URLContext) + strlen(url) + 1);
	if (!uc) {
		ffmpeg_mpegts_log_error(LOG_ERROR, &stream->ff_data, "Couldn't allocate memory");
		err = OBS_OUTPUT_ERROR;
		goto fail;
	}

	uc->url = (char *)url;
	uc->max_packet_size = is_rist ? RIST_MAX_PAYLOAD_SIZE : SRT_LIVE_DEFAULT_PAYLOAD_SIZE;

	if (stream->ff_data.config.srt_pkt_size)
		uc->max_packet_size = stream->ff_data.config.srt_pkt_size;

	uc->priv_data = is_rist ? av_mallocz(sizeof(RISTContext)) : av_mallocz(sizeof(SRTContext));

	if (!uc->priv_data) {
		ffmpeg_mpegts_log_error(LOG_ERROR, &stream->ff_data, "Couldn't allocate memory");
		err = OBS_OUTPUT_ERROR;
		goto fail;
	}

	/* For SRT, pass streamid & passphrase; for RIST, pass passphrase, username
	 * & password.
	 */
	if (!is_rist) {
		SRTContext *context = (SRTContext *)uc->priv_data;
		context->streamid = NULL;
		if (stream->ff_data.config.stream_id != NULL) {
			if (strlen(stream->ff_data.config.stream_id))
				context->streamid = av_strdup(stream->ff_data.config.stream_id);
		}
		context->passphrase = NULL;
		if (stream->ff_data.config.encrypt_passphrase != NULL) {
			if (strlen(stream->ff_data.config.encrypt_passphrase))
				context->passphrase = av_strdup(stream->ff_data.config.encrypt_passphrase);
		}
	} else {
		RISTContext *context = (RISTContext *)uc->priv_data;
		context->secret = NULL;
		if (stream->ff_data.config.encrypt_passphrase != NULL) {
			if (strlen(stream->ff_data.config.encrypt_passphrase))
				context->secret = bstrdup(stream->ff_data.config.encrypt_passphrase);
		}
		context->username = NULL;
		if (stream->ff_data.config.username != NULL) {
			if (strlen(stream->ff_data.config.username))
				context->username = bstrdup(stream->ff_data.config.username);
		}
		context->password = NULL;
		if (stream->ff_data.config.password != NULL) {
			if (strlen(stream->ff_data.config.password))
				context->password = bstrdup(stream->ff_data.config.password);
		}
	}
	stream->h = uc;
	if (is_rist)
		err = librist_open(uc, uc->url);
	else
		err = libsrt_open(uc, uc->url);

	if (err < 0)
		goto fail;
	else
		stream->has_connected = true;

	return 0;
fail:
	stream->has_connected = false;

	if (uc) {
		if (is_rist)
			librist_close(uc);
		else
			libsrt_close(uc);

		av_freep(&uc->priv_data);
	}

	av_freep(&uc);
#if HAVE_WINSOCK2_H
	WSACleanup();
#endif
	return err;
}

#if LIBAVFORMAT_VERSION_MAJOR >= 61
typedef int (*write_packet_cb)(void *, const uint8_t *, int);
#else
typedef int (*write_packet_cb)(void *, uint8_t *, int);
#endif

static inline int allocate_custom_aviocontext(struct ffmpeg_output *stream, bool is_rist)
{
	/* allocate buffers */
	uint8_t *buffer = NULL;
	int buffer_size;
	URLContext *h = stream->h;
	AVIOContext *s = NULL;

	buffer_size = UDP_DEFAULT_PAYLOAD_SIZE;

	if (h->max_packet_size)
		buffer_size = h->max_packet_size;

	buffer = av_malloc(buffer_size);

	if (!buffer)
		return AVERROR(ENOMEM);

	/* allocate custom avio_context */
	if (is_rist)
		s = avio_alloc_context(buffer, buffer_size, AVIO_FLAG_WRITE, h, NULL, (write_packet_cb)librist_write,
				       NULL);
	else
		s = avio_alloc_context(buffer, buffer_size, AVIO_FLAG_WRITE, h, NULL, (write_packet_cb)libsrt_write,
				       NULL);

	if (!s)
		goto fail;

	s->max_packet_size = h->max_packet_size;
	s->opaque = h;
	stream->s = s;
	stream->ff_data.output->pb = s;

	return 0;
fail:
	av_freep(&buffer);
	return AVERROR(ENOMEM);
}

static inline int open_output_file(struct ffmpeg_output *stream, struct ffmpeg_data *data)
{
	int ret;
	bool rist = data->config.is_rist;
	bool srt = data->config.is_srt;
	bool allowed_proto = proto_is_allowed(stream);
	AVDictionary *dict = NULL;

	/* Retrieve protocol settings for udp, tcp, rtp ... (not srt or rist).
	 * These options will be passed to protocol by avio_open2 through dict.
	 * The invalid options will be left in dict. */
	if (!rist && !srt) {
		if ((ret = av_dict_parse_string(&dict, data->config.protocol_settings, "=", " ", 0))) {
			ffmpeg_mpegts_log_error(LOG_WARNING, data, "Failed to parse protocol settings: %s, %s",
						data->config.protocol_settings, av_err2str(ret));

			av_dict_free(&dict);
			return OBS_OUTPUT_ERROR;
		}

		if (av_dict_count(dict) > 0) {
			struct dstr str = {0};

			AVDictionaryEntry *entry = NULL;
			while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)))
				dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

			info("Using protocol settings: %s", str.array);
			dstr_free(&str);
		}
	}

	/* Ensure h264 bitstream auto conversion from avcc to annex B */
	data->output->flags |= AVFMT_FLAG_AUTO_BSF;

	/* Open URL for rist, srt or other protocols compatible with mpegts
	 *  muxer supported by avformat (udp, tcp, rtp ...).
	 */
	if (rist) {
		ret = connect_mpegts_url(stream, true);
	} else if (srt) {
		ret = connect_mpegts_url(stream, false);
	} else if (allowed_proto) {
		ret = avio_open2(&data->output->pb, data->config.url, AVIO_FLAG_WRITE, NULL, &dict);
	} else {
		info("[ffmpeg mpegts muxer]: Invalid protocol: %s", data->config.url);
		return OBS_OUTPUT_BAD_PATH;
	}

	if (ret < 0) {
		if ((rist || srt)) {
			switch (ret) {
			case OBS_OUTPUT_BAD_PATH:
				error("URL is malformed");
				break;
			case OBS_OUTPUT_DISCONNECTED:
				error("Server unreachable");
				break;
			case OBS_OUTPUT_ERROR:
				error("I/O error");
				break;
			case OBS_OUTPUT_CONNECT_FAILED:
			case OBS_OUTPUT_INVALID_STREAM:
				error("Failed to open the url or invalid stream");
				break;
			default:
				break;
			}
		} else {
			ffmpeg_mpegts_log_error(LOG_WARNING, data, "Couldn't open '%s', %s", data->config.url,
						av_err2str(ret));
			av_dict_free(&dict);
		}
		return ret;
	}

	/* Log invalid protocol settings for all protocols except srt or rist.
	 * Or for srt & rist, allocate custom avio_ctx which will host the
	 * protocols write callbacks.
	 */
	if (!rist && !srt) {
		if (av_dict_count(dict) > 0) {
			struct dstr str = {0};

			AVDictionaryEntry *entry = NULL;
			while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)))
				dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

			info("[ffmpeg mpegts muxer]: Invalid protocol settings: %s", str.array);
			dstr_free(&str);
		}
		av_dict_free(&dict);
	} else {
		ret = allocate_custom_aviocontext(stream, rist);
		if (ret < 0) {
			info("Couldn't allocate custom avio_context for url: '%s', %s", data->config.url,
			     av_err2str(ret));
			return OBS_OUTPUT_ERROR;
		}
	}

	return OBS_OUTPUT_SUCCESS;
}

static void close_video(struct ffmpeg_data *data)
{
	avcodec_free_context(&data->video_ctx);
}

static void close_audio(struct ffmpeg_data *data)
{
	for (int idx = 0; idx < data->num_audio_streams; idx++) {
		for (size_t i = 0; i < MAX_AV_PLANES; i++)
			deque_free(&data->excess_frames[idx][i]);

		if (data->samples[idx][0])
			av_freep(&data->samples[idx][0]);
		if (data->audio_infos[idx].ctx) {
			avcodec_free_context(&data->audio_infos[idx].ctx);
		}
		if (data->aframe[idx])
			av_frame_free(&data->aframe[idx]);
	}
}

static void close_mpegts_url(struct ffmpeg_output *stream, bool is_rist)
{
	int err = 0;
	URLContext *h = stream->h;
	if (!h)
		return; /* can happen when opening the url fails */

	/* close rist or srt URLs ; free URLContext */
	if (is_rist) {
		err = librist_close(h);
	} else {
		err = libsrt_close(h);
	}
	av_freep(&h->priv_data);
	av_freep(&h);
	h = NULL;

	/* close custom avio_context for srt or rist */
	AVIOContext *s = stream->s;
	if (!s)
		return;

	avio_flush(s);
	s->opaque = NULL;
	av_freep(&s->buffer);
	avio_context_free(&s);
	s = NULL;

	if (err)
		info("[ffmpeg mpegts muxer]: Error closing URL %s", stream->ff_data.config.url);
}

void ffmpeg_mpegts_data_free(struct ffmpeg_output *stream, struct ffmpeg_data *data)
{
	if (data->initialized)
		av_write_trailer(data->output);

	if (data->video)
		close_video(data);
	if (data->audio_infos) {
		close_audio(data);
		free(data->audio_infos);
	}

	if (data->output) {
		if (data->config.is_rist || data->config.is_srt) {
			if (stream->has_connected) {
				close_mpegts_url(stream, data->config.is_rist);
				stream->has_connected = false;
			}
		} else {
			avio_close(data->output->pb);
		}
		avformat_free_context(data->output);
		data->video = NULL;
		data->audio_infos = NULL;
		data->output = NULL;
		data->num_audio_streams = 0;
	}

	if (data->last_error)
		bfree(data->last_error);

	memset(data, 0, sizeof(struct ffmpeg_data));
}

bool ffmpeg_mpegts_data_init(struct ffmpeg_output *stream, struct ffmpeg_data *data, struct ffmpeg_cfg *config)
{
	memset(data, 0, sizeof(struct ffmpeg_data));
	data->config = *config;
	data->num_audio_streams = config->audio_mix_count;

	if (!config->url || !*config->url)
		return false;

	avformat_network_init();

	const AVOutputFormat *output_format = av_guess_format("mpegts", NULL, "video/M2PT");

	if (output_format == NULL) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data, "Couldn't set output format to mpegts");
		goto fail;
	} else {
		info("Output format name and long_name: %s, %s", output_format->name ? output_format->name : "unknown",
		     output_format->long_name ? output_format->long_name : "unknown");
	}

	avformat_alloc_output_context2(&data->output, output_format, NULL, data->config.url);
	av_dict_set(&data->output->metadata, "service_provider", "obs-studio", 0);
	av_dict_set(&data->output->metadata, "service_name", "mpegts output", 0);

	if (!data->output) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data, "Couldn't create avformat context");
		goto fail;
	}

	return true;

fail:
	warn("ffmpeg_data_init failed");
	return false;
}

/* ------------------------------------------------------------------------- */

static inline bool stopping(struct ffmpeg_output *stream)
{
	return os_atomic_load_bool(&stream->stopping);
}

static const char *ffmpeg_mpegts_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegMpegts");
}

static void ffmpeg_mpegts_log_callback(void *param, int level, const char *format, va_list args)
{
	if (level <= AV_LOG_INFO)
		blogva(LOG_DEBUG, format, args);

	UNUSED_PARAMETER(param);
}

static void *ffmpeg_mpegts_create(obs_data_t *settings, obs_output_t *output)
{
	struct ffmpeg_output *data = bzalloc(sizeof(struct ffmpeg_output));
	pthread_mutex_init_value(&data->write_mutex);
	data->output = output;

	if (pthread_mutex_init(&data->write_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&data->stop_event, OS_EVENT_TYPE_AUTO) != 0)
		goto fail;
	if (os_sem_init(&data->write_sem, 0) != 0)
		goto fail;
	if (pthread_mutex_init(&data->start_stop_mutex, NULL) != 0)
		goto fail;

	av_log_set_callback(ffmpeg_mpegts_log_callback);

	UNUSED_PARAMETER(settings);
	return data;

fail:
	pthread_mutex_destroy(&data->write_mutex);
	os_event_destroy(data->stop_event);
	pthread_mutex_destroy(&data->start_stop_mutex);
	bfree(data);
	return NULL;
}

static inline bool active(void *data)
{
	struct ffmpeg_output *stream = data;
	return os_atomic_load_bool(&stream->running) && stream->write_thread_active;
}

static void ffmpeg_mpegts_full_stop(void *data);
static void ffmpeg_mpegts_deactivate(struct ffmpeg_output *stream);
static void ffmpeg_mpegts_stop(void *data, uint64_t ts);

static void ffmpeg_mpegts_destroy(void *data)
{
	struct ffmpeg_output *stream = data;

	if (stream) {
		/* immediately stop without draining packets */
		ffmpeg_mpegts_stop(data, 0);

		/* Now wait for that stop to finish */
		pthread_mutex_lock(&stream->start_stop_mutex);
		if (os_atomic_load_bool(&stream->start_stop_thread_active))
			pthread_join(stream->start_stop_thread, NULL);
		pthread_mutex_unlock(&stream->start_stop_mutex);

		/* Clean up resources */
		pthread_mutex_destroy(&stream->write_mutex);
		os_sem_destroy(stream->write_sem);
		os_event_destroy(stream->stop_event);
		pthread_mutex_destroy(&stream->start_stop_mutex);

		bfree(data);
	}
}

static uint64_t get_packet_sys_dts(struct ffmpeg_output *stream, AVPacket *packet)
{
	struct ffmpeg_data *data = &stream->ff_data;
	uint64_t pause_offset = obs_output_get_pause_offset(stream->output);
	uint64_t start_ts;

	AVRational time_base;

	if (data->video && data->video->index == packet->stream_index) {
		time_base = data->video->time_base;
		start_ts = stream->video_start_ts;
	} else {
		time_base = data->audio_infos[0].stream->time_base;
		start_ts = stream->audio_start_ts;
	}

	return start_ts + pause_offset + (uint64_t)av_rescale_q(packet->dts, time_base, (AVRational){1, 1000000000});
}

static int mpegts_process_packet(struct ffmpeg_output *stream)
{
	AVPacket *packet = NULL;
	int ret = 0;

	pthread_mutex_lock(&stream->write_mutex);
	if (stream->packets.num) {
		packet = stream->packets.array[0];
		da_erase(stream->packets, 0);
	}
	pthread_mutex_unlock(&stream->write_mutex);

	if (!packet)
		return 0;

	//blog(LOG_DEBUG,
	//     "size = %d, flags = %lX, stream = %d, "
	//     "packets queued: %lu",
	//     packet->size, packet->flags, packet->stream_index,
	//     output->packets.num);

	if (stopping(stream)) {
		uint64_t sys_ts = get_packet_sys_dts(stream, packet);
		if (sys_ts >= stream->stop_ts) {
			ret = 0;
			goto end;
		}
	}
	stream->total_bytes += packet->size;
	uint8_t *buf = packet->data;
	ret = av_interleaved_write_frame(stream->ff_data.output, packet);
	av_freep(&buf);

	if (ret < 0) {
		ffmpeg_mpegts_log_error(LOG_WARNING, &stream->ff_data, "process_packet: Error writing packet: %s",
					av_err2str(ret));

		/* Treat "Invalid data found when processing input" and
		 * "Invalid argument" as non-fatal */
		if (ret == AVERROR_INVALIDDATA || ret == -EINVAL) {
			ret = 0;
		}
	}
end:
	av_packet_free(&packet);
	return ret;
}

static void ffmpeg_mpegts_stop_internal(void *data, uint64_t ts, bool signal);
static void *write_thread(void *data)
{
	struct ffmpeg_output *stream = data;

	while (os_sem_wait(stream->write_sem) == 0) {
		/* check to see if shutting down */
		if (os_event_try(stream->stop_event) == 0)
			break;

		int ret = mpegts_process_packet(stream);
		if (ret != 0) {
			if (stream->ff_data.config.is_srt) {
				SRTContext *s = (SRTContext *)stream->h->priv_data;
				SRT_SOCKSTATUS srt_sock_state = srt_getsockstate(s->fd);
				if (srt_sock_state == SRTS_BROKEN || srt_sock_state == SRTS_NONEXIST)
					obs_output_signal_stop(stream->output, OBS_OUTPUT_DISCONNECTED);
				else
					obs_output_signal_stop(stream->output, OBS_OUTPUT_ERROR);
			} else if (stream->ff_data.config.is_rist) {
				obs_output_signal_stop(stream->output, OBS_OUTPUT_DISCONNECTED);
			} else {
				obs_output_signal_stop(stream->output, OBS_OUTPUT_ERROR);
			}
			break;
		}
	}
	os_atomic_set_bool(&stream->stopping, true);
	return NULL;
}

static bool get_extradata(struct ffmpeg_output *stream)
{
	struct ffmpeg_data *ff_data = &stream->ff_data;

	/* get extradata for av headers from encoders */
	if (!get_video_headers(stream, ff_data))
		return false;
	for (int i = 0; i < ff_data->num_audio_streams; i++) {
		if (!get_audio_headers(stream, ff_data, i))
			return false;
	}

	return true;
}

static bool fetch_service_info(struct ffmpeg_output *stream, struct ffmpeg_cfg *config, int *code)
{
	obs_service_t *service = obs_output_get_service(stream->output);
	if (!service) {
		*code = OBS_OUTPUT_ERROR;
		return false;
	}
	config->url = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_SERVER_URL);
	config->username = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_USERNAME);
	config->password = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_PASSWORD);
	config->stream_id = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_STREAM_ID);
	config->encrypt_passphrase = obs_service_get_connect_info(service, OBS_SERVICE_CONNECT_INFO_ENCRYPT_PASSPHRASE);
	config->format_name = "mpegts";
	config->format_mime_type = "video/M2PT";
	config->is_rist = is_rist(config->url);
	config->is_srt = is_srt(config->url);
	config->srt_pkt_size = 0; /* use default, which is usually 1316 for mpegts */

	/* parse pkt_size option */
	const char *p;
	char buf[1024];
	p = strchr(config->url, '?');
	if (p && (av_find_info_tag(buf, sizeof(buf), "payload_size", p) ||
		  av_find_info_tag(buf, sizeof(buf), "pkt_size", p))) {
		config->srt_pkt_size = strtol(buf, NULL, 10);
	}
	return true;
}

static bool setup_video_settings(struct ffmpeg_output *stream, struct ffmpeg_cfg *config, int *code)
{
	/* video settings */

	/* a) set width & height */
	config->width = (int)obs_output_get_width(stream->output);
	config->height = (int)obs_output_get_height(stream->output);
	config->scale_width = config->width;
	config->scale_height = config->height;

	/* b) set video codec & ID from video encoder */
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	config->video_encoder = obs_encoder_get_codec(vencoder);
	if (strcmp(config->video_encoder, "h264") == 0)
		config->video_encoder_id = AV_CODEC_ID_H264;
	else
		config->video_encoder_id = AV_CODEC_ID_AV1;

	/* c) set video format from OBS to FFmpeg */
	video_t *video = obs_encoder_video(vencoder);
	config->format = obs_to_ffmpeg_video_format(video_output_get_format(video));

	if (config->format == AV_PIX_FMT_NONE) {
		blog(LOG_WARNING, "Invalid pixel format used for mpegts output");
		*code = OBS_OUTPUT_ERROR;
		return false;
	}

	/* d) set colorspace, color_range & transfer characteristic (from voi) */
	const struct video_output_info *voi = video_output_get_info(video);
	config->color_range = voi->range == VIDEO_RANGE_FULL ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
	config->colorspace = format_is_yuv(voi->format) ? AVCOL_SPC_BT709 : AVCOL_SPC_RGB;
	switch (voi->colorspace) {
	case VIDEO_CS_601:
		config->color_primaries = AVCOL_PRI_SMPTE170M;
		config->color_trc = AVCOL_TRC_SMPTE170M;
		config->colorspace = AVCOL_SPC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		config->color_primaries = AVCOL_PRI_BT709;
		config->color_trc = AVCOL_TRC_BT709;
		config->colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_SRGB:
		config->color_primaries = AVCOL_PRI_BT709;
		config->color_trc = AVCOL_TRC_IEC61966_2_1;
		config->colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_2100_PQ:
		config->color_primaries = AVCOL_PRI_BT2020;
		config->color_trc = AVCOL_TRC_SMPTE2084;
		config->colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	case VIDEO_CS_2100_HLG:
		config->color_primaries = AVCOL_PRI_BT2020;
		config->color_trc = AVCOL_TRC_ARIB_STD_B67;
		config->colorspace = AVCOL_SPC_BT2020_NCL;
	}
	/* e)  set video bitrate & gop through video encoder settings */
	obs_data_t *settings = obs_encoder_get_settings(vencoder);
	config->video_bitrate = (int)obs_data_get_int(settings, "bitrate");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	config->gop_size = keyint_sec ? keyint_sec * voi->fps_num / voi->fps_den : 250;
	obs_data_release(settings);
	return true;
}

static bool setup_audio_settings(struct ffmpeg_output *stream, struct ffmpeg_cfg *config)
{
	/* Audio settings */
	/* a) get audio encoders & retrieve number of tracks */
	obs_encoder_t *aencoders[MAX_AUDIO_MIXES];
	int num_tracks = 0;

	for (;;) {
		obs_encoder_t *aencoder = obs_output_get_audio_encoder(stream->output, num_tracks);
		if (!aencoder)
			break;
		aencoders[num_tracks] = aencoder;
		num_tracks++;
	}

	config->audio_mix_count = num_tracks;

	/* b) set audio codec & id from audio encoder */
	config->audio_encoder = obs_encoder_get_codec(aencoders[0]);
	if (strcmp(config->audio_encoder, "aac") == 0)
		config->audio_encoder_id = AV_CODEC_ID_AAC;
	else if (strcmp(config->audio_encoder, "opus") == 0)
		config->audio_encoder_id = AV_CODEC_ID_OPUS;

	/* c) get audio bitrate from the audio encoder. */
	for (int idx = 0; idx < num_tracks; idx++) {
		obs_data_t *settings = obs_encoder_get_settings(aencoders[idx]);
		config->audio_bitrates[idx] = (int)obs_data_get_int(settings, "bitrate");
		obs_data_release(settings);
	}

	/* d) set audio frame size  */
	config->frame_size = (int)obs_encoder_get_frame_size(aencoders[0]);
	return true;
}

static bool setup_muxer_settings(struct ffmpeg_output *stream, struct ffmpeg_cfg *config)
{
	/* Muxer & protocol settings */
	/* TODO: This will require some UI to be written for the mpegts output. */

	obs_data_t *settings = obs_output_get_settings(stream->output);
	obs_data_set_default_string(settings, "muxer_settings", "");
	config->muxer_settings = obs_data_get_string(settings, "muxer_settings");
	obs_data_release(settings);
	config->protocol_settings = "";
	return true;
}

static bool ffmpeg_mpegts_finalize(struct ffmpeg_output *stream, struct ffmpeg_cfg *config, int *code)
{
	bool success = ffmpeg_mpegts_data_init(stream, &stream->ff_data, config);
	if (!success) {
		if (stream->ff_data.last_error) {
			obs_output_set_last_error(stream->output, stream->ff_data.last_error);
		}
		ffmpeg_mpegts_data_free(stream, &stream->ff_data);
		*code = OBS_OUTPUT_ERROR;
		return false;
	}
	struct ffmpeg_data *ff_data = &stream->ff_data;
	if (!stream->got_headers) {
		if (!init_streams(stream, ff_data)) {
			error("mpegts avstream failed to be created");
			*code = OBS_OUTPUT_ERROR;
			return false;
		}
		*code = open_output_file(stream, ff_data);
		if (*code != OBS_OUTPUT_SUCCESS) {
			error("Failed to open the url");
			return false;
		}
		av_dump_format(ff_data->output, 0, NULL, 1);
	}
	os_event_reset(stream->stop_event);
	int ret = pthread_create(&stream->write_thread, NULL, write_thread, stream);
	if (ret != 0) {
		ffmpeg_mpegts_log_error(LOG_WARNING, &stream->ff_data,
					"ffmpeg_output_start: Failed to create write thread.");
		*code = OBS_OUTPUT_ERROR;
		return false;
	}
	stream->write_thread_active = true;
	stream->total_bytes = 0;
	obs_output_begin_data_capture(stream->output, 0);
	return true;
}
static void stop(void *data, bool signal, uint64_t ts);
static bool set_config(struct ffmpeg_output *stream)
{
	struct ffmpeg_cfg config;
	int code = OBS_OUTPUT_ERROR;

	if (!fetch_service_info(stream, &config, &code))
		goto fail;
	if (!setup_video_settings(stream, &config, &code))
		goto fail;

	setup_audio_settings(stream, &config);
	setup_muxer_settings(stream, &config);

	/* unused for now; placeholder. */
	config.video_settings = "";
	config.audio_settings = "";

	if (!ffmpeg_mpegts_finalize(stream, &config, &code))
		goto fail;

	return true;
fail:
	obs_output_signal_stop(stream->output, code);
	stop(stream, false, 0);
	return false;
}

static bool start(void *data)
{
	struct ffmpeg_output *stream = data;
	if (!set_config(stream))
		return false;

	os_atomic_set_bool(&stream->running, true);
	os_atomic_set_bool(&stream->stopping, false);
	return true;
}

static void ffmpeg_mpegts_full_stop(void *data)
{
	struct ffmpeg_output *stream = data;

	if (active(stream)) {
		ffmpeg_mpegts_deactivate(stream);
	}
	ffmpeg_mpegts_data_free(stream, &stream->ff_data);
}

static void stop(void *data, bool signal, uint64_t ts)
{
	struct ffmpeg_output *stream = data;

	if (active(stream)) {
		/* bypassed when called by destroy ==> no draining, instant stop */
		if (ts > 0) {
			/* this controls the draining of all packets in the write_threads when we stop */
			stream->stop_ts = ts;
			os_atomic_set_bool(&stream->stopping, true);
		}
		ffmpeg_mpegts_full_stop(stream);
	}

	/* Based on whip-output.cpp reconnect logic, coded by tt2468. "signal" exists because we have to preserve the
	 * "running" state across reconnect attempts. If we don't emit a signal if something calls obs_output_stop()
	 * and it's reconnecting, you'll desync the UI, as the output will be "stopped" and not "reconnecting", but the
	 * "stop" signal will have never been emitted.
	 * We only clear bool 'running' if this is a user-requested stop, not a reconnect-triggered stop.
	 */

	if (os_atomic_load_bool(&stream->running) && signal) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_SUCCESS);
		os_atomic_set_bool(&stream->running, false);
	}
}

void *start_stop_thread_fn(void *data)
{
	struct mpegts_cmd *cmd = data;
	struct ffmpeg_output *stream = cmd->stream;

	if (cmd->type == MPEGTS_CMD_START) {
		if (!start(stream))
			blog(LOG_ERROR, "failed to start the mpegts output");
	} else if (cmd->type == MPEGTS_CMD_STOP) {
		stop(stream, cmd->signal_stop, cmd->ts);
	}

	os_atomic_set_bool(&stream->start_stop_thread_active, false);
	bfree(cmd);
	return NULL;
}

static void ffmpeg_mpegts_stop_internal(void *data, uint64_t ts, bool signal)
{
	struct ffmpeg_output *stream = data;
	struct mpegts_cmd *cmd = bzalloc(sizeof(struct mpegts_cmd));
	cmd->type = MPEGTS_CMD_STOP;
	cmd->signal_stop = signal;
	cmd->stream = stream;
	cmd->ts = ts;

	bool have_to_join = false;
	pthread_t to_join;

	/* macOS fix: handover of pthread; for some reason, maybe related to srt sockets, if the pthread joining is
	 * done within the mutex, there can be a stall. So we just copy it and join out of the mutex. */
	pthread_mutex_lock(&stream->start_stop_mutex);
	if (os_atomic_load_bool(&stream->start_stop_thread_active)) {
		to_join = stream->start_stop_thread;
		os_atomic_set_bool(&stream->start_stop_thread_active, false);
		have_to_join = true;
	}
	pthread_mutex_unlock(&stream->start_stop_mutex);

	/* Join outside the mutex (avoid macOS stalls & priority inversion) */
	if (have_to_join) {
		if (!pthread_equal(pthread_self(), to_join)) {
			pthread_join(to_join, NULL);
		} else {
			/* Shouldn't happen */
			error("The dev made a big mistake. Post an issue.");
		}
	}

	pthread_mutex_lock(&stream->start_stop_mutex);
	pthread_create(&stream->start_stop_thread, NULL, start_stop_thread_fn, cmd);
	os_atomic_set_bool(&stream->start_stop_thread_active, true);
	pthread_mutex_unlock(&stream->start_stop_mutex);
}

static void ffmpeg_mpegts_stop(void *data, uint64_t ts)
{
	ffmpeg_mpegts_stop_internal(data, ts, true);
}

static bool ffmpeg_mpegts_start(void *data)
{
	struct ffmpeg_output *stream = data;
	struct mpegts_cmd *cmd = bzalloc(sizeof(struct mpegts_cmd));
	cmd->stream = stream;
	cmd->type = MPEGTS_CMD_START;
	cmd->signal_stop = false;
	cmd->ts = 0;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;

	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	pthread_mutex_lock(&stream->start_stop_mutex);

	if (os_atomic_load_bool(&stream->start_stop_thread_active))
		pthread_join(stream->start_stop_thread, NULL);

	if (stream->write_thread_active)
		pthread_join(stream->write_thread, NULL);

	stream->audio_start_ts = 0;
	stream->video_start_ts = 0;
	stream->total_bytes = 0;
	stream->got_headers = false;

	pthread_create(&stream->start_stop_thread, NULL, start_stop_thread_fn, cmd);
	os_atomic_set_bool(&stream->start_stop_thread_active, true);
	pthread_mutex_unlock(&stream->start_stop_mutex);

	return true;
}

static void ffmpeg_mpegts_deactivate(struct ffmpeg_output *stream)
{
	if (stream->write_thread_active) {
		os_event_signal(stream->stop_event);
		os_sem_post(stream->write_sem);
		pthread_join(stream->write_thread, NULL);
		stream->write_thread_active = false;
	}

	pthread_mutex_lock(&stream->write_mutex);

	for (size_t i = 0; i < stream->packets.num; i++)
		av_packet_free(stream->packets.array + i);

	da_free(stream->packets);

	pthread_mutex_unlock(&stream->write_mutex);
}

static uint64_t ffmpeg_mpegts_total_bytes(void *data)
{
	struct ffmpeg_output *stream = data;
	return stream->total_bytes;
}

static inline int64_t rescale_ts2(AVStream *stream, AVRational codec_time_base, int64_t val)
{
	return av_rescale_q_rnd(val / codec_time_base.num, codec_time_base, stream->time_base,
				AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
}

/* Convert obs encoder_packet to FFmpeg AVPacket and write to circular buffer
 * where it will be processed in the write_thread by process_packet.
 */
void mpegts_write_packet(struct ffmpeg_output *stream, struct encoder_packet *encpacket)
{
	if (stopping(stream) || !stream->ff_data.video || !stream->ff_data.video_ctx || !stream->ff_data.audio_infos)
		return;
	bool is_video = encpacket->type == OBS_ENCODER_VIDEO;
	if (!is_video) {
		if (!stream->ff_data.audio_infos[encpacket->track_idx].stream)
			return;
	}

	AVStream *avstream = is_video ? stream->ff_data.video
				      : stream->ff_data.audio_infos[encpacket->track_idx].stream;
	AVPacket *packet = NULL;

	const AVRational codec_time_base = is_video ? stream->ff_data.video_ctx->time_base
						    : stream->ff_data.audio_infos[encpacket->track_idx].ctx->time_base;

	packet = av_packet_alloc();

	packet->data = av_memdup(encpacket->data, (int)encpacket->size);
	if (packet->data == NULL) {
		error("Couldn't allocate packet data");
		goto fail;
	}
	packet->size = (int)encpacket->size;
	packet->stream_index = avstream->id;
	packet->pts = rescale_ts2(avstream, codec_time_base, encpacket->pts);
	packet->dts = rescale_ts2(avstream, codec_time_base, encpacket->dts);

	if (encpacket->keyframe)
		packet->flags = AV_PKT_FLAG_KEY;

	pthread_mutex_lock(&stream->write_mutex);
	da_push_back(stream->packets, &packet);
	pthread_mutex_unlock(&stream->write_mutex);
	os_sem_post(stream->write_sem);
	return;
fail:
	av_packet_free(&packet);
}

static bool write_header(struct ffmpeg_output *stream, struct ffmpeg_data *data)
{
	AVDictionary *dict = NULL;
	int ret;
	/* get mpegts muxer settings (can be used with rist, srt, rtp, etc ... */
	if ((ret = av_dict_parse_string(&dict, data->config.muxer_settings, "=", " ", 0))) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data, "Failed to parse muxer settings: %s, %s",
					data->config.muxer_settings, av_err2str(ret));

		av_dict_free(&dict);
		return false;
	}

	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		info("Using muxer settings: %s", str.array);
		dstr_free(&str);
	}

	/* Allocate the stream private data and write the stream header. */
	ret = avformat_write_header(data->output, &dict);
	if (ret < 0) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data, "Error setting stream header for '%s': %s", data->config.url,
					av_err2str(ret));
		return false;
	}

	/* Log invalid muxer settings. */
	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry, AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		info("[ffmpeg mpegts muxer]: Invalid mpegts muxer settings: %s", str.array);
		dstr_free(&str);
	}
	av_dict_free(&dict);

	return true;
}

static void ffmpeg_mpegts_data(void *data, struct encoder_packet *packet)
{
	struct ffmpeg_output *stream = data;
	struct ffmpeg_data *ff_data = &stream->ff_data;
	int code;
	if (!stream->got_headers) {
		if (get_extradata(stream)) {
			stream->got_headers = true;
		} else {
			warn("Failed to retrieve headers");
			code = OBS_OUTPUT_INVALID_STREAM;
			goto fail;
		}
		if (!write_header(stream, ff_data)) {
			error("Failed to write headers");
			code = OBS_OUTPUT_INVALID_STREAM;
			goto fail;
		}
		av_dump_format(ff_data->output, 0, NULL, 1);
		ff_data->initialized = true;
	}

	if (!active(stream))
		return;

	/* encoder failure */
	if (!packet) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_ENCODE_ERROR);
		ffmpeg_mpegts_stop_internal(stream, 0, false);
		return;
	}

	if (stopping(stream)) {
		if (packet->sys_dts_usec >= (int64_t)stream->stop_ts) {
			ffmpeg_mpegts_stop_internal(stream, 0, false);
			return;
		}
	}

	mpegts_write_packet(stream, packet);
	return;
fail:
	obs_output_signal_stop(stream->output, code);
	ffmpeg_mpegts_stop_internal(stream, 0, false);
}

static obs_properties_t *ffmpeg_mpegts_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "path", obs_module_text("FilePath"), OBS_TEXT_DEFAULT);
	return props;
}

struct obs_output_info ffmpeg_mpegts_muxer = {
	.id = "ffmpeg_mpegts_muxer",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK | OBS_OUTPUT_SERVICE,
	.protocols = "SRT;RIST",
#ifdef ENABLE_HEVC
	.encoded_video_codecs = "h264;hevc",
#else
	.encoded_video_codecs = "h264",
#endif
	.encoded_audio_codecs = "aac;opus",
	.get_name = ffmpeg_mpegts_getname,
	.create = ffmpeg_mpegts_create,
	.destroy = ffmpeg_mpegts_destroy,
	.start = ffmpeg_mpegts_start,
	.stop = ffmpeg_mpegts_stop,
	.encoded_packet = ffmpeg_mpegts_data,
	.get_total_bytes = ffmpeg_mpegts_total_bytes,
	.get_properties = ffmpeg_mpegts_properties,
};
