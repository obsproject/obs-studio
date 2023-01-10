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

#include <obs-module.h>
#include <util/circlebuf.h>
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
#define do_log(level, format, ...)                              \
	blog(level, "[obs-ffmpeg mpegts muxer: '%s']: " format, \
	     obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)

static void ffmpeg_mpegts_set_last_error(struct ffmpeg_data *data,
					 const char *error)
{
	if (data->last_error)
		bfree(data->last_error);

	data->last_error = bstrdup(error);
}

void ffmpeg_mpegts_log_error(int log_level, struct ffmpeg_data *data,
			     const char *format, ...)
{
	va_list args;
	char out[4096];

	va_start(args, format);
	vsnprintf(out, sizeof(out), format, args);
	va_end(args);

	ffmpeg_mpegts_set_last_error(data, out);

	blog(log_level, "%s", out);
}

static bool is_rist(struct ffmpeg_output *stream)
{
	return !strncmp(stream->ff_data.config.url, RIST_PROTO,
			sizeof(RIST_PROTO) - 1);
}

static bool is_srt(struct ffmpeg_output *stream)
{
	return !strncmp(stream->ff_data.config.url, SRT_PROTO,
			sizeof(SRT_PROTO) - 1);
}

static bool proto_is_allowed(struct ffmpeg_output *stream)
{
	return !strncmp(stream->ff_data.config.url, UDP_PROTO,
			sizeof(UDP_PROTO) - 1) ||
	       !strncmp(stream->ff_data.config.url, TCP_PROTO,
			sizeof(TCP_PROTO) - 1) ||
	       !strncmp(stream->ff_data.config.url, HTTP_PROTO,
			sizeof(HTTP_PROTO) - 1);
}

static bool new_stream(struct ffmpeg_data *data, AVStream **stream,
		       const char *name)
{

	*stream = avformat_new_stream(data->output, NULL);
	if (!*stream) {
		ffmpeg_mpegts_log_error(
			LOG_WARNING, data,
			"Couldn't create stream for encoder '%s'", name);
		return false;
	}

	(*stream)->id = data->output->nb_streams - 1;
	return true;
}

static bool get_audio_headers(struct ffmpeg_output *stream,
			      struct ffmpeg_data *data, int idx)
{
	AVCodecParameters *par = data->audio_infos[idx].stream->codecpar;
	obs_encoder_t *aencoder =
		obs_output_get_audio_encoder(stream->output, idx);
	struct encoder_packet packet = {
		.type = OBS_ENCODER_AUDIO, .timebase_den = 1, .track_idx = idx};

	if (obs_encoder_get_extra_data(aencoder, &packet.data, &packet.size)) {
		par->extradata = av_memdup(packet.data, packet.size);
		par->extradata_size = (int)packet.size;
		avcodec_parameters_to_context(data->audio_infos[idx].ctx, par);
		return 1;
	}
	return 0;
}

static bool get_video_headers(struct ffmpeg_output *stream,
			      struct ffmpeg_data *data)
{
	AVCodecParameters *par = data->video->codecpar;
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	struct encoder_packet packet = {.type = OBS_ENCODER_VIDEO,
					.timebase_den = 1};

	if (obs_encoder_get_extra_data(vencoder, &packet.data, &packet.size)) {
		par->extradata = av_memdup(packet.data, packet.size);
		par->extradata_size = (int)packet.size;
		avcodec_parameters_to_context(data->video_ctx,
					      data->video->codecpar);
		return 1;
	}
	return 0;
}

static bool create_video_stream(struct ffmpeg_output *stream,
				struct ffmpeg_data *data)
{
	AVCodecContext *context;
	void *extradata = NULL;
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
	const bool pq = data->config.color_trc == AVCOL_TRC_SMPTE2084;
	const bool hlg = data->config.color_trc == AVCOL_TRC_ARIB_STD_B67;
	if (pq || hlg) {
		const int hdr_nominal_peak_level =
			pq ? (int)obs_get_video_hdr_nominal_peak_level()
			   : (hlg ? 1000 : 0);

		size_t content_size;
		AVContentLightMetadata *const content =
			av_content_light_metadata_alloc(&content_size);
		content->MaxCLL = hdr_nominal_peak_level;
		content->MaxFALL = hdr_nominal_peak_level;
		av_stream_add_side_data(data->video,
					AV_PKT_DATA_CONTENT_LIGHT_LEVEL,
					(uint8_t *)content, content_size);

		AVMasteringDisplayMetadata *const mastering =
			av_mastering_display_metadata_alloc();
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
		av_stream_add_side_data(data->video,
					AV_PKT_DATA_MASTERING_DISPLAY_METADATA,
					(uint8_t *)mastering,
					sizeof(*mastering));
	}
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
	context->chroma_sample_location = determine_chroma_location(
		data->config.format, data->config.colorspace);
	context->thread_count = 0;

	data->video->time_base = context->time_base;
#if LIBAVFORMAT_VERSION_MAJOR < 59
	data->video->codec->time_base = context->time_base;
#endif
	data->video->avg_frame_rate = av_inv_q(context->time_base);

	data->video_ctx = context;
	data->config.width = data->config.scale_width;
	data->config.height = data->config.scale_height;

	avcodec_parameters_from_context(data->video->codecpar, context);

	return true;
}

static bool create_audio_stream(struct ffmpeg_output *stream,
				struct ffmpeg_data *data, int idx)
{
	AVCodecContext *context;
	AVStream *avstream;
	void *extradata = NULL;
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
	context->bit_rate = (int64_t)data->config.audio_bitrate * 1000;
	context->time_base = (AVRational){1, aoi.samples_per_sec};
	channels = get_audio_channels(aoi.speakers);
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(57, 24, 100)
	context->channels = get_audio_channels(aoi.speakers);
#endif
	context->sample_rate = aoi.samples_per_sec;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(59, 24, 100)
	context->channel_layout =
		av_get_default_channel_layout(context->channels);

	//avutil default channel layout for 5 channels is 5.0 ; fix for 4.1
	if (aoi.speakers == SPEAKERS_4POINT1)
		context->channel_layout = av_get_channel_layout("4.1");
#else
	av_channel_layout_default(&context->ch_layout, channels);
	if (aoi.speakers == SPEAKERS_4POINT1)
		context->ch_layout = (AVChannelLayout)AV_CHANNEL_LAYOUT_4POINT1;
#endif

	context->sample_fmt = AV_SAMPLE_FMT_S16;
	context->frame_size = data->config.frame_size;

	avstream->time_base = context->time_base;

	data->audio_samplerate = aoi.samples_per_sec;
	data->audio_format = convert_ffmpeg_sample_format(context->sample_fmt);
	data->audio_planes = get_audio_planes(data->audio_format, aoi.speakers);
	data->audio_size = get_audio_size(data->audio_format, aoi.speakers, 1);

	data->audio_infos[idx].stream = avstream;
	data->audio_infos[idx].ctx = context;
	avcodec_parameters_from_context(data->audio_infos[idx].stream->codecpar,
					context);
	return true;
}

static inline bool init_streams(struct ffmpeg_output *stream,
				struct ffmpeg_data *data)
{
	if (!create_video_stream(stream, data))
		return false;

	if (data->num_audio_streams) {
		data->audio_infos = calloc(data->num_audio_streams,
					   sizeof(*data->audio_infos));
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
		ffmpeg_mpegts_log_error(LOG_ERROR, &stream->ff_data,
					"Couldn't initialize network");
		return AVERROR(EIO);
	}

	URLContext *uc = av_mallocz(sizeof(URLContext) + strlen(url) + 1);
	if (!uc) {
		ffmpeg_mpegts_log_error(LOG_ERROR, &stream->ff_data,
					"Couldn't allocate memory");
		goto fail;
	}
	uc->url = (char *)url;
	uc->max_packet_size = is_rist ? RIST_MAX_PAYLOAD_SIZE
				      : SRT_LIVE_DEFAULT_PAYLOAD_SIZE;
	uc->priv_data = is_rist ? av_mallocz(sizeof(RISTContext))
				: av_mallocz(sizeof(SRTContext));
	if (!uc->priv_data) {
		ffmpeg_mpegts_log_error(LOG_ERROR, &stream->ff_data,
					"Couldn't allocate memory");
		goto fail;
	}
	/* For SRT, pass streamid & passphrase; for RIST, pass passphrase, username
	 * & password.
	 */
	if (!is_rist) {
		SRTContext *context = (SRTContext *)uc->priv_data;
		context->streamid = NULL;
		if (stream->ff_data.config.key != NULL) {
			if (strlen(stream->ff_data.config.key))
				context->streamid =
					av_strdup(stream->ff_data.config.key);
		}
		context->passphrase = NULL;
		if (stream->ff_data.config.password != NULL) {
			if (strlen(stream->ff_data.config.password))
				context->passphrase = av_strdup(
					stream->ff_data.config.password);
		}
	} else {
		RISTContext *context = (RISTContext *)uc->priv_data;
		context->secret = NULL;
		if (stream->ff_data.config.key != NULL) {
			if (strlen(stream->ff_data.config.key))
				context->secret =
					bstrdup(stream->ff_data.config.key);
		}
		context->username = NULL;
		if (stream->ff_data.config.username != NULL) {
			if (strlen(stream->ff_data.config.username))
				context->username = bstrdup(
					stream->ff_data.config.username);
		}
		context->password = NULL;
		if (stream->ff_data.config.password != NULL) {
			if (strlen(stream->ff_data.config.password))
				context->password = bstrdup(
					stream->ff_data.config.password);
		}
	}
	stream->h = uc;
	if (is_rist)
		err = librist_open(uc, uc->url);
	else
		err = libsrt_open(uc, uc->url);

	if (err < 0)
		goto fail;
	return 0;
fail:
	if (uc)
		av_freep(&uc->priv_data);
	av_freep(&uc);
#if HAVE_WINSOCK2_H
	WSACleanup();
#endif
	return err;
}

static inline int allocate_custom_aviocontext(struct ffmpeg_output *stream,
					      bool is_rist)
{
	/* allocate buffers */
	uint8_t *buffer = NULL;
	int buffer_size;
	URLContext *h = stream->h;
	AVIOContext *s = NULL;

	buffer_size = UDP_DEFAULT_PAYLOAD_SIZE;

	buffer = av_malloc(buffer_size);
	if (!buffer)
		return AVERROR(ENOMEM);
	/* allocate custom avio_context */
	if (is_rist)
		s = avio_alloc_context(
			buffer, buffer_size, AVIO_FLAG_WRITE, h, NULL,
			(int (*)(void *, uint8_t *, int))librist_write, NULL);
	else
		s = avio_alloc_context(
			buffer, buffer_size, AVIO_FLAG_WRITE, h, NULL,
			(int (*)(void *, uint8_t *, int))libsrt_write, NULL);
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

static inline int open_output_file(struct ffmpeg_output *stream,
				   struct ffmpeg_data *data)
{
	int ret;
	bool rist = is_rist(stream);
	bool srt = is_srt(stream);
	bool allowed_proto = proto_is_allowed(stream);
	AVDictionary *dict = NULL;

	/* Retrieve protocol settings for udp, tcp, rtp ... (not srt or rist).
	 * These options will be passed to protocol by avio_open2 through dict.
	 * The invalid options will be left in dict. */
	if (!rist && !srt) {
		if ((ret = av_dict_parse_string(&dict,
						data->config.protocol_settings,
						"=", " ", 0))) {
			ffmpeg_mpegts_log_error(
				LOG_WARNING, data,
				"Failed to parse protocol settings: %s, %s",
				data->config.protocol_settings,
				av_err2str(ret));

			av_dict_free(&dict);
			return OBS_OUTPUT_INVALID_STREAM;
		}

		if (av_dict_count(dict) > 0) {
			struct dstr str = {0};

			AVDictionaryEntry *entry = NULL;
			while ((entry = av_dict_get(dict, "", entry,
						    AV_DICT_IGNORE_SUFFIX)))
				dstr_catf(&str, "\n\t%s=%s", entry->key,
					  entry->value);

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
		ret = avio_open2(&data->output->pb, data->config.url,
				 AVIO_FLAG_WRITE, NULL, &dict);
	} else {
		info("[ffmpeg mpegts muxer]: Invalid protocol: %s",
		     data->config.url);
		return OBS_OUTPUT_BAD_PATH;
	}

	if (ret < 0) {
		if ((rist || srt) && (ret == OBS_OUTPUT_CONNECT_FAILED ||
				      ret == OBS_OUTPUT_INVALID_STREAM)) {
			error("Failed to open the url or invalid stream");
		} else {
			ffmpeg_mpegts_log_error(LOG_WARNING, data,
						"Couldn't open '%s', %s",
						data->config.url,
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
			while ((entry = av_dict_get(dict, "", entry,
						    AV_DICT_IGNORE_SUFFIX)))
				dstr_catf(&str, "\n\t%s=%s", entry->key,
					  entry->value);

			info("[ffmpeg mpegts muxer]: Invalid protocol settings: %s",
			     str.array);
			dstr_free(&str);
		}
		av_dict_free(&dict);
	} else {
		ret = allocate_custom_aviocontext(stream, rist);
		if (ret < 0) {
			info("Couldn't allocate custom avio_context for url: '%s', %s",
			     data->config.url, av_err2str(ret));
			return OBS_OUTPUT_INVALID_STREAM;
		}
	}

	return 0;
}

static void close_video(struct ffmpeg_data *data)
{
	avcodec_free_context(&data->video_ctx);
}

static void close_audio(struct ffmpeg_data *data)
{
	for (int idx = 0; idx < data->num_audio_streams; idx++) {
		for (size_t i = 0; i < MAX_AV_PLANES; i++)
			circlebuf_free(&data->excess_frames[idx][i]);

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
	AVIOContext *s = stream->s;
	if (!s)
		return;
	URLContext *h = s->opaque;
	if (!h)
		return; /* can happen when opening the url fails */

	/* close rist or srt URLs ; free URLContext */
	if (is_rist) {
		err = librist_close(h);
	} else {
		err = libsrt_close(h);
	}
	av_freep(&h->priv_data);
	av_freep(h);

	/* close custom avio_context for srt or rist */
	avio_flush(stream->s);
	stream->s->opaque = NULL;
	av_freep(&stream->s->buffer);
	avio_context_free(&stream->s);

	if (err)
		info("[ffmpeg mpegts muxer]: Error closing URL %s",
		     stream->ff_data.config.url);
}

void ffmpeg_mpegts_data_free(struct ffmpeg_output *stream,
			     struct ffmpeg_data *data)
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
		if (is_rist(stream) || is_srt(stream)) {
			close_mpegts_url(stream, is_rist(stream));
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

static inline const char *safe_str(const char *s)
{
	if (s == NULL)
		return "(NULL)";
	else
		return s;
}

bool ffmpeg_mpegts_data_init(struct ffmpeg_output *stream,
			     struct ffmpeg_data *data,
			     struct ffmpeg_cfg *config)
{
	memset(data, 0, sizeof(struct ffmpeg_data));
	data->config = *config;
	data->num_audio_streams = config->audio_mix_count;
	data->audio_tracks = config->audio_tracks;

	if (!config->url || !*config->url)
		return false;

#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(58, 9, 100)
	av_register_all();
#endif
	avformat_network_init();

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(59, 0, 100)
	AVOutputFormat *output_format;
#else
	const AVOutputFormat *output_format;
#endif

	output_format = av_guess_format("mpegts", NULL, "video/M2PT");

	if (output_format == NULL) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data,
					"Couldn't set output format to mpegts");
		goto fail;
	} else {
		info("Output format name and long_name: %s, %s",
		     output_format->name ? output_format->name : "unknown",
		     output_format->long_name ? output_format->long_name
					      : "unknown");
	}

	avformat_alloc_output_context2(&data->output, output_format, NULL,
				       data->config.url);

	if (!data->output) {
		ffmpeg_mpegts_log_error(LOG_WARNING, data,
					"Couldn't create avformat context");
		goto fail;
	}

	return true;

fail:
	warn("ffmpeg_data_init failed");
	return false;
}

/* ------------------------------------------------------------------------- */

static inline bool stopping(struct ffmpeg_output *output)
{
	return os_atomic_load_bool(&output->stopping);
}

static const char *ffmpeg_mpegts_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("FFmpegMpegts");
}

static void ffmpeg_mpegts_log_callback(void *param, int level,
				       const char *format, va_list args)
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

	av_log_set_callback(ffmpeg_mpegts_log_callback);

	UNUSED_PARAMETER(settings);
	return data;

fail:
	pthread_mutex_destroy(&data->write_mutex);
	os_event_destroy(data->stop_event);
	bfree(data);
	return NULL;
}

static void ffmpeg_mpegts_full_stop(void *data);
static void ffmpeg_mpegts_deactivate(struct ffmpeg_output *output);

static void ffmpeg_mpegts_destroy(void *data)
{
	struct ffmpeg_output *output = data;

	if (output) {
		if (output->connecting)
			pthread_join(output->start_thread, NULL);

		ffmpeg_mpegts_full_stop(output);

		pthread_mutex_destroy(&output->write_mutex);
		os_sem_destroy(output->write_sem);
		os_event_destroy(output->stop_event);
		bfree(data);
	}
}

static uint64_t get_packet_sys_dts(struct ffmpeg_output *output,
				   AVPacket *packet)
{
	struct ffmpeg_data *data = &output->ff_data;
	uint64_t pause_offset = obs_output_get_pause_offset(output->output);
	uint64_t start_ts;

	AVRational time_base;

	if (data->video && data->video->index == packet->stream_index) {
		time_base = data->video->time_base;
		start_ts = output->video_start_ts;
	} else {
		time_base = data->audio_infos[0].stream->time_base;
		start_ts = output->audio_start_ts;
	}

	return start_ts + pause_offset +
	       (uint64_t)av_rescale_q(packet->dts, time_base,
				      (AVRational){1, 1000000000});
}

static int mpegts_process_packet(struct ffmpeg_output *output)
{
	AVPacket *packet = NULL;
	int ret = 0;

	pthread_mutex_lock(&output->write_mutex);
	if (output->packets.num) {
		packet = output->packets.array[0];
		da_erase(output->packets, 0);
	}
	pthread_mutex_unlock(&output->write_mutex);

	if (!packet)
		return 0;

	//blog(LOG_DEBUG,
	//     "size = %d, flags = %lX, stream = %d, "
	//     "packets queued: %lu",
	//     packet->size, packet->flags, packet->stream_index,
	//     output->packets.num);

	if (stopping(output)) {
		uint64_t sys_ts = get_packet_sys_dts(output, packet);
		if (sys_ts >= output->stop_ts) {
			ret = 0;
			goto end;
		}
	}
	output->total_bytes += packet->size;
	uint8_t *buf = packet->data;
	ret = av_interleaved_write_frame(output->ff_data.output, packet);
	av_freep(&buf);

	if (ret < 0) {
		ffmpeg_mpegts_log_error(
			LOG_WARNING, &output->ff_data,
			"process_packet: Error writing packet: %s",
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

static void *write_thread(void *data)
{
	struct ffmpeg_output *output = data;

	while (os_sem_wait(output->write_sem) == 0) {
		/* check to see if shutting down */
		if (os_event_try(output->stop_event) == 0)
			break;

		int ret = mpegts_process_packet(output);
		if (ret != 0) {
			int code = OBS_OUTPUT_DISCONNECTED;

			pthread_detach(output->write_thread);
			output->write_thread_active = false;

			if (ret == -ENOSPC)
				code = OBS_OUTPUT_NO_SPACE;

			obs_output_signal_stop(output->output, code);
			ffmpeg_mpegts_deactivate(output);
			break;
		}
	}

	os_atomic_set_bool(&output->active, false);
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

/* set ffmpeg_config & init write_thread & capture */
static bool set_config(struct ffmpeg_output *stream)
{
	struct ffmpeg_cfg config;
	bool success;
	int ret;
	int code;

	/* 1. Get URL/username/password from service & set format + mime-type. */
	obs_service_t *service;
	service = obs_output_get_service(stream->output);
	if (!service)
		return false;
	config.url = obs_service_get_url(service);
	config.username = obs_service_get_username(service);
	config.password = obs_service_get_password(service);
	config.key = obs_service_get_key(service);
	config.format_name = "mpegts";
	config.format_mime_type = "video/M2PT";

	/* 2. video settings */
	// 2.a) set video format from obs to FFmpeg
	video_t *video = obs_output_video(stream->output);
	config.format =
		obs_to_ffmpeg_video_format(video_output_get_format(video));

	if (config.format == AV_PIX_FMT_NONE) {
		blog(LOG_WARNING,
		     "Invalid pixel format used for mpegts output");
		return false;
	}

	// 2.b) set colorspace, color_range & transfer characteristic (from voi)
	const struct video_output_info *voi = video_output_get_info(video);
	config.color_range = voi->range == VIDEO_RANGE_FULL ? AVCOL_RANGE_JPEG
							    : AVCOL_RANGE_MPEG;
	config.colorspace = format_is_yuv(voi->format) ? AVCOL_SPC_BT709
						       : AVCOL_SPC_RGB;
	switch (voi->colorspace) {
	case VIDEO_CS_601:
		config.color_primaries = AVCOL_PRI_SMPTE170M;
		config.color_trc = AVCOL_TRC_SMPTE170M;
		config.colorspace = AVCOL_SPC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		config.color_primaries = AVCOL_PRI_BT709;
		config.color_trc = AVCOL_TRC_BT709;
		config.colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_SRGB:
		config.color_primaries = AVCOL_PRI_BT709;
		config.color_trc = AVCOL_TRC_IEC61966_2_1;
		config.colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_2100_PQ:
		config.color_primaries = AVCOL_PRI_BT2020;
		config.color_trc = AVCOL_TRC_SMPTE2084;
		config.colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	case VIDEO_CS_2100_HLG:
		config.color_primaries = AVCOL_PRI_BT2020;
		config.color_trc = AVCOL_TRC_ARIB_STD_B67;
		config.colorspace = AVCOL_SPC_BT2020_NCL;
	}

	// 2.c) set width & height
	config.width = (int)obs_output_get_width(stream->output);
	config.height = (int)obs_output_get_height(stream->output);
	config.scale_width = config.width;
	config.scale_height = config.height;

	// 2.d) set video codec & id from video encoder
	obs_encoder_t *vencoder = obs_output_get_video_encoder(stream->output);
	config.video_encoder = obs_encoder_get_codec(vencoder);
	if (strcmp(config.video_encoder, "h264") == 0)
		config.video_encoder_id = AV_CODEC_ID_H264;
	else
		config.video_encoder_id = AV_CODEC_ID_AV1;

	// 2.e)  set video bitrate & gop through video encoder settings
	obs_data_t *settings = obs_encoder_get_settings(vencoder);
	config.video_bitrate = (int)obs_data_get_int(settings, "bitrate");
	int keyint_sec = (int)obs_data_get_int(settings, "keyint_sec");
	config.gop_size = keyint_sec ? keyint_sec * voi->fps_num / voi->fps_den
				     : 250;
	obs_data_release(settings);

	/* 3. Audio settings */
	// 3.a) set audio encoder and id to aac
	obs_encoder_t *aencoder =
		obs_output_get_audio_encoder(stream->output, 0);
	config.audio_encoder = "aac";
	config.audio_encoder_id = AV_CODEC_ID_AAC;

	// 3.b) get audio bitrate from the audio encoder.
	settings = obs_encoder_get_settings(aencoder);
	config.audio_bitrate = (int)obs_data_get_int(settings, "bitrate");
	obs_data_release(settings);

	// 3.c set audio frame size
	config.frame_size = (int)obs_encoder_get_frame_size(aencoder);

	// 3.d) set the number of tracks
	// The UI for multiple tracks is not written for streaming outputs.
	// When it is, modify write_packet & uncomment :
	// config.audio_tracks = (int)obs_output_get_mixers(stream->output);
	// config.audio_mix_count = get_audio_mix_count(config.audio_tracks);
	config.audio_tracks = 1;
	config.audio_mix_count = 1;

	/* 4. Muxer & protocol settings */
	// This requires some UI to be written for the output.
	// at the service level unless one can load the output in the settings/stream screen.
	settings = obs_output_get_settings(stream->output);
	obs_data_set_default_string(settings, "muxer_settings", "");
	config.muxer_settings = obs_data_get_string(settings, "muxer_settings");
	obs_data_release(settings);
	config.protocol_settings = "";

	/* 5. unused ffmpeg codec settings */
	config.video_settings = "";
	config.audio_settings = "";

	success = ffmpeg_mpegts_data_init(stream, &stream->ff_data, &config);
	if (!success) {
		if (stream->ff_data.last_error) {
			obs_output_set_last_error(stream->output,
						  stream->ff_data.last_error);
		}
		ffmpeg_mpegts_data_free(stream, &stream->ff_data);
		code = OBS_OUTPUT_INVALID_STREAM;
		goto fail;
	}
	struct ffmpeg_data *ff_data = &stream->ff_data;
	if (!stream->got_headers) {
		if (!init_streams(stream, ff_data)) {
			error("mpegts avstream failed to be created");
			code = OBS_OUTPUT_INVALID_STREAM;
			goto fail;
		}
		code = open_output_file(stream, ff_data);
		if (code != 0) {
			error("Failed to open the url");
			goto fail;
		}
		av_dump_format(ff_data->output, 0, NULL, 1);
	}
	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;
	if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	ret = pthread_create(&stream->write_thread, NULL, write_thread, stream);
	if (ret != 0) {
		ffmpeg_mpegts_log_error(
			LOG_WARNING, &stream->ff_data,
			"ffmpeg_output_start: Failed to create write "
			"thread.");
		code = OBS_OUTPUT_ERROR;
		goto fail;
	}
	os_atomic_set_bool(&stream->active, true);
	stream->write_thread_active = true;
	stream->total_bytes = 0;
	obs_output_begin_data_capture(stream->output, 0);

	return true;
fail:
	obs_output_signal_stop(stream->output, code);
	ffmpeg_mpegts_full_stop(stream);
	return false;
}

static void *start_thread(void *data)
{
	struct ffmpeg_output *output = data;
	set_config(output);
	output->connecting = false;
	return NULL;
}

static bool ffmpeg_mpegts_start(void *data)
{
	struct ffmpeg_output *output = data;
	int ret;

	if (output->connecting)
		return false;

	os_atomic_set_bool(&output->stopping, false);
	output->audio_start_ts = 0;
	output->video_start_ts = 0;
	output->total_bytes = 0;
	output->got_headers = false;

	ret = pthread_create(&output->start_thread, NULL, start_thread, output);
	return (output->connecting = (ret == 0));
}

static void ffmpeg_mpegts_full_stop(void *data)
{
	struct ffmpeg_output *output = data;

	if (output->active) {
		obs_output_end_data_capture(output->output);
		ffmpeg_mpegts_deactivate(output);
	}
}

static void ffmpeg_mpegts_stop(void *data, uint64_t ts)
{
	struct ffmpeg_output *output = data;

	if (output->active) {
		if (ts > 0) {
			output->stop_ts = ts;
			os_atomic_set_bool(&output->stopping, true);
		}

		ffmpeg_mpegts_full_stop(output);
	} else {
		obs_output_signal_stop(output->output, OBS_OUTPUT_SUCCESS);
	}
}

static void ffmpeg_mpegts_deactivate(struct ffmpeg_output *output)
{
	if (output->write_thread_active) {
		os_event_signal(output->stop_event);
		os_sem_post(output->write_sem);
		pthread_join(output->write_thread, NULL);
		output->write_thread_active = false;
	}

	pthread_mutex_lock(&output->write_mutex);

	for (size_t i = 0; i < output->packets.num; i++)
		av_packet_free(output->packets.array + i);
	da_free(output->packets);

	pthread_mutex_unlock(&output->write_mutex);

	ffmpeg_mpegts_data_free(output, &output->ff_data);
}

static uint64_t ffmpeg_mpegts_total_bytes(void *data)
{
	struct ffmpeg_output *output = data;
	return output->total_bytes;
}

static inline int64_t rescale_ts2(AVStream *stream, AVRational codec_time_base,
				  int64_t val)
{
	return av_rescale_q_rnd(val / codec_time_base.num, codec_time_base,
				stream->time_base,
				AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
}

/* Convert obs encoder_packet to FFmpeg AVPacket and write to circular buffer
 * where it will be processed in the write_thread by process_packet.
 */
void mpegts_write_packet(struct ffmpeg_output *stream,
			 struct encoder_packet *encpacket)
{
	if (stopping(stream) || !stream->ff_data.video ||
	    !stream->ff_data.video_ctx || !stream->ff_data.audio_infos)
		return;
	if (!stream->ff_data.audio_infos[encpacket->track_idx].stream)
		return;
	bool is_video = encpacket->type == OBS_ENCODER_VIDEO;
	AVStream *avstream =
		is_video ? stream->ff_data.video
			 : stream->ff_data.audio_infos[encpacket->track_idx]
				   .stream;
	AVPacket *packet = NULL;

	const AVRational codec_time_base =
		is_video ? stream->ff_data.video_ctx->time_base
			 : stream->ff_data.audio_infos[encpacket->track_idx]
				   .ctx->time_base;

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
	if ((ret = av_dict_parse_string(&dict, data->config.muxer_settings, "=",
					" ", 0))) {
		ffmpeg_mpegts_log_error(
			LOG_WARNING, data,
			"Failed to parse muxer settings: %s, %s",
			data->config.muxer_settings, av_err2str(ret));

		av_dict_free(&dict);
		return false;
	}

	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry,
					    AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		info("Using muxer settings: %s", str.array);
		dstr_free(&str);
	}

	/* Allocate the stream private data and write the stream header. */
	ret = avformat_write_header(data->output, &dict);
	if (ret < 0) {
		ffmpeg_mpegts_log_error(
			LOG_WARNING, data,
			"Error setting stream header for '%s': %s",
			data->config.url, av_err2str(ret));
		return false;
	}

	/* Log invalid muxer settings. */
	if (av_dict_count(dict) > 0) {
		struct dstr str = {0};

		AVDictionaryEntry *entry = NULL;
		while ((entry = av_dict_get(dict, "", entry,
					    AV_DICT_IGNORE_SUFFIX)))
			dstr_catf(&str, "\n\t%s=%s", entry->key, entry->value);

		info("[ffmpeg mpegts muxer]: Invalid mpegts muxer settings: %s",
		     str.array);
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

	if (!stream->active)
		return;

	/* encoder failure */
	if (!packet) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_ENCODE_ERROR);
		ffmpeg_mpegts_deactivate(stream);
		return;
	}

	if (stopping(stream)) {
		if (packet->sys_dts_usec >= (int64_t)stream->stop_ts) {
			ffmpeg_mpegts_deactivate(stream);
			return;
		}
	}

	mpegts_write_packet(stream, packet);
	return;
fail:
	obs_output_signal_stop(stream->output, code);
	ffmpeg_mpegts_full_stop(stream);
}

static obs_properties_t *ffmpeg_mpegts_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_text(props, "path", obs_module_text("FilePath"),
				OBS_TEXT_DEFAULT);
	return props;
}

struct obs_output_info ffmpeg_mpegts_muxer = {
	.id = "ffmpeg_mpegts_muxer",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK |
		 OBS_OUTPUT_SERVICE,
#ifdef ENABLE_HEVC
	.encoded_video_codecs = "h264;hevc;av1",
#else
	.encoded_video_codecs = "h264;av1",
#endif
	.encoded_audio_codecs = "aac",
	.get_name = ffmpeg_mpegts_getname,
	.create = ffmpeg_mpegts_create,
	.destroy = ffmpeg_mpegts_destroy,
	.start = ffmpeg_mpegts_start,
	.stop = ffmpeg_mpegts_stop,
	.encoded_packet = ffmpeg_mpegts_data,
	.get_total_bytes = ffmpeg_mpegts_total_bytes,
	.get_properties = ffmpeg_mpegts_properties,
};
