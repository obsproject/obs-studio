#pragma once
#include <obs-module.h>
#include <util/deque.h>
#include <util/threading.h>
#include <util/dstr.h>
#include <util/darray.h>
#include <util/platform.h>

#include <libavutil/avutil.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avassert.h>
#include <libavutil/avstring.h>
#include <libavutil/parseutils.h>
#include <libavutil/time.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mastering_display_metadata.h>

#include <stdio.h>
#include <time.h>

#define HTTP_PROTO "http"
#define RIST_PROTO "rist"
#define SRT_PROTO "srt"
#define TCP_PROTO "tcp"
#define UDP_PROTO "udp"

/* lightened version of a struct used by avformat */
typedef struct URLContext {
	void *priv_data; /* SRTContext or RISTContext */
	char *url;       /* URL */
	int max_packet_size;
	AVIOInterruptCB interrupt_callback;
	int64_t rw_timeout; /* max time to wait for write completion in mcs */
} URLContext;

struct mpegts_cfg {
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
};

struct mpegts_audio_info {
	AVStream *stream;
	AVCodecContext *ctx;
};

struct mpegts_data {
	AVStream *video;
	AVCodecContext *video_ctx;
	struct mpegts_audio_info *audio_infos;
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

	struct mpegts_cfg config;

	bool initialized;

	char *last_error;
};

struct mpegts_output {
	obs_output_t *output;
	volatile bool active;
	struct mpegts_data ff_data;

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
	/* used for SRT & RIST */
	URLContext *h;
	AVIOContext *s;
	bool got_headers;
};

#define UDP_DEFAULT_PAYLOAD_SIZE 1316

/* We need to override libsrt/win/syslog_defs.h due to conflicts w/ some libobs
 * definitions.
 */
#ifndef _WIN32
#define _SYS_SYSLOG_H
#endif

#ifdef _WIN32
#ifndef INC_SRT_WINDOWS_SYSLOG_DEFS_H
#define INC_SRT_WINDOWS_SYSLOG_DEFS_H

#define LOG_EMERG 0
#define LOG_ALERT 1
#define LOG_CRIT 2
#define LOG_ERR 3
//#define LOG_WARNING 4 // this creates issues w/ libobs LOG_WARNING = 200
#define LOG_NOTICE 5
//#define LOG_INFO 6 // issue w/ libobs
//#define LOG_DEBUG 7 // issue w/ libobs
#define LOG_PRIMASK 0x07

#define LOG_PRI(p) ((p)&LOG_PRIMASK)
#define LOG_MAKEPRI(fac, pri) (((fac) << 3) | (pri))

#define LOG_KERN (0 << 3)
#define LOG_USER (1 << 3)
#define LOG_MAIL (2 << 3)
#define LOG_DAEMON (3 << 3)
#define LOG_AUTH (4 << 3)
#define LOG_SYSLOG (5 << 3)
#define LOG_LPR (6 << 3)
#define LOG_NEWS (7 << 3)
#define LOG_UUCP (8 << 3)
#define LOG_CRON (9 << 3)
#define LOG_AUTHPRIV (10 << 3)
#define LOG_FTP (11 << 3)

/* Codes through 15 are reserved for system use */
#define LOG_LOCAL0 (16 << 3)
#define LOG_LOCAL1 (17 << 3)
#define LOG_LOCAL2 (18 << 3)
#define LOG_LOCAL3 (19 << 3)
#define LOG_LOCAL4 (20 << 3)
#define LOG_LOCAL5 (21 << 3)
#define LOG_LOCAL6 (22 << 3)
#define LOG_LOCAL7 (23 << 3)

#define LOG_NFACILITIES 24
#define LOG_FACMASK 0x03f8
#define LOG_FAC(p) (((p)&LOG_FACMASK) >> 3)
#endif
#endif

/* We need to override libsrt/logging_api.h due to conflicts with some libobs
 * definitions.
 */

#define INC_SRT_LOGGING_API_H

// These are required for access functions:
// - adding FA (requires set)
// - setting a log stream (requires iostream)
#ifdef __cplusplus
#include <set>
#include <iostream>
#endif

#ifndef _WIN32
#include <syslog.h>
#endif

// Syslog is included so that it provides log level names.
// Haivision log standard requires the same names plus extra one:
#ifndef LOG_DEBUG_TRACE
#define LOG_DEBUG_TRACE 8
#endif

// Flags
#define SRT_LOGF_DISABLE_TIME 1
#define SRT_LOGF_DISABLE_THREADNAME 2
#define SRT_LOGF_DISABLE_SEVERITY 4
#define SRT_LOGF_DISABLE_EOL 8

// Handler type
typedef void SRT_LOG_HANDLER_FN(void *opaque, int level, const char *file,
				int line, const char *area,
				const char *message);

#ifdef __cplusplus
namespace srt_logging {

struct LogFA {
private:
	int value;

public:
	operator int() const { return value; }

	LogFA(int v) : value(v) {}
};

const LogFA LOGFA_GENERAL = 0;

namespace LogLevel {
enum type {
	fatal = LOG_CRIT,
	error = LOG_ERR,
	warning = 4, //issue w/ libobs so LOG_WARNING is removed
	note = LOG_NOTICE,
	debug = 7 //issue w/ libobs so LOG_DEBUG is removed
};
}
class Logger;
} // namespace srt_logging
#endif
