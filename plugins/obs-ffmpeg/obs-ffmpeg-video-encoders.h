#pragma once

#include <util/platform.h>
#include <util/darray.h>
#include <util/dstr.h>
#include <util/base.h>
#include <media-io/video-io.h>
#include <opts-parser.h>
#include <obs-module.h>

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavformat/avformat.h>

#include "obs-ffmpeg-formats.h"

typedef void (*init_error_cb)(void *data, int ret);
typedef void (*first_packet_cb)(void *data, AVPacket *pkt, struct darray *out);

struct ffmpeg_video_encoder {
	obs_encoder_t *encoder;
	const char *enc_name;

	const AVCodec *avcodec;
	AVCodecContext *context;
	int64_t start_ts;
	bool first_packet;

	AVFrame *vframe;

	DARRAY(uint8_t) buffer;

	int height;
	bool initialized;

	void *parent;
	init_error_cb on_init_error;
	first_packet_cb on_first_packet;
};

extern bool ffmpeg_video_encoder_init(struct ffmpeg_video_encoder *enc,
				      void *parent, obs_encoder_t *encoder,
				      const char *enc_lib, const char *enc_lib2,
				      const char *enc_name,
				      init_error_cb on_init_error,
				      first_packet_cb on_first_packet);
extern void ffmpeg_video_encoder_free(struct ffmpeg_video_encoder *enc);
extern bool ffmpeg_video_encoder_init_codec(struct ffmpeg_video_encoder *enc);
extern void ffmpeg_video_encoder_update(struct ffmpeg_video_encoder *enc,
					int bitrate, int keyint_sec,
					const struct video_output_info *voi,
					const struct video_scale_info *info,
					const char *ffmpeg_opts);
extern bool ffmpeg_video_encode(struct ffmpeg_video_encoder *enc,
				struct encoder_frame *frame,
				struct encoder_packet *packet,
				bool *received_packet);
