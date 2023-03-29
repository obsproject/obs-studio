#include "obs-ffmpeg-video-encoders.h"

#define do_log(level, format, ...)                               \
	blog(level, "[%s encoder: '%s'] " format, enc->enc_name, \
	     obs_encoder_get_name(enc->encoder), ##__VA_ARGS__)

#define error(format, ...) do_log(LOG_ERROR, format, ##__VA_ARGS__)
#define warn(format, ...) do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...) do_log(LOG_INFO, format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG, format, ##__VA_ARGS__)

bool ffmpeg_video_encoder_init_codec(struct ffmpeg_video_encoder *enc)
{
	int ret = avcodec_open2(enc->context, enc->avcodec, NULL);
	if (ret < 0) {
		if (!obs_encoder_get_last_error(enc->encoder)) {
			if (enc->on_init_error) {
				enc->on_init_error(enc->parent, ret);

			} else {
				struct dstr error_message = {0};

				dstr_copy(&error_message,
					  obs_module_text("Encoder.Error"));
				dstr_replace(&error_message, "%1",
					     enc->enc_name);
				dstr_replace(&error_message, "%2",
					     av_err2str(ret));
				dstr_cat(&error_message, "\r\n\r\n");

				obs_encoder_set_last_error(enc->encoder,
							   error_message.array);
				dstr_free(&error_message);
			}
		}
		return false;
	}

	enc->vframe = av_frame_alloc();
	if (!enc->vframe) {
		warn("Failed to allocate video frame");
		return false;
	}

	enc->vframe->format = enc->context->pix_fmt;
	enc->vframe->width = enc->context->width;
	enc->vframe->height = enc->context->height;
	enc->vframe->color_range = enc->context->color_range;
	enc->vframe->color_primaries = enc->context->color_primaries;
	enc->vframe->color_trc = enc->context->color_trc;
	enc->vframe->colorspace = enc->context->colorspace;
	enc->vframe->chroma_location = enc->context->chroma_sample_location;

	ret = av_frame_get_buffer(enc->vframe, base_get_alignment());
	if (ret < 0) {
		warn("Failed to allocate vframe: %s", av_err2str(ret));
		return false;
	}

	enc->initialized = true;
	return true;
}

void ffmpeg_video_encoder_update(struct ffmpeg_video_encoder *enc, int bitrate,
				 int keyint_sec,
				 const struct video_output_info *voi,
				 const struct video_scale_info *info,
				 const char *ffmpeg_opts)
{
	const int rate = bitrate * 1000;
	const enum AVPixelFormat pix_fmt =
		obs_to_ffmpeg_video_format(info->format);
	enc->context->bit_rate = rate;
	enc->context->rc_buffer_size = rate;
	enc->context->width = obs_encoder_get_width(enc->encoder);
	enc->context->height = obs_encoder_get_height(enc->encoder);
	enc->context->time_base = (AVRational){voi->fps_den, voi->fps_num};
	enc->context->pix_fmt = pix_fmt;
	enc->context->color_range = info->range == VIDEO_RANGE_FULL
					    ? AVCOL_RANGE_JPEG
					    : AVCOL_RANGE_MPEG;

	enum AVColorSpace colorspace = AVCOL_SPC_UNSPECIFIED;
	switch (info->colorspace) {
	case VIDEO_CS_601:
		enc->context->color_primaries = AVCOL_PRI_SMPTE170M;
		enc->context->color_trc = AVCOL_TRC_SMPTE170M;
		colorspace = AVCOL_SPC_SMPTE170M;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:
		enc->context->color_primaries = AVCOL_PRI_BT709;
		enc->context->color_trc = AVCOL_TRC_BT709;
		colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_SRGB:
		enc->context->color_primaries = AVCOL_PRI_BT709;
		enc->context->color_trc = AVCOL_TRC_IEC61966_2_1;
		colorspace = AVCOL_SPC_BT709;
		break;
	case VIDEO_CS_2100_PQ:
		enc->context->color_primaries = AVCOL_PRI_BT2020;
		enc->context->color_trc = AVCOL_TRC_SMPTE2084;
		colorspace = AVCOL_SPC_BT2020_NCL;
		break;
	case VIDEO_CS_2100_HLG:
		enc->context->color_primaries = AVCOL_PRI_BT2020;
		enc->context->color_trc = AVCOL_TRC_ARIB_STD_B67;
		colorspace = AVCOL_SPC_BT2020_NCL;
	}

	enc->context->colorspace = colorspace;
	enc->context->chroma_sample_location =
		determine_chroma_location(pix_fmt, colorspace);

	if (keyint_sec)
		enc->context->gop_size =
			keyint_sec * voi->fps_num / voi->fps_den;

	enc->height = enc->context->height;

	struct obs_options opts = obs_parse_options(ffmpeg_opts);
	for (size_t i = 0; i < opts.count; i++) {
		struct obs_option *opt = &opts.options[i];
		av_opt_set(enc->context->priv_data, opt->name, opt->value, 0);
	}
	obs_free_options(opts);
}

void ffmpeg_video_encoder_free(struct ffmpeg_video_encoder *enc)
{
	if (enc->initialized) {
		AVPacket pkt = {0};
		int r_pkt = 1;

		/* flush remaining data */
		avcodec_send_frame(enc->context, NULL);

		while (r_pkt) {
			if (avcodec_receive_packet(enc->context, &pkt) < 0)
				break;

			if (r_pkt)
				av_packet_unref(&pkt);
		}
	}

	avcodec_free_context(&enc->context);
	av_frame_unref(enc->vframe);
	av_frame_free(&enc->vframe);
	da_free(enc->buffer);
}

bool ffmpeg_video_encoder_init(struct ffmpeg_video_encoder *enc, void *parent,
			       obs_encoder_t *encoder, const char *enc_lib,
			       const char *enc_lib2, const char *enc_name,
			       init_error_cb on_init_error,
			       first_packet_cb on_first_packet)
{
	enc->encoder = encoder;
	enc->parent = parent;
	enc->avcodec = avcodec_find_encoder_by_name(enc_lib);
	if (!enc->avcodec && enc_lib2)
		enc->avcodec = avcodec_find_encoder_by_name(enc_lib2);
	enc->enc_name = enc_name;
	enc->on_init_error = on_init_error;
	enc->on_first_packet = on_first_packet;
	enc->first_packet = true;

	blog(LOG_INFO, "---------------------------------");

	if (!enc->avcodec) {
		struct dstr error_message;
		dstr_printf(&error_message, "Couldn't find encoder: %s",
			    enc_lib);
		obs_encoder_set_last_error(encoder, error_message.array);
		dstr_free(&error_message);

		warn("Couldn't find encoder: '%s'", enc_lib);
		return false;
	}

	enc->context = avcodec_alloc_context3(enc->avcodec);
	if (!enc->context) {
		warn("Failed to create codec context");
		return false;
	}

	return true;
}

static inline void copy_data(AVFrame *pic, const struct encoder_frame *frame,
			     int height, enum AVPixelFormat format)
{
	int h_chroma_shift, v_chroma_shift;
	av_pix_fmt_get_chroma_sub_sample(format, &h_chroma_shift,
					 &v_chroma_shift);
	for (int plane = 0; plane < MAX_AV_PLANES; plane++) {
		if (!frame->data[plane])
			continue;

		int frame_rowsize = (int)frame->linesize[plane];
		int pic_rowsize = pic->linesize[plane];
		int bytes = frame_rowsize < pic_rowsize ? frame_rowsize
							: pic_rowsize;
		int plane_height = height >> (plane ? v_chroma_shift : 0);

		for (int y = 0; y < plane_height; y++) {
			int pos_frame = y * frame_rowsize;
			int pos_pic = y * pic_rowsize;

			memcpy(pic->data[plane] + pos_pic,
			       frame->data[plane] + pos_frame, bytes);
		}
	}
}

#define SEC_TO_NSEC 1000000000LL
#define TIMEOUT_MAX_SEC 5
#define TIMEOUT_MAX_NSEC (TIMEOUT_MAX_SEC * SEC_TO_NSEC)

bool ffmpeg_video_encode(struct ffmpeg_video_encoder *enc,
			 struct encoder_frame *frame,
			 struct encoder_packet *packet, bool *received_packet)
{
	AVPacket av_pkt = {0};
	bool timeout = false;
	const int64_t cur_ts = (int64_t)os_gettime_ns();
	const int64_t pause_offset =
		(int64_t)obs_encoder_get_pause_offset(enc->encoder);
	int got_packet;
	int ret;

	if (!enc->start_ts)
		enc->start_ts = cur_ts;

	copy_data(enc->vframe, frame, enc->height, enc->context->pix_fmt);

	enc->vframe->pts = frame->pts;
	ret = avcodec_send_frame(enc->context, enc->vframe);
	if (ret == 0)
		ret = avcodec_receive_packet(enc->context, &av_pkt);

	got_packet = (ret == 0);

	if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
		ret = 0;
	if (ret < 0) {
		warn("%s: Error encoding: %s", __func__, av_err2str(ret));
		return false;
	}

	if (got_packet && av_pkt.size) {
		if (enc->on_first_packet && enc->first_packet) {
			enc->on_first_packet(enc->parent, &av_pkt,
					     &enc->buffer.da);
			enc->first_packet = false;
		} else {
			da_copy_array(enc->buffer, av_pkt.data, av_pkt.size);
		}

		packet->pts = av_pkt.pts;
		packet->dts = av_pkt.dts;
		packet->data = enc->buffer.array;
		packet->size = enc->buffer.num;
		packet->type = OBS_ENCODER_VIDEO;
		packet->keyframe = !!(av_pkt.flags & AV_PKT_FLAG_KEY);
		*received_packet = true;

		const int64_t recv_ts_nsec =
			(int64_t)util_mul_div64(
				(uint64_t)av_pkt.pts, (uint64_t)SEC_TO_NSEC,
				(uint64_t)enc->context->time_base.den) +
			enc->start_ts;

#if 0
		debug("cur: %lld, packet: %lld, diff: %lld", cur_ts,
		      recv_ts_nsec, cur_ts - recv_ts_nsec);
#endif
		if ((cur_ts - recv_ts_nsec - pause_offset) > TIMEOUT_MAX_NSEC) {
			char timeout_str[16];
			snprintf(timeout_str, sizeof(timeout_str), "%d",
				 TIMEOUT_MAX_SEC);

			struct dstr error_text = {0};
			dstr_copy(&error_text,
				  obs_module_text("Encoder.Timeout"));
			dstr_replace(&error_text, "%1", enc->enc_name);
			dstr_replace(&error_text, "%2", timeout_str);
			obs_encoder_set_last_error(enc->encoder,
						   error_text.array);
			dstr_free(&error_text);

			error("Encoding queue duration surpassed %d "
			      "seconds, terminating encoder",
			      TIMEOUT_MAX_SEC);
			timeout = true;
		}
	} else {
		*received_packet = false;
	}

	av_packet_unref(&av_pkt);
	return !timeout;
}
