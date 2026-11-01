#pragma once

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef NEW_MPEGTS_OUTPUT
#include "obs-ffmpeg-url.h"
#endif

struct ffmpeg_cfg {
	const char *url;
	const char *format_name;
	const char *format_mime_type;
	const char *muxer_settings;
	const char *protocol_settings; // not used yet for SRT nor RIST
	int gop_size;
	int video_bitrate;
	int audio_bitrate;
	const char *video_encoder;
	int video_encoder_id;
	const char *audio_encoder;
	int audio_encoder_id;
	int audio_bitrates[MAX_AUDIO_MIXES]; // multi-track
	const char *video_settings;
	const char *audio_settings;
	int audio_mix_count;
	int audio_tracks;
	const char *audio_stream_names[MAX_AUDIO_MIXES];
	enum AVPixelFormat format;
	enum AVColorRange color_range;
	enum AVColorPrimaries color_primaries;
	enum AVColorTransferCharacteristic color_trc;
	enum AVColorSpace colorspace;
	int max_luminance;
	int scale_width;
	int scale_height;
	int width;
	int height;
	int frame_size; // audio frame size
	const char *username;
	const char *password;
	const char *stream_id;
	const char *encrypt_passphrase;
	bool is_srt;
	bool is_rist;
	int srt_pkt_size;
};

struct ffmpeg_audio_info {
	AVStream *stream;
	AVCodecContext *ctx;
};

struct ffmpeg_data {
	AVStream *video;
	AVCodecContext *video_ctx;
	struct ffmpeg_audio_info *audio_infos;
	const AVCodec *acodec;
	const AVCodec *vcodec;
	AVFormatContext *output;
	struct SwsContext *swscale;

	int64_t total_frames;
	AVFrame *vframe;
	int frame_size;

	uint64_t start_timestamp;

	int64_t total_samples[MAX_AUDIO_MIXES];
	uint32_t audio_samplerate;
	enum audio_format audio_format;
	size_t audio_planes;
	size_t audio_size;
	int num_audio_streams;

	/* audio_tracks is a bitmask storing the indices of the mixes */
	int audio_tracks;
	struct deque excess_frames[MAX_AUDIO_MIXES][MAX_AV_PLANES];
	uint8_t *samples[MAX_AUDIO_MIXES][MAX_AV_PLANES];
	AVFrame *aframe[MAX_AUDIO_MIXES];

	struct ffmpeg_cfg config;

	bool initialized;

	char *last_error;
};

struct ffmpeg_output {
	obs_output_t *output;
	volatile bool active;
	struct ffmpeg_data ff_data;

	bool connecting;
	pthread_t start_thread;

	uint64_t total_bytes;

	uint64_t audio_start_ts;
	uint64_t video_start_ts;
	uint64_t stop_ts;
	volatile bool stopping;

	bool write_thread_active;
	pthread_mutex_t write_mutex;
	pthread_t write_thread;
	os_sem_t *write_sem;
	os_event_t *stop_event;

	DARRAY(AVPacket *) packets;
#ifdef NEW_MPEGTS_OUTPUT
	/* used for SRT & RIST */
	URLContext *h;
	AVIOContext *s;
	bool got_headers;

	volatile bool running;

	pthread_t start_stop_thread;
	pthread_mutex_t start_stop_mutex;
	volatile bool start_stop_thread_active;
	bool has_connected;
#endif
};

#ifdef NEW_MPEGTS_OUTPUT
enum mpegts_cmd_type { MPEGTS_CMD_START, MPEGTS_CMD_STOP };

struct mpegts_cmd {
	enum mpegts_cmd_type type;
	bool signal_stop;
	struct ffmpeg_output *stream;
	uint64_t ts;
};
#endif
bool ffmpeg_data_init(struct ffmpeg_data *data, struct ffmpeg_cfg *config);
void ffmpeg_data_free(struct ffmpeg_data *data);
