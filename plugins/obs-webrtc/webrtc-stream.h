#include "obs-module.h"
#include "./webrtc/bindings.h"

struct webrtc_stream {
	obs_output_t *output;

	int64_t audio_timestamp;
	int64_t video_timestamp;

	OBSWebRTCStream *obsrtc;
};