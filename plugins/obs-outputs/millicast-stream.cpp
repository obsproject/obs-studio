#include "millicast-stream.h"

extern "C" const char *millicast_stream_getname(void *unused)
{
	info("millicast_stream_getname");
	UNUSED_PARAMETER(unused);
	return obs_module_text("MILLICASTStream");
}

extern "C" void millicast_stream_destroy(void *data)
{
	info("millicast_stream_destroy");
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Stop it
	stream->stop();
	//Remove ref and let it self destroy
	stream->Release();
}

extern "C" void *millicast_stream_create(obs_data_t *, obs_output_t *output)
{
	info("millicast_stream_create");
	// Create new stream
	WebRTCStream *stream = new WebRTCStream(output);
	// Don't allow it to be deleted
	stream->AddRef();
	// info("millicast_setCodec: h264");
	// stream->setCodec("h264");
	// Return it
	return (void *)stream;
}

extern "C" void millicast_stream_stop(void *data, uint64_t ts)
{
	info("millicast_stream_stop");
	UNUSED_PARAMETER(ts);
	// Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	// Stop it
	stream->stop();
	// Remove ref and let it self destroy
	stream->Release();
}

extern "C" bool millicast_stream_start(void *data)
{
	info("millicast_stream_start");
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Don't allow it to be deleted
	stream->AddRef();
	//Start it
	return stream->start(WebRTCStream::Type::Millicast);
}

extern "C" void millicast_receive_video(void *data, struct video_data *frame)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Process audio
	stream->onVideoFrame(frame);
}
extern "C" void millicast_receive_audio(void *data, struct audio_data *frame)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	//Process audio
	stream->onAudioFrame(frame);
}

extern "C" void millicast_stream_defaults(obs_data_t *defaults)
{
	info("millicast_stream_defaults");
	obs_data_set_default_int(defaults, OPT_DROP_THRESHOLD, 700);
	obs_data_set_default_int(defaults, OPT_PFRAME_DROP_THRESHOLD, 900);
	obs_data_set_default_int(defaults, OPT_MAX_SHUTDOWN_TIME_SEC, 30);
	obs_data_set_default_string(defaults, OPT_BIND_IP, "default");
	obs_data_set_default_bool(defaults, OPT_NEWSOCKETLOOP_ENABLED, false);
	obs_data_set_default_bool(defaults, OPT_LOWLATENCY_ENABLED, false);
}

extern "C" obs_properties_t *millicast_stream_properties(void *unused)
{
	info("millicast_stream_properties");
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_int(props, OPT_DROP_THRESHOLD,
			       obs_module_text("MILLICASTStream.DropThreshold"),
			       200, 10000, 100);

	obs_properties_add_bool(
		props, OPT_NEWSOCKETLOOP_ENABLED,
		obs_module_text("MILLICASTStream.NewSocketLoop"));
	obs_properties_add_bool(
		props, OPT_LOWLATENCY_ENABLED,
		obs_module_text("MILLICASTStream.LowLatencyMode"));

	return props;
}

extern "C" uint64_t millicast_stream_total_bytes_sent(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getBitrate();
}

extern "C" int millicast_stream_dropped_frames(void *data)
{
	//Get stream
	WebRTCStream *stream = (WebRTCStream *)data;
	return stream->getDroppedFrames();
}

extern "C" float millicast_stream_congestion(void *data)
{
	UNUSED_PARAMETER(data);
	return 0.0f;
}

extern "C" {
#ifdef _WIN32
struct obs_output_info millicast_output_info = {
	"millicast_output",                 //id
	OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE, //flags
	millicast_stream_getname,           //get_name
	millicast_stream_create,            //create
	millicast_stream_destroy,           //destroy
	millicast_stream_start,             //start
	millicast_stream_stop,              //stop
	millicast_receive_video,            //raw_video
	millicast_receive_audio,            //raw_audio
	nullptr,                            //encoded_packet
	nullptr,                            //update
	millicast_stream_defaults,          //get_defaults
	millicast_stream_properties,        //get_properties
	nullptr,                            //unused1 (formerly pause)
	millicast_stream_total_bytes_sent,  //get_total_bytes
	millicast_stream_dropped_frames,    //get_dropped_frames
	nullptr,                            //type_data
	nullptr,                            //free_type_data
	millicast_stream_congestion,        //get_congestion
	nullptr,                            //get_connect_time_ms
	"vp8",                              //encoded_video_codecs
	"opus",                             //encoded_audio_codecs
	nullptr                             //raw_audio2
};
#else
struct obs_output_info millicast_output_info = {
	.id = "millicast_output",
	.flags = OBS_OUTPUT_AV | OBS_OUTPUT_SERVICE,
	.get_name = millicast_stream_getname,
	.create = millicast_stream_create,
	.destroy = millicast_stream_destroy,
	.start = millicast_stream_start,
	.stop = millicast_stream_stop,
	.raw_video = millicast_receive_video,
	.raw_audio = millicast_receive_audio, //for single-track
	.encoded_packet = nullptr,
	.update = nullptr,
	.get_defaults = millicast_stream_defaults,
	.get_properties = millicast_stream_properties,
	.unused1 = nullptr,
	.get_total_bytes = millicast_stream_total_bytes_sent,
	.get_dropped_frames = millicast_stream_dropped_frames,
	.type_data = nullptr,
	.free_type_data = nullptr,
	.get_congestion = millicast_stream_congestion,
	.get_connect_time_ms = nullptr,
	.encoded_video_codecs = "vp8",
	.encoded_audio_codecs = "opus",
	.raw_audio2 = nullptr
	// .raw_audio2           = millicast_receive_multitrack_audio, //for multi-track
};
#endif
}
