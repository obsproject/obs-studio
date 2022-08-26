#include "webrtc-stream.h"

static const char *webrtc_stream_getname(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("WebRTCStream");
}

static void *webrtc_stream_create(obs_data_t *settings, obs_output_t *output)
{
	struct webrtc_stream *stream = bzalloc(sizeof(struct webrtc_stream));

	stream->output = output;

	stream->obsrtc = obs_webrtc_stream_init("test");

	return stream;
}

static void webrtc_stream_destroy(void *data) {}

static bool webrtc_stream_start(void *data)
{
	struct webrtc_stream *stream = data;

	if (!obs_output_can_begin_data_capture(stream->output, 0)) {
		return false;
	}
	if (!obs_output_initialize_encoders(stream->output, 0)) {
		return false;
	}

	obs_webrtc_stream_connect(stream->obsrtc);

	obs_output_begin_data_capture(stream->output, 0);

	return true;
}

static void webrtc_stream_stop(void *data, uint64_t ts)
{
	struct webrtc_stream *stream = data;

	obs_output_end_data_capture(stream->output);
}

static void webrtc_stream_data(void *data, struct encoder_packet *packet)
{
	struct webrtc_stream *stream = data;

	if (packet->type == OBS_ENCODER_VIDEO) {
		int64_t duration = packet->dts_usec - stream->video_timestamp;
		obs_webrtc_stream_data(stream->obsrtc, packet->data,
				       packet->size, duration);
		stream->video_timestamp = packet->dts_usec;
	}

	if (packet->type == OBS_ENCODER_AUDIO) {
		int64_t duration = packet->dts_usec - stream->audio_timestamp;
		obs_webrtc_stream_audio(stream->obsrtc, packet->data,
					packet->size, duration);
		stream->audio_timestamp = packet->dts_usec;
	}
}

static void webrtc_stream_defaults(obs_data_t *defaults)
{
	/*obs_data_set_default_int(defaults, OPT_DROP_THRESHOLD, 700);
	obs_data_set_default_int(defaults, OPT_PFRAME_DROP_THRESHOLD, 900);
	obs_data_set_default_int(defaults, OPT_MAX_SHUTDOWN_TIME_SEC, 30);
	obs_data_set_default_string(defaults, OPT_BIND_IP, "default");
	obs_data_set_default_bool(defaults, OPT_NEWSOCKETLOOP_ENABLED, false);
	obs_data_set_default_bool(defaults, OPT_LOWLATENCY_ENABLED, false);*/
}

static obs_properties_t *webrtc_stream_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	return props;
}

static uint64_t webrtc_stream_total_bytes_sent(void *data)
{
	//struct rtmp_stream *stream = data;
	return 100;
}

static int webrtc_stream_dropped_frames(void *data)
{
	//struct rtmp_stream *stream = data;
	return 0;
}

static int webrtc_stream_connect_time(void *data)
{
	//struct rtmp_stream *stream = data;
	return 100;
}

struct obs_output_info webrtc_output_info = {
	.id = "webrtc_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_SERVICE |
		 OBS_OUTPUT_MULTI_TRACK,
	.encoded_video_codecs = "h264",
	.encoded_audio_codecs = "opus",
	.get_name = webrtc_stream_getname,
	.create = webrtc_stream_create,
	.destroy = webrtc_stream_destroy,
	.start = webrtc_stream_start,
	.stop = webrtc_stream_stop,
	.encoded_packet = webrtc_stream_data,
	.get_defaults = webrtc_stream_defaults,
	.get_properties = webrtc_stream_properties,
	.get_total_bytes = webrtc_stream_total_bytes_sent,
	.get_connect_time_ms = webrtc_stream_connect_time,
	.get_dropped_frames = webrtc_stream_dropped_frames,
};