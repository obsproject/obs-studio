/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <util/platform.h>

#include <assert.h>

#include "media-playback.h"
#include "media.h"
#include "closest-format.h"

#include <libavdevice/avdevice.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/imgutils.h>

static int64_t base_sys_ts = 0;

static inline enum video_format convert_pixel_format(int f)
{
	switch (f) {
	case AV_PIX_FMT_NONE:
		return VIDEO_FORMAT_NONE;
	case AV_PIX_FMT_YUV420P:
		return VIDEO_FORMAT_I420;
	case AV_PIX_FMT_YUYV422:
		return VIDEO_FORMAT_YUY2;
	case AV_PIX_FMT_YUV422P:
		return VIDEO_FORMAT_I422;
	case AV_PIX_FMT_YUV422P10LE:
		return VIDEO_FORMAT_I210;
	case AV_PIX_FMT_YUV444P:
		return VIDEO_FORMAT_I444;
	case AV_PIX_FMT_YUV444P12LE:
		return VIDEO_FORMAT_I412;
	case AV_PIX_FMT_UYVY422:
		return VIDEO_FORMAT_UYVY;
	case AV_PIX_FMT_YVYU422:
		return VIDEO_FORMAT_YVYU;
	case AV_PIX_FMT_NV12:
		return VIDEO_FORMAT_NV12;
	case AV_PIX_FMT_RGBA:
		return VIDEO_FORMAT_RGBA;
	case AV_PIX_FMT_BGRA:
		return VIDEO_FORMAT_BGRA;
	case AV_PIX_FMT_YUVA420P:
		return VIDEO_FORMAT_I40A;
	case AV_PIX_FMT_YUV420P10LE:
		return VIDEO_FORMAT_I010;
	case AV_PIX_FMT_YUVA422P:
		return VIDEO_FORMAT_I42A;
	case AV_PIX_FMT_YUVA444P:
		return VIDEO_FORMAT_YUVA;
	case AV_PIX_FMT_YUVA444P12LE:
		return VIDEO_FORMAT_YA2L;
	case AV_PIX_FMT_BGR0:
		return VIDEO_FORMAT_BGRX;
	case AV_PIX_FMT_P010LE:
		return VIDEO_FORMAT_P010;
	default:;
	}

	return VIDEO_FORMAT_NONE;
}

static inline enum audio_format convert_sample_format(int f)
{
	switch (f) {
	case AV_SAMPLE_FMT_U8:
		return AUDIO_FORMAT_U8BIT;
	case AV_SAMPLE_FMT_S16:
		return AUDIO_FORMAT_16BIT;
	case AV_SAMPLE_FMT_S32:
		return AUDIO_FORMAT_32BIT;
	case AV_SAMPLE_FMT_FLT:
		return AUDIO_FORMAT_FLOAT;
	case AV_SAMPLE_FMT_U8P:
		return AUDIO_FORMAT_U8BIT_PLANAR;
	case AV_SAMPLE_FMT_S16P:
		return AUDIO_FORMAT_16BIT_PLANAR;
	case AV_SAMPLE_FMT_S32P:
		return AUDIO_FORMAT_32BIT_PLANAR;
	case AV_SAMPLE_FMT_FLTP:
		return AUDIO_FORMAT_FLOAT_PLANAR;
	default:;
	}

	return AUDIO_FORMAT_UNKNOWN;
}

static inline enum speaker_layout convert_speaker_layout(uint8_t channels)
{
	switch (channels) {
	case 0:
		return SPEAKERS_UNKNOWN;
	case 1:
		return SPEAKERS_MONO;
	case 2:
		return SPEAKERS_STEREO;
	case 3:
		return SPEAKERS_2POINT1;
	case 4:
		return SPEAKERS_4POINT0;
	case 5:
		return SPEAKERS_4POINT1;
	case 6:
		return SPEAKERS_5POINT1;
	case 8:
		return SPEAKERS_7POINT1;
	default:
		return SPEAKERS_UNKNOWN;
	}
}

static inline enum video_colorspace convert_color_space(enum AVColorSpace s, enum AVColorTransferCharacteristic trc,
							enum AVColorPrimaries color_primaries)
{
	switch (s) {
	case AVCOL_SPC_BT709:
		return (trc == AVCOL_TRC_IEC61966_2_1) ? VIDEO_CS_SRGB : VIDEO_CS_709;
	case AVCOL_SPC_FCC:
	case AVCOL_SPC_BT470BG:
	case AVCOL_SPC_SMPTE170M:
	case AVCOL_SPC_SMPTE240M:
		return VIDEO_CS_601;
	case AVCOL_SPC_BT2020_NCL:
		return (trc == AVCOL_TRC_ARIB_STD_B67) ? VIDEO_CS_2100_HLG : VIDEO_CS_2100_PQ;
	default:
		return (color_primaries == AVCOL_PRI_BT2020)
			       ? ((trc == AVCOL_TRC_ARIB_STD_B67) ? VIDEO_CS_2100_HLG : VIDEO_CS_2100_PQ)
			       : VIDEO_CS_DEFAULT;
	}
}

static inline enum video_range_type convert_color_range(enum AVColorRange r)
{
	return r == AVCOL_RANGE_JPEG ? VIDEO_RANGE_FULL : VIDEO_RANGE_DEFAULT;
}

static inline struct mp_decode *get_packet_decoder(mp_media_t *media, const AVPacket *pkt)
{
	if (media->has_audio && pkt->stream_index == media->a.stream->index)
		return &media->a;
	if (media->has_video && pkt->stream_index == media->v.stream->index)
		return &media->v;

	return NULL;
}

void mp_media_free_packet(struct mp_media *media, AVPacket *pkt)
{
	av_packet_unref(pkt);
	da_push_back(media->packet_pool, &pkt);
}

static int mp_media_next_packet(mp_media_t *media)
{
	AVPacket *pkt;
	AVPacket **const cached = da_end(media->packet_pool);
	if (cached) {
		pkt = *cached;
		da_pop_back(media->packet_pool);
	} else {
		pkt = av_packet_alloc();
	}

	int ret = av_read_frame(media->fmt, pkt);
	if (ret < 0) {
		if (ret != AVERROR_EOF && ret != AVERROR_EXIT)
			blog(LOG_WARNING, "MP: av_read_frame failed: %s (%d)", av_err2str(ret), ret);
		return ret;
	}

	struct mp_decode *d = get_packet_decoder(media, pkt);
	if (d && pkt->size) {
		mp_decode_push_packet(d, pkt);
	} else {
		mp_media_free_packet(media, pkt);
	}

	return ret;
}

static inline bool mp_media_ready_to_start(mp_media_t *m)
{
	if (m->has_audio && !m->a.eof && !m->a.frame_ready)
		return false;
	if (m->has_video && !m->v.eof && !m->v.frame_ready)
		return false;
	return true;
}

static inline bool mp_decode_frame(struct mp_decode *d)
{
	return d->frame_ready || mp_decode_next(d);
}

static inline int get_sws_colorspace(enum AVColorSpace cs)
{
	switch (cs) {
	case AVCOL_SPC_BT709:
		return SWS_CS_ITU709;
	case AVCOL_SPC_FCC:
		return SWS_CS_FCC;
	case AVCOL_SPC_BT470BG:
		return SWS_CS_ITU624;
	case AVCOL_SPC_SMPTE170M:
		return SWS_CS_SMPTE170M;
	case AVCOL_SPC_SMPTE240M:
		return SWS_CS_SMPTE240M;
	case AVCOL_SPC_BT2020_NCL:
		return SWS_CS_BT2020;
	default:
		break;
	}

	return SWS_CS_ITU709;
}

static inline int get_sws_range(enum AVColorRange r)
{
	return r == AVCOL_RANGE_JPEG ? 1 : 0;
}

#define FIXED_1_0 (1 << 16)

static bool mp_media_init_scaling(mp_media_t *m)
{
	int space = get_sws_colorspace(m->v.frame->colorspace);
	int range = get_sws_range(m->v.frame->color_range);
	const int *coeff = sws_getCoefficients(space);

	m->swscale = sws_getCachedContext(NULL, m->v.frame->width, m->v.frame->height, m->v.frame->format,
					  m->v.frame->width, m->v.frame->height, m->scale_format, SWS_POINT, NULL, NULL,
					  NULL);
	if (!m->swscale) {
		blog(LOG_WARNING, "MP: Failed to initialize scaler");
		return false;
	}

	sws_setColorspaceDetails(m->swscale, coeff, range, coeff, range, 0, FIXED_1_0, FIXED_1_0);

	int ret = av_image_alloc(m->scale_pic, m->scale_linesizes, m->v.frame->width, m->v.frame->height,
				 m->scale_format, 32);
	if (ret < 0) {
		blog(LOG_WARNING, "MP: Failed to create scale pic data");
		return false;
	}

	return true;
}

bool mp_media_prepare_frames(mp_media_t *m)
{
	bool actively_seeking = m->seek_next_ts && m->pause;

	while (!mp_media_ready_to_start(m)) {
		if (!m->eof) {
			int ret = mp_media_next_packet(m);
			if (ret == AVERROR_EOF || ret == AVERROR_EXIT) {
				if (!actively_seeking) {
					m->eof = true;
				} else {
					break;
				}
			} else if (ret < 0) {
				return false;
			}
		}

		/* kind of a cheap fix, but because a stinger might be
		 * interrupted and restart playback, the request_preload signal
		 * might happen when the current frame is invalid, so clear out
		 * these pointers to signify they're not valid. (the obsframe
		 * structure is only used in the media thread, so this isn't a
		 * threading issue) */
		m->obsframe.data[0] = NULL;

		if (m->has_video && !mp_decode_frame(&m->v))
			return false;
		if (m->has_audio && !mp_decode_frame(&m->a))
			return false;
	}

	if (m->has_video && m->v.frame_ready && !m->swscale) {
		m->scale_format = closest_format(m->v.frame->format);
		if (m->scale_format != m->v.frame->format) {
			if (!mp_media_init_scaling(m)) {
				return false;
			}
		}
	}

	return true;
}

/* maximum timestamp variance in nanoseconds */
#define MAX_TS_VAR 2000000000LL

/* ------------------------------------------------------------------------- */
/* Pitch-preserving speed                                                    */

struct mp_tempo_frame {
	AVFrame *frame;
	int64_t pts;
};

/* added to the filter's worst-case delay to cover rounding */
#define TEMPO_LOOKAHEAD_MARGIN 2000000LL

static inline bool mp_media_use_tempo(mp_media_t *m)
{
	return m->speed != 100 && !m->tempo_failed;
}

static inline int64_t tempo_samples_to_ns(int64_t samples, int rate)
{
	return av_rescale(samples, 1000000000, rate);
}

/* media time spanned by audio fed to the filter, at the scaled speed */
static inline int64_t mp_media_tempo_fed_ns(mp_media_t *m, int64_t samples)
{
	return av_rescale(samples, 100000000000LL, (int64_t)m->tempo_sample_rate * m->speed);
}

/* atempo holds samples back, so its output isn't aligned with its input. The
 * output is laid out from the media time the filter's input started at: once
 * the frame at pts is fed, that's pts minus what was fed before it. Deriving
 * it again for every frame keeps gaps or jumps in the source timestamps. */
static inline int64_t mp_media_tempo_anchor(mp_media_t *m, int64_t pts)
{
	return pts - mp_media_tempo_fed_ns(m, m->tempo_in_samples);
}

static void mp_media_tempo_free_graph(mp_media_t *m)
{
	avfilter_graph_free(&m->tempo_graph);
	m->tempo_src = NULL;
	m->tempo_sink = NULL;
	av_frame_free(&m->tempo_frame);
	av_channel_layout_uninit(&m->tempo_ch_layout);
	m->tempo_format = AV_SAMPLE_FMT_NONE;
	m->tempo_sample_rate = 0;
	m->tempo_in_samples = 0;
	m->tempo_out_samples = 0;
	m->tempo_anchor_pts = 0;
	m->tempo_out_end_pts = 0;
}

static void mp_media_tempo_clear_queue(mp_media_t *m)
{
	struct mp_tempo_frame chunk = {0};

	while (m->tempo_queue.size) {
		deque_pop_front(&m->tempo_queue, &chunk, sizeof(chunk));
		av_frame_free(&chunk.frame);
	}
}

/* Drops the audio in progress, for when the timeline is interrupted. */
static void mp_media_tempo_reset(mp_media_t *m)
{
	mp_media_tempo_free_graph(m);
	mp_media_tempo_clear_queue(m);
	m->tempo_sent = false;
}

static void mp_media_tempo_free(mp_media_t *m)
{
	mp_media_tempo_reset(m);
	deque_free(&m->tempo_queue);
}

/* atempo accepts factors in [0.5, 100], so speeds below 50% are reached by
 * chaining 0.5x stages ahead of a final stage that is in range. Returns the
 * number of stages, and the final stage's factor in percent. */
static int mp_media_tempo_stages(int speed, int *last_percent)
{
	int stages = 1;

	while (speed < 50) {
		speed *= 2;
		stages++;
	}

	*last_percent = speed;
	return stages;
}

/* Factors are written as integer ratios so the description doesn't depend on
 * the C locale's decimal separator. */
static void mp_media_tempo_build_chain(char *chain, size_t size, int speed)
{
	int last_percent;
	int stages = mp_media_tempo_stages(speed, &last_percent);
	size_t len = 0;

	chain[0] = '\0';
	for (int i = 1; i < stages && len < size; i++)
		len += snprintf(chain + len, size - len, "atempo=1/2,");
	if (len < size)
		snprintf(chain + len, size - len, "atempo=%d/100", last_percent);
}

/* atempo works on overlapping windows of rate / 24 samples, rounded up to a
 * power of two, and moves its output half a window at a time */
static inline int mp_media_tempo_window(int rate)
{
	int window = rate / 24;
	int power_of_two = 1 << av_log2(window);

	return power_of_two < window ? power_of_two * 2 : power_of_two;
}

/* Audio is fed to the filter at most this many samples at a time, as the
 * filter holds back more when given larger frames. */
static inline int mp_media_tempo_slice(int rate)
{
	return FFMAX(mp_media_tempo_window(rate) / 2, 1);
}

/* Upper bound, in ns of media time, on how much audio the atempo chain holds
 * back (fed to it but not output yet). atempo moves its output W / 2 samples
 * at a time (W being its window size), and only passes output on once it has
 * filled a frame of round(input frame / tempo) samples. Each stage therefore
 * holds back less than W / tempo + W / 2 + output frame - 1 samples, and every
 * later stage stretches what the earlier ones hold back. */
static int64_t mp_media_tempo_max_delay_ns(int speed, int rate, int max_frame_samples)
{
	int window_samples = mp_media_tempo_window(rate);
	int last_percent;
	int stages = mp_media_tempo_stages(speed, &last_percent);
	double frame_samples = max_frame_samples;
	double delay_samples = 0.0;

	for (int i = 0; i < stages; i++) {
		double tempo = i + 1 < stages ? 0.5 : last_percent / 100.0;

		frame_samples = (double)(int64_t)(0.5 + frame_samples / tempo);
		delay_samples =
			delay_samples / tempo + window_samples / tempo + window_samples / 2 + frame_samples - 1.0;
	}

	return (int64_t)(delay_samples * 1000000000.0 / rate) + 1;
}

static void mp_media_tempo_raise_lookahead(mp_media_t *m, int64_t lookahead_ns)
{
	/* the bound stays well below this at any speed; it only keeps a filter
	 * that stops producing output from having the whole file decoded */
	int64_t max_lookahead_ns = MAX_TS_VAR * 100 / m->speed;

	if (lookahead_ns > max_lookahead_ns)
		lookahead_ns = max_lookahead_ns;
	if (lookahead_ns > m->tempo_lookahead_ns)
		m->tempo_lookahead_ns = lookahead_ns;
}

/* How far ahead of its pts decoded audio is fed to the tempo filter, so that
 * the filtered audio is ready by the time it's due. 0 if audio isn't fed
 * ahead. Updated for the frame that's waiting, as larger frames make the
 * filter hold more back. */
static int64_t mp_media_tempo_update_lookahead(mp_media_t *m)
{
	const AVFrame *f = m->a.frame;

	if (!m->has_audio || !m->a.frame_ready || m->full_decode || !mp_media_use_tempo(m))
		return 0;

	if (f->sample_rate != m->tempo_lookahead_rate) {
		m->tempo_lookahead_rate = f->sample_rate;
		m->tempo_max_frame_samples = 0;
	}

	int frame_samples = FFMIN(f->nb_samples, mp_media_tempo_slice(f->sample_rate));

	if (frame_samples > m->tempo_max_frame_samples) {
		int64_t delay_ns = mp_media_tempo_max_delay_ns(m->speed, f->sample_rate, frame_samples);

		m->tempo_max_frame_samples = frame_samples;
		mp_media_tempo_raise_lookahead(m, delay_ns + TEMPO_LOOKAHEAD_MARGIN);
	}

	return m->tempo_lookahead_ns;
}

static bool mp_media_tempo_init(mp_media_t *m, const AVFrame *f)
{
	AVFilterInOut *outputs = avfilter_inout_alloc();
	AVFilterInOut *inputs = avfilter_inout_alloc();
	AVChannelLayout layout = {0};
	char layout_name[128];
	char chain[128];
	char args[256];
	int ret = AVERROR(ENOMEM);

	mp_media_tempo_free_graph(m);

	m->tempo_graph = avfilter_graph_alloc();
	m->tempo_frame = av_frame_alloc();
	if (!outputs || !inputs || !m->tempo_graph || !m->tempo_frame)
		goto fail;

	m->tempo_graph->nb_threads = 1;

	/* abuffer needs a concrete layout, so use the default one for the
	 * channel count when the decoder doesn't report an order */
	if (f->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC)
		av_channel_layout_default(&layout, f->ch_layout.nb_channels);
	else if ((ret = av_channel_layout_copy(&layout, &f->ch_layout)) < 0)
		goto fail;
	av_channel_layout_describe(&layout, layout_name, sizeof(layout_name));

	snprintf(args, sizeof(args), "time_base=1/%d:sample_rate=%d:sample_fmt=%s:channel_layout=%s", f->sample_rate,
		 f->sample_rate, av_get_sample_fmt_name(f->format), layout_name);

	ret = avfilter_graph_create_filter(&m->tempo_src, avfilter_get_by_name("abuffer"), "in", args, NULL,
					   m->tempo_graph);
	if (ret < 0)
		goto fail;

	ret = avfilter_graph_create_filter(&m->tempo_sink, avfilter_get_by_name("abuffersink"), "out", NULL, NULL,
					   m->tempo_graph);
	if (ret < 0)
		goto fail;

	outputs->name = av_strdup("in");
	outputs->filter_ctx = m->tempo_src;
	outputs->pad_idx = 0;
	outputs->next = NULL;

	inputs->name = av_strdup("out");
	inputs->filter_ctx = m->tempo_sink;
	inputs->pad_idx = 0;
	inputs->next = NULL;

	mp_media_tempo_build_chain(chain, sizeof(chain), m->speed);

	ret = avfilter_graph_parse_ptr(m->tempo_graph, chain, &inputs, &outputs, NULL);
	if (ret < 0)
		goto fail;

	ret = avfilter_graph_config(m->tempo_graph, NULL);
	if (ret < 0)
		goto fail;

	ret = av_channel_layout_copy(&m->tempo_ch_layout, &f->ch_layout);
	if (ret < 0)
		goto fail;

	m->tempo_format = f->format;
	m->tempo_sample_rate = f->sample_rate;

	av_channel_layout_uninit(&layout);
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	return true;

fail:
	blog(LOG_WARNING, "MP: Failed to create tempo filter, speed will change pitch: %s", av_err2str(ret));
	av_channel_layout_uninit(&layout);
	avfilter_inout_free(&inputs);
	avfilter_inout_free(&outputs);
	mp_media_tempo_free_graph(m);
	return false;
}

/* Queues filtered audio. The output is laid out from tempo_anchor_pts, at the
 * real sample rate. */
static void mp_media_tempo_push(mp_media_t *m, AVFrame *f)
{
	struct mp_tempo_frame chunk = {0};

	chunk.frame = f;
	chunk.pts = m->tempo_anchor_pts + tempo_samples_to_ns(m->tempo_out_samples, f->sample_rate);
	deque_push_back(&m->tempo_queue, &chunk, sizeof(chunk));

	m->tempo_out_samples += f->nb_samples;
	m->tempo_out_end_pts = m->tempo_anchor_pts + tempo_samples_to_ns(m->tempo_out_samples, f->sample_rate);
}

/* Moves the filter's output to the queue. */
static int mp_media_tempo_receive(mp_media_t *m)
{
	for (;;) {
		AVFrame *out = av_frame_alloc();
		int ret;

		if (!out)
			return AVERROR(ENOMEM);

		ret = av_buffersink_get_frame(m->tempo_sink, out);
		if (ret < 0) {
			av_frame_free(&out);
			return ret == AVERROR(EAGAIN) || ret == AVERROR_EOF ? 0 : ret;
		}

		mp_media_tempo_push(m, out);
	}
}

/* atempo's output doesn't end exactly where its input ends at the scaled
 * speed: it can be a few ms short or long. Trim or pad it so the audio ends
 * where the media timeline says it does; otherwise the audio that follows
 * (e.g. the next loop) is snapped against it by the source's timestamp
 * smoothing, shifting it against the video. */
static void mp_media_tempo_fix_length(mp_media_t *m)
{
	const int silence_chunk_samples = 1024;
	int rate = av_buffersink_get_sample_rate(m->tempo_sink);
	int64_t wanted_samples =
		av_rescale(m->tempo_in_samples, 100LL * rate, (int64_t)m->speed * m->tempo_sample_rate);
	int64_t diff_samples = wanted_samples - m->tempo_out_samples;

	/* too long: shorten what's still queued */
	while (diff_samples < 0 && m->tempo_queue.size) {
		struct mp_tempo_frame chunk = {0};

		deque_peek_back(&m->tempo_queue, &chunk, sizeof(chunk));
		if (-diff_samples < chunk.frame->nb_samples) {
			chunk.frame->nb_samples += (int)diff_samples;
			diff_samples = 0;
		} else {
			deque_pop_back(&m->tempo_queue, NULL, sizeof(chunk));
			diff_samples += chunk.frame->nb_samples;
			av_frame_free(&chunk.frame);
		}
	}

	/* too short: pad with silence */
	while (diff_samples > 0) {
		AVFrame *f = av_frame_alloc();
		int samples = (int)(diff_samples < silence_chunk_samples ? diff_samples : silence_chunk_samples);

		if (!f)
			break;

		f->format = av_buffersink_get_format(m->tempo_sink);
		f->sample_rate = rate;
		f->nb_samples = samples;
		if (av_buffersink_get_ch_layout(m->tempo_sink, &f->ch_layout) < 0 || av_frame_get_buffer(f, 0) < 0) {
			av_frame_free(&f);
			break;
		}
		av_samples_set_silence(f->extended_data, 0, samples, f->ch_layout.nb_channels, f->format);

		mp_media_tempo_push(m, f);
		diff_samples -= samples;
	}
}

/* Once the audio has run out, moves what the filter still holds to the queue
 * and releases the filter. */
static void mp_media_tempo_drain(mp_media_t *m)
{
	if (!m->tempo_graph)
		return;

	if (av_buffersrc_add_frame_flags(m->tempo_src, NULL, 0) >= 0 && mp_media_tempo_receive(m) >= 0)
		mp_media_tempo_fix_length(m);

	mp_media_tempo_free_graph(m);
}

static int mp_media_tempo_feed(mp_media_t *m, const AVFrame *f, int sample_offset, int sample_count)
{
	AVFrame *in = m->tempo_frame;
	int ret;

	if (sample_offset == 0 && sample_count == f->nb_samples) {
		ret = av_frame_ref(in, f);
	} else {
		in->format = f->format;
		in->sample_rate = f->sample_rate;
		in->nb_samples = sample_count;
		ret = av_channel_layout_copy(&in->ch_layout, &f->ch_layout);
		if (ret >= 0)
			ret = av_frame_get_buffer(in, 0);
		if (ret >= 0)
			ret = av_samples_copy(in->extended_data, f->extended_data, 0, sample_offset, sample_count,
					      f->ch_layout.nb_channels, f->format);
	}

	if (ret >= 0) {
		in->pts = m->tempo_in_samples + sample_offset;
		if (in->ch_layout.order == AV_CHANNEL_ORDER_UNSPEC) {
			int channels = in->ch_layout.nb_channels;
			av_channel_layout_uninit(&in->ch_layout);
			av_channel_layout_default(&in->ch_layout, channels);
		}
		ret = av_buffersrc_add_frame_flags(m->tempo_src, in, 0);
	}

	av_frame_unref(in);
	return ret;
}

/* Feeds a decoded frame to the filter. Returns false if that failed, in which
 * case the tempo path is turned off and the frame should be output the
 * regular (pitch-shifting) way. */
static bool mp_media_tempo_process(mp_media_t *m, const AVFrame *f, int64_t pts)
{
	bool needs_new_graph = !m->tempo_graph || f->format != m->tempo_format ||
			       f->sample_rate != m->tempo_sample_rate ||
			       av_channel_layout_compare(&f->ch_layout, &m->tempo_ch_layout) != 0;
	int slice_samples = mp_media_tempo_slice(f->sample_rate);
	int ret = 0;

	if (needs_new_graph) {
		/* the filter is set up for the previous format, so finish
		 * what it holds, laid out against this frame like any other
		 * frame would, before starting over with the new one */
		if (m->tempo_graph)
			m->tempo_anchor_pts = mp_media_tempo_anchor(m, pts);
		mp_media_tempo_drain(m);

		if (!mp_media_tempo_init(m, f)) {
			m->tempo_failed = true;
			return false;
		}
	}

	m->tempo_anchor_pts = mp_media_tempo_anchor(m, pts);

	for (int offset = 0; ret >= 0 && offset < f->nb_samples; offset += slice_samples)
		ret = mp_media_tempo_feed(m, f, offset, FFMIN(slice_samples, f->nb_samples - offset));
	if (ret >= 0)
		ret = mp_media_tempo_receive(m);

	if (ret < 0) {
		blog(LOG_WARNING, "MP: Failed to filter audio, speed will change pitch: %s", av_err2str(ret));
		mp_media_tempo_free_graph(m);
		m->tempo_failed = true;
		return false;
	}

	m->tempo_in_samples += f->nb_samples;

	int rate = av_buffersink_get_sample_rate(m->tempo_sink);
	int64_t fed_end_pts = m->tempo_anchor_pts + mp_media_tempo_fed_ns(m, m->tempo_in_samples);
	m->tempo_out_end_pts = m->tempo_anchor_pts + tempo_samples_to_ns(m->tempo_out_samples, rate);

	/* the look-ahead comes from atempo's worst case, but make sure it
	 * always covers what the filter actually holds back */
	mp_media_tempo_raise_lookahead(m, fed_end_pts - m->tempo_out_end_pts + TEMPO_LOOKAHEAD_MARGIN);
	return true;
}

static inline bool mp_media_tempo_due(mp_media_t *m, int64_t pts)
{
	if (m->full_decode || pts <= m->next_pts_ns)
		return true;

	/* the video frame this pass output, if any, is the one the clock sits
	 * on: it was output just before this, so it's no longer ready */
	bool clock_at_video = m->has_video && !m->v.frame_ready && m->v.frame_pts == m->next_pts_ns;

	/* like decoded frames, audio far ahead of the clock goes out right
	 * away, but only at a video frame, which is the only time the regular
	 * path can see audio that far ahead. Otherwise, long frames at low
	 * speeds or a timestamp jump would have audio fed or sent early at the
	 * tempo path's own wake-ups. */
	return clock_at_video && pts - m->next_pts_ns > MAX_TS_VAR;
}

static void mp_media_tempo_send(mp_media_t *m, const AVFrame *f, int64_t pts)
{
	struct obs_source_audio audio = {0};

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		audio.data[i] = f->data[i];

	/* the samples are already time-stretched, so they play at their real
	 * rate */
	audio.samples_per_sec = f->sample_rate;
	audio.speakers = convert_speaker_layout(f->ch_layout.nb_channels);
	audio.format = convert_sample_format(f->format);
	audio.frames = f->nb_samples;
	audio.timestamp = m->full_decode ? pts : m->base_ts + pts - m->start_ts + m->play_sys_ts - base_sys_ts;

	if (audio.format == AUDIO_FORMAT_UNKNOWN)
		return;

	/* the cache takes the length of the last audio it gets from here,
	 * which is no longer the length of the last decoded frame */
	if (m->full_decode)
		m->a.last_duration = tempo_samples_to_ns(f->nb_samples, f->sample_rate);

	m->a_cb(m->opaque, &audio);
}

/* Sends the queued audio that's due. */
static void mp_media_tempo_send_due(mp_media_t *m)
{
	struct mp_tempo_frame chunk = {0};

	while (m->tempo_queue.size) {
		deque_peek_front(&m->tempo_queue, &chunk, sizeof(chunk));
		if (!mp_media_tempo_due(m, chunk.pts))
			break;

		deque_pop_front(&m->tempo_queue, NULL, sizeof(chunk));
		if (m->a_cb)
			mp_media_tempo_send(m, chunk.frame, chunk.pts);

		m->tempo_sent_end_pts =
			chunk.pts + tempo_samples_to_ns(chunk.frame->nb_samples, chunk.frame->sample_rate);
		m->tempo_sent = true;
		av_frame_free(&chunk.frame);
	}
}

/* Media time of the first audio that hasn't been sent to the source. */
static bool mp_media_audio_pending_pts(mp_media_t *m, int64_t *pts)
{
	if (m->tempo_queue.size) {
		struct mp_tempo_frame chunk = {0};

		deque_peek_front(&m->tempo_queue, &chunk, sizeof(chunk));
		*pts = chunk.pts;
		return true;
	}
	if (m->tempo_graph) {
		/* audio the filter holds back, which is laid out against the
		 * next frame once that is fed */
		int64_t anchor = m->a.frame_ready ? mp_media_tempo_anchor(m, m->a.frame_pts) : m->tempo_anchor_pts;
		int rate = av_buffersink_get_sample_rate(m->tempo_sink);

		*pts = anchor + tempo_samples_to_ns(m->tempo_out_samples, rate);
		return true;
	}
	if (m->a.frame_ready) {
		*pts = m->a.frame_pts;
		return true;
	}

	return false;
}

/* ------------------------------------------------------------------------- */

/* Earliest media time of what's waiting to be output: where the clock starts
 * after the timeline is reset. */
static inline int64_t mp_media_get_next_min_pts(mp_media_t *m)
{
	int64_t min_next_ns = 0x7FFFFFFFFFFFFFFFLL;
	int64_t audio_pts = 0;

	if (m->has_video && m->v.frame_ready) {
		if (m->v.frame_pts < min_next_ns)
			min_next_ns = m->v.frame_pts;
	}
	if (m->has_audio && mp_media_audio_pending_pts(m, &audio_pts)) {
		if (audio_pts < min_next_ns)
			min_next_ns = audio_pts;
	}

	return min_next_ns;
}

/* Media time at which something next needs to happen: a frame being output,
 * filtered audio being sent, or decoded audio being fed to the tempo filter. */
static inline int64_t mp_media_get_next_event_pts(mp_media_t *m)
{
	int64_t min_next_ns = 0x7FFFFFFFFFFFFFFFLL;

	if (m->has_video && m->v.frame_ready) {
		if (m->v.frame_pts < min_next_ns)
			min_next_ns = m->v.frame_pts;
	}
	if (m->tempo_queue.size) {
		struct mp_tempo_frame chunk = {0};

		deque_peek_front(&m->tempo_queue, &chunk, sizeof(chunk));
		if (chunk.pts < min_next_ns)
			min_next_ns = chunk.pts;
	}
	if (m->has_audio && m->a.frame_ready) {
		int64_t lookahead_ns = mp_media_tempo_update_lookahead(m);
		int64_t feed_pts = m->a.frame_pts - lookahead_ns;

		/* feeding ahead doesn't move the clock back: audio that's
		 * already due to be fed is fed right away */
		if (lookahead_ns && feed_pts < m->next_pts_ns)
			feed_pts = m->a.frame_pts < m->next_pts_ns ? m->a.frame_pts : m->next_pts_ns;
		if (feed_pts < min_next_ns)
			min_next_ns = feed_pts;
	}

	return min_next_ns;
}

static inline int64_t mp_media_get_base_pts(mp_media_t *m)
{
	int64_t base_ts = 0;
	int64_t audio_end = m->a.next_pts;

	/* with the tempo filter, only audio that has been sent counts */
	if (m->tempo_sent)
		audio_end = m->tempo_sent_end_pts;
	else if (m->tempo_graph || m->tempo_queue.size)
		mp_media_audio_pending_pts(m, &audio_end);

	if (m->has_video && m->v.next_pts > base_ts)
		base_ts = m->v.next_pts;
	if (m->has_audio && audio_end > base_ts)
		base_ts = audio_end;

	return base_ts;
}

static inline bool mp_media_can_play_frame(mp_media_t *m, struct mp_decode *d)
{
	if (m->full_decode)
		return d->frame_ready;
	return d->frame_ready && (d->frame_pts <= m->next_pts_ns || (d->frame_pts - m->next_pts_ns > MAX_TS_VAR));
}

void mp_media_next_audio(mp_media_t *m)
{
	struct mp_decode *d = &m->a;
	struct obs_source_audio audio = {0};
	AVFrame *f = d->frame;

	mp_media_tempo_send_due(m);

	if (mp_media_use_tempo(m)) {
		int64_t feed_pts = d->frame_pts - mp_media_tempo_update_lookahead(m);

		if (!d->frame_ready || !mp_media_tempo_due(m, feed_pts))
			return;

		if (!m->a_cb) {
			d->frame_ready = false;
			return;
		}

		if (mp_media_tempo_process(m, f, d->frame_pts)) {
			d->frame_ready = false;
			mp_media_tempo_send_due(m);
			return;
		}

		/* the tempo filter failed, the frame goes out the regular way
		 * once it's due */
	}

	if (!mp_media_can_play_frame(m, d))
		return;

	d->frame_ready = false;
	if (!m->a_cb)
		return;

	for (size_t i = 0; i < MAX_AV_PLANES; i++)
		audio.data[i] = f->data[i];

	audio.samples_per_sec = f->sample_rate * m->speed / 100;
	audio.speakers = convert_speaker_layout(f->ch_layout.nb_channels);
	audio.format = convert_sample_format(f->format);
	audio.frames = f->nb_samples;
	audio.timestamp = m->full_decode ? d->frame_pts
					 : m->base_ts + d->frame_pts - m->start_ts + m->play_sys_ts - base_sys_ts;

	if (audio.format == AUDIO_FORMAT_UNKNOWN)
		return;

	m->tempo_sent = false;
	m->a_cb(m->opaque, &audio);
}

void mp_media_next_video(mp_media_t *m, bool preload)
{
	struct mp_decode *d = &m->v;
	struct obs_source_frame *frame = &m->obsframe;
	enum video_format new_format;
	enum video_colorspace new_space;
	enum video_range_type new_range;
	AVFrame *f = d->frame;

	if (!preload) {
		if (!mp_media_can_play_frame(m, d))
			return;

		d->frame_ready = false;

		if (!m->v_cb)
			return;
	} else if (!d->frame_ready) {
		return;
	}

	if (!f->width || !f->height) {
		blog(LOG_ERROR, "MP: media frame width or height are zero ('%s': %" PRIu32 "x%" PRIu32 ")", m->path,
		     f->width, f->height);
		return;
	}

	bool flip = false;
	if (m->swscale) {
		int ret = sws_scale(m->swscale, (const uint8_t *const *)f->data, f->linesize, 0, f->height,
				    m->scale_pic, m->scale_linesizes);
		if (ret < 0)
			return;

		flip = m->scale_linesizes[0] < 0 && m->scale_linesizes[1] == 0;
		for (size_t i = 0; i < 4; i++) {
			frame->data[i] = m->scale_pic[i];
			frame->linesize[i] = abs(m->scale_linesizes[i]);
		}

	} else {
		flip = f->linesize[0] < 0 && f->linesize[1] == 0;

		for (size_t i = 0; i < MAX_AV_PLANES; i++) {
			frame->data[i] = f->data[i];
			frame->linesize[i] = abs(f->linesize[i]);
		}
	}

	if (flip)
		frame->data[0] -= frame->linesize[0] * ((size_t)f->height - 1);

	new_format = convert_pixel_format(m->scale_format);
	new_space = convert_color_space(f->colorspace, f->color_trc, f->color_primaries);
	new_range = m->force_range == VIDEO_RANGE_DEFAULT ? convert_color_range(f->color_range) : m->force_range;

	if (new_format != frame->format || new_space != m->cur_space || new_range != m->cur_range) {
		bool success;

		frame->format = new_format;
		frame->full_range = new_range == VIDEO_RANGE_FULL;

		success = video_format_get_parameters_for_format(new_space, new_range, new_format, frame->color_matrix,
								 frame->color_range_min, frame->color_range_max);

		frame->format = new_format;
		m->cur_space = new_space;
		m->cur_range = new_range;

		if (!success) {
			frame->format = VIDEO_FORMAT_NONE;
			return;
		}
	}

	if (frame->format == VIDEO_FORMAT_NONE)
		return;

	frame->timestamp = m->full_decode ? d->frame_pts
					  : (m->base_ts + d->frame_pts - m->start_ts + m->play_sys_ts - base_sys_ts);

	frame->width = f->width;
	frame->height = f->height;
	frame->max_luminance = d->max_luminance;
	frame->flip = flip;
	frame->flags = m->is_linear_alpha ? OBS_SOURCE_FRAME_LINEAR_ALPHA : 0;
	switch (f->color_trc) {
	case AVCOL_TRC_BT709:
	case AVCOL_TRC_GAMMA22:
	case AVCOL_TRC_GAMMA28:
	case AVCOL_TRC_SMPTE170M:
	case AVCOL_TRC_SMPTE240M:
	case AVCOL_TRC_IEC61966_2_1:
		frame->trc = VIDEO_TRC_SRGB;
		break;
	case AVCOL_TRC_SMPTE2084:
		frame->trc = VIDEO_TRC_PQ;
		break;
	case AVCOL_TRC_ARIB_STD_B67:
		frame->trc = VIDEO_TRC_HLG;
		break;
	default:
		frame->trc = VIDEO_TRC_DEFAULT;
	}

	if (!m->is_local_file && !d->got_first_keyframe) {
		if (!(f->flags & AV_FRAME_FLAG_KEY))
			return;

		d->got_first_keyframe = true;
	}

	if (preload) {
		if (m->seek_next_ts && m->v_seek_cb) {
			m->v_seek_cb(m->opaque, frame);
		} else if (!m->request_preload) {
			m->v_preload_cb(m->opaque, frame);
		}
	} else {
		m->v_cb(m->opaque, frame);
	}
}

static void mp_media_calc_next_ns(mp_media_t *m)
{
	int64_t min_next_ns;
	int64_t delta;

	if (m->seek_next_ts) {
		/* the clock starts over at the new position */
		min_next_ns = mp_media_get_next_min_pts(m);
		delta = 0;
		m->seek_next_ts = false;
	} else {
		min_next_ns = mp_media_get_next_event_pts(m);
		delta = min_next_ns - m->next_pts_ns;
#ifdef _DEBUG
		assert(delta >= 0);
#endif
		if (delta < 0)
			delta = 0;
		if (delta > 3000000000)
			delta = 0;
	}

	m->next_ns += delta;
	m->next_pts_ns = min_next_ns;
}

static void seek_to(mp_media_t *m, int64_t pos)
{
	AVStream *stream = m->fmt->streams[0];
	int64_t seek_pos = pos;
	int seek_flags;

	if (m->fmt->duration == AV_NOPTS_VALUE)
		seek_flags = AVSEEK_FLAG_FRAME;
	else
		seek_flags = AVSEEK_FLAG_BACKWARD;

	int64_t seek_target = seek_flags == AVSEEK_FLAG_BACKWARD
				      ? av_rescale_q(seek_pos, AV_TIME_BASE_Q, stream->time_base)
				      : seek_pos;

	if (m->is_local_file) {
		int ret = av_seek_frame(m->fmt, 0, seek_target, seek_flags);
		if (ret < 0) {
			blog(LOG_WARNING, "MP: Failed to seek: %s", av_err2str(ret));
		}
	}

	if (m->has_video && m->is_local_file) {
		mp_decode_flush(&m->v);
		if (m->seek_next_ts && m->pause && m->v_preload_cb && mp_media_prepare_frames(m))
			mp_media_next_video(m, true);
	}
	if (m->has_audio && m->is_local_file)
		mp_decode_flush(&m->a);

	/* the timeline restarts, so drop any audio held for the old position */
	mp_media_tempo_reset(m);
}

bool mp_media_reset(mp_media_t *m)
{
	bool stopping;
	bool active;

	int64_t next_ts = mp_media_get_base_pts(m);
	int64_t offset = next_ts - m->next_pts_ns;
	int64_t start_time = m->fmt->start_time;
	if (start_time == AV_NOPTS_VALUE)
		start_time = 0;

	m->eof = false;
	m->base_ts += next_ts;
	m->seek_next_ts = false;

	seek_to(m, start_time);

	pthread_mutex_lock(&m->mutex);
	stopping = m->stopping;
	active = m->active;
	m->stopping = false;
	pthread_mutex_unlock(&m->mutex);

	if (!mp_media_prepare_frames(m))
		return false;

	if (active) {
		if (!m->play_sys_ts)
			m->play_sys_ts = (int64_t)os_gettime_ns();
		m->start_ts = m->next_pts_ns = mp_media_get_next_min_pts(m);
		if (m->next_ns)
			m->next_ns += offset;
	} else {
		m->start_ts = m->next_pts_ns = mp_media_get_next_min_pts(m);
		m->play_sys_ts = (int64_t)os_gettime_ns();
		m->next_ns = 0;
	}

	m->pause = false;

	if (!active && m->is_local_file && m->v_preload_cb)
		mp_media_next_video(m, true);
	if (stopping && m->stop_cb)
		m->stop_cb(m->opaque);
	return true;
}

static inline bool mp_media_sleep(mp_media_t *m)
{
	bool timeout = false;

	if (!m->next_ns) {
		m->next_ns = os_gettime_ns();
	} else {
		const uint64_t t = os_gettime_ns();
		if (m->next_ns > t) {
			const uint32_t delta_ms = (uint32_t)((m->next_ns - t + 500000) / 1000000);
			if (delta_ms > 0) {
				static const uint32_t timeout_ms = 200;
				timeout = delta_ms > timeout_ms;
				os_sleep_ms(timeout ? timeout_ms : delta_ms);
			}
		}
	}

	return timeout;
}

bool mp_media_eof(mp_media_t *m)
{
	bool v_ended = !m->has_video || !m->v.frame_ready;
	bool a_ended = !m->has_audio || !m->a.frame_ready;

	/* once the audio has run out, flush what the tempo filter still holds;
	 * the audio isn't over until everything queued has been sent */
	if (m->has_audio && m->a.eof && !m->a.frame_ready)
		mp_media_tempo_drain(m);
	if (m->tempo_queue.size)
		a_ended = false;

	bool eof = v_ended && a_ended;
	if (eof) {
		bool looping;

		pthread_mutex_lock(&m->mutex);
		looping = m->looping;
		if (!looping) {
			m->active = false;
			m->stopping = true;
		}
		pthread_mutex_unlock(&m->mutex);

		mp_media_reset(m);
	}

	return eof;
}

static int interrupt_callback(void *data)
{
	mp_media_t *m = data;
	bool stop = false;
	uint64_t ts = os_gettime_ns();

	if ((ts - m->interrupt_poll_ts) > 20000000) {
		pthread_mutex_lock(&m->mutex);
		stop = m->kill || m->stopping;
		pthread_mutex_unlock(&m->mutex);

		m->interrupt_poll_ts = ts;
	}

	return stop;
}

#define RIST_PROTO "rist"

static bool init_avformat(mp_media_t *m)
{
	const AVInputFormat *format = NULL;

	if (m->format_name && *m->format_name) {
		format = av_find_input_format(m->format_name);
		if (!format)
			blog(LOG_INFO,
			     "MP: Unable to find input format for "
			     "'%s'",
			     m->path);
	}

	AVDictionary *opts = NULL;
	bool is_rist = strncmp(m->path, RIST_PROTO, strlen(RIST_PROTO)) == 0;
	if (m->buffering && !m->is_local_file && !is_rist)
		av_dict_set_int(&opts, "buffer_size", m->buffering, 0);

	if (m->ffmpeg_options) {
		int ret = av_dict_parse_string(&opts, m->ffmpeg_options, "=", " ", 0);
		if (ret)
			blog(LOG_WARNING, "Failed to parse FFmpeg options: %s\n%s", av_err2str(ret), m->ffmpeg_options);
	}

	m->fmt = avformat_alloc_context();
	if (m->buffering == 0) {
		m->fmt->flags |= AVFMT_FLAG_NOBUFFER;
	}
	if (!m->is_local_file) {
		av_dict_set(&opts, "stimeout", "30000000", 0);
		m->fmt->interrupt_callback.callback = interrupt_callback;
		m->fmt->interrupt_callback.opaque = m;
	}

	int ret = avformat_open_input(&m->fmt, m->path, format, opts ? &opts : NULL);
	av_dict_free(&opts);

	if (ret < 0) {
		if (!m->reconnecting)
			blog(LOG_WARNING, "MP: Failed to open media: '%s'", m->path);
		return false;
	}

	if (avformat_find_stream_info(m->fmt, NULL) < 0) {
		blog(LOG_WARNING, "MP: Failed to find stream info for '%s'", m->path);
		return false;
	}

	m->reconnecting = false;
	m->has_video = mp_decode_init(m, AVMEDIA_TYPE_VIDEO, m->hw);
	m->has_audio = mp_decode_init(m, AVMEDIA_TYPE_AUDIO, m->hw);

	if (!m->has_video && !m->has_audio) {
		blog(LOG_WARNING,
		     "MP: Could not initialize audio or video: "
		     "'%s'",
		     m->path);
		return false;
	}

	return true;
}

static void reset_ts(mp_media_t *m)
{
	m->base_ts += mp_media_get_base_pts(m);
	m->play_sys_ts = (int64_t)os_gettime_ns();
	m->start_ts = m->next_pts_ns = mp_media_get_next_min_pts(m);
	m->next_ns = 0;
}

bool mp_media_init2(mp_media_t *m)
{
	if (!init_avformat(m)) {
		return false;
	}
	return true;
}

static inline bool mp_media_thread(mp_media_t *m)
{
	os_set_thread_name("mp_media_thread");

	if (!mp_media_init2(m)) {
		return false;
	}
	if (!mp_media_reset(m)) {
		return false;
	}

	for (;;) {
		bool reset, kill, is_active, seek, pause, reset_time, preload_frame;
		int64_t seek_pos;
		bool timeout = false;

		pthread_mutex_lock(&m->mutex);
		is_active = m->active;
		pause = m->pause;
		pthread_mutex_unlock(&m->mutex);

		if (!is_active || pause) {
			if (os_sem_wait(m->sem) < 0)
				return false;
			if (pause)
				reset_ts(m);
		} else {
			timeout = mp_media_sleep(m);
		}

		pthread_mutex_lock(&m->mutex);

		reset = m->reset;
		kill = m->kill;
		m->reset = false;
		m->kill = false;

		preload_frame = m->preload_frame;
		pause = m->pause;
		seek_pos = m->seek_pos;
		seek = m->seek;
		reset_time = m->reset_ts;
		m->preload_frame = false;
		m->seek = false;
		m->reset_ts = false;

		pthread_mutex_unlock(&m->mutex);

		if (kill) {
			break;
		}
		if (reset) {
			mp_media_reset(m);
			continue;
		}

		if (seek) {
			m->seek_next_ts = true;
			seek_to(m, seek_pos);
			continue;
		}

		if (reset_time) {
			reset_ts(m);
			continue;
		}

		if (pause)
			continue;

		/* see note in mp_media_prepare_frames() for context on the
		 * pointer check */
		if (preload_frame && m->obsframe.data[0] && !is_active) {
			m->v_preload_cb(m->opaque, &m->obsframe);
		}

		/* frames are ready */
		if (is_active && !timeout) {
			if (m->has_video)
				mp_media_next_video(m, false);
			if (m->has_audio)
				mp_media_next_audio(m);

			if (!mp_media_prepare_frames(m))
				return false;
			if (mp_media_eof(m))
				continue;

			mp_media_calc_next_ns(m);
		}
	}

	return true;
}

static void *mp_media_thread_start(void *opaque)
{
	mp_media_t *m = opaque;

	if (!mp_media_thread(m)) {
		if (m->stop_cb) {
			m->stop_cb(m->opaque);
		}
	}

	return NULL;
}

static inline bool mp_media_init_internal(mp_media_t *m, const struct mp_media_info *info)
{
	if (pthread_mutex_init(&m->mutex, NULL) != 0) {
		blog(LOG_WARNING, "MP: Failed to init mutex");
		return false;
	}
	if (os_sem_init(&m->sem, 0) != 0) {
		blog(LOG_WARNING, "MP: Failed to init semaphore");
		return false;
	}

	m->path = info->path ? bstrdup(info->path) : NULL;
	m->format_name = info->format ? bstrdup(info->format) : NULL;
	m->hw = info->hardware_decoding;

	if (info->full_decode)
		return true;

	if (pthread_create(&m->thread, NULL, mp_media_thread_start, m) != 0) {
		blog(LOG_WARNING, "MP: Could not create media thread");
		return false;
	}

	m->thread_valid = true;
	return true;
}

bool mp_media_init(mp_media_t *media, const struct mp_media_info *info)
{
	memset(media, 0, sizeof(*media));
	pthread_mutex_init_value(&media->mutex);
	media->opaque = info->opaque;
	media->v_cb = info->v_cb;
	media->a_cb = info->a_cb;
	media->stop_cb = info->stop_cb;
	media->ffmpeg_options = info->ffmpeg_options;
	media->v_seek_cb = info->v_seek_cb;
	media->v_preload_cb = info->v_preload_cb;
	media->force_range = info->force_range;
	media->is_linear_alpha = info->is_linear_alpha;
	media->buffering = info->buffering;
	media->speed = info->speed;
	media->request_preload = info->request_preload;
	media->is_local_file = info->is_local_file;
	da_init(media->packet_pool);

	if (!info->is_local_file || media->speed < 1 || media->speed > 200)
		media->speed = 100;

	static bool initialized = false;
	if (!initialized) {
		avdevice_register_all();
		avformat_network_init();
		initialized = true;
	}

	if (!base_sys_ts)
		base_sys_ts = (int64_t)os_gettime_ns();

	if (!mp_media_init_internal(media, info)) {
		mp_media_free(media);
		return false;
	}

	return true;
}

static void mp_kill_thread(mp_media_t *m)
{
	if (m->thread_valid) {
		pthread_mutex_lock(&m->mutex);
		m->kill = true;
		pthread_mutex_unlock(&m->mutex);
		os_sem_post(m->sem);

		pthread_join(m->thread, NULL);
	}
}

void mp_media_free(mp_media_t *media)
{
	if (!media)
		return;

	mp_media_stop(media);
	mp_kill_thread(media);
	mp_decode_free(&media->v);
	mp_decode_free(&media->a);
	mp_media_tempo_free(media);
	for (size_t i = 0; i < media->packet_pool.num; i++)
		av_packet_free(&media->packet_pool.array[i]);
	da_free(media->packet_pool);
	avformat_close_input(&media->fmt);
	pthread_mutex_destroy(&media->mutex);
	os_sem_destroy(media->sem);
	sws_freeContext(media->swscale);
	av_freep(&media->scale_pic[0]);
	bfree(media->path);
	bfree(media->format_name);
	memset(media, 0, sizeof(*media));
	pthread_mutex_init_value(&media->mutex);
}

void mp_media_play(mp_media_t *m, bool loop, bool reconnecting)
{
	pthread_mutex_lock(&m->mutex);

	if (m->active)
		m->reset = true;

	m->looping = loop;
	m->active = true;
	m->reconnecting = reconnecting;

	pthread_mutex_unlock(&m->mutex);

	os_sem_post(m->sem);
}

void mp_media_play_pause(mp_media_t *m, bool pause)
{
	pthread_mutex_lock(&m->mutex);
	if (m->active) {
		m->pause = pause;
		m->reset_ts = !pause;
	}
	pthread_mutex_unlock(&m->mutex);

	os_sem_post(m->sem);
}

void mp_media_preload_frame(mp_media_t *m)
{
	if (m->request_preload && m->thread_valid && m->v_preload_cb) {
		pthread_mutex_lock(&m->mutex);
		m->preload_frame = true;
		pthread_mutex_unlock(&m->mutex);
		os_sem_post(m->sem);
	}
}

void mp_media_stop(mp_media_t *m)
{
	pthread_mutex_lock(&m->mutex);
	if (m->active) {
		m->reset = true;
		m->active = false;
		m->stopping = true;
	}
	pthread_mutex_unlock(&m->mutex);

	os_sem_post(m->sem);
}

int64_t mp_media_get_current_time(mp_media_t *m)
{
	/* this isn't called from the media thread, so it can't look at the
	 * tempo queue; only the end of the audio sent so far is used */
	int64_t base_ts = 0;
	int64_t audio_end = m->tempo_sent ? m->tempo_sent_end_pts : m->a.next_pts;

	if (m->has_video && m->v.next_pts > base_ts)
		base_ts = m->v.next_pts;
	if (m->has_audio && audio_end > base_ts)
		base_ts = audio_end;

	return base_ts * (int64_t)m->speed / 100000000LL;
}

int64_t mp_media_get_frames(mp_media_t *m)
{
	int64_t frames = 0;

	if (!m->fmt) {
		return 0;
	}

	int video_stream_index = av_find_best_stream(m->fmt, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);

	if (video_stream_index < 0) {
		blog(LOG_WARNING, "MP: Getting number of frames failed: No "
				  "video stream in media file!");
		return 0;
	}

	AVStream *stream = m->fmt->streams[video_stream_index];

	if (stream->nb_frames > 0) {
		frames = stream->nb_frames;
	} else {
		blog(LOG_DEBUG, "MP: nb_frames not set, estimating using frame "
				"rate and duration");
		AVRational avg_frame_rate = stream->avg_frame_rate;
		frames = (int64_t)ceil((double)m->fmt->duration / (double)AV_TIME_BASE * (double)avg_frame_rate.num /
				       (double)avg_frame_rate.den);
	}

	return frames;
}

int64_t mp_media_get_duration(mp_media_t *m)
{
	return m->fmt ? m->fmt->duration : 0;
}

void mp_media_seek(mp_media_t *m, int64_t pos)
{
	pthread_mutex_lock(&m->mutex);
	if (m->active) {
		m->seek = true;
		m->seek_pos = pos * 1000;
	}
	pthread_mutex_unlock(&m->mutex);

	os_sem_post(m->sem);
}
