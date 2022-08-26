#include <obs.h>
#include <graphics/graphics.h>
#include <util/threading.h>
#include <util/platform.h>

#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

#include "webrtc-call.h"
#include "./webrtc/bindings.h"

OBSWebRTCCall *call;

pthread_t thread;

AVFrame *frame;
AVFrame *audioFrame;
AVFrame *dst;

struct OBSCallParticipant {
	AVCodecContext *videoContext;
	AVCodecContext *audioContext;
};

void packet_callback(const void *user_data, const uint8_t *packet_data,
		     unsigned int length, unsigned int video)
{

	//blog(LOG_INFO, "%d", length);
	struct OBSCallParticipant *participant =
		(struct OBSCallParticipant *)user_data;

	AVCodecContext *context = NULL;
	if (video) {
		context = participant->videoContext;
	} else {
		context = participant->audioContext;
	}

	uint8_t *unowned_packet = bzalloc(sizeof(uint8_t) * length);
	memcpy(unowned_packet, packet_data, length);

	AVPacket *packet = av_packet_alloc();
	av_packet_from_data(packet, unowned_packet, length);
	avcodec_send_packet(context, packet);

	if (video) {
		int res = avcodec_receive_frame(context, frame);

		if (res == 0) {

			/*blog(LOG_INFO, "Width: %d, Height: %d, Format: %s",
	     frame->width, frame->height,
	     av_get_pix_fmt_name(frame->format));*/

			dst->width = frame->width;
			dst->height = frame->height;
			dst->format = AV_PIX_FMT_BGRA;
			av_image_alloc(dst->data, dst->linesize, frame->width,
				       frame->height, AV_PIX_FMT_BGRA, 1);
			struct SwsContext *swsContext = sws_getContext(
				frame->width, frame->height, frame->format,
				frame->width, frame->height, AV_PIX_FMT_BGRA,
				SWS_BILINEAR, NULL, NULL, NULL);
			int out = sws_scale(swsContext,
					    (const uint8_t *const *)frame->data,
					    frame->linesize, 0, frame->height,
					    dst->data, dst->linesize);
			sws_freeContext(swsContext);

			//blog(LOG_INFO, "out %d", out);

			//blog(LOG_INFO, "Width: %d, Height: %d, Format: %s", dst->width, dst->height, av_get_pix_fmt_name(dst->format));

			obs_enter_graphics();

			webrtcTexture = gs_texture_create(
				dst->width, dst->height, GS_BGRA, 1,
				(const uint8_t **)&dst->data, GS_DYNAMIC);

			//gs_texture_set_image(webrtcTexture, (const uint8_t **)&frame->data, frame->linesize[0], false);

			obs_leave_graphics();
		}
	} else {
		int res = avcodec_receive_frame(context, audioFrame);

		if (res == 0) {
			//blog(LOG_INFO, "channels: %d", audioFrame->channels);
		}
	}
}

void setup_call()
{
	obs_enter_graphics();
	webrtcTexture =
		gs_texture_create(1920, 1080, GS_BGRA, 1, NULL, GS_DYNAMIC);
	obs_leave_graphics();

	const AVCodec *codec = avcodec_find_decoder(AV_CODEC_ID_H264);

	AVDictionary *opts = NULL;
	//av_dict_set(&opts, "b", "2.5M", 0);
	if (!codec)
		exit(1);
	AVCodecContext *context = avcodec_alloc_context3(codec);
	if (avcodec_open2(context, codec, &opts) < 0)
		exit(1);

	/*AVCodecParameters* codecpar = avcodec_parameters_alloc();
	codecpar->codec_id = AV_CODEC_ID_H264;
	codecpar->height = 720;
	codecpar->width = 1280;*/

	frame = av_frame_alloc();
	audioFrame = av_frame_alloc();
	dst = av_frame_alloc();

	const AVCodec *opusCodec = avcodec_find_decoder(AV_CODEC_ID_OPUS);
	AVCodecContext *audioContext = avcodec_alloc_context3(opusCodec);
	if (avcodec_open2(audioContext, opusCodec, NULL) < 0)
		exit(1);

	//codecpar->

	//avcodec_parameters_to_context(context, codecpar);

	struct OBSCallParticipant *callParticipant =
		malloc(sizeof(struct OBSCallParticipant));
	callParticipant->videoContext = context;
	callParticipant->audioContext = audioContext;

	call = obs_webrtc_call_init(callParticipant, packet_callback);

	/*if (pthread_create(&thread, NULL, video_thread, NULL) != 0) {
		blog(LOG_ERROR, "Failed to make render thread of webrtc call");
	}else {
		blog(LOG_ERROR, "Thread running");
	}*/
}

void start_call()
{
	obs_webrtc_call_start(call);
}
