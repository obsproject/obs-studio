#include <obs-module.h>
#include <obs-avc.h>
#include <util/platform.h>
#include <util/circlebuf.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <inttypes.h>
#include "librtmp/rtmp.h"
#include "librtmp/log.h"
#include "flv-mux.h"
#include "net-if.h"

#ifdef _WIN32
#include <Iphlpapi.h>
#else
#include <sys/ioctl.h>
#endif

#define do_log(level, format, ...) \
	blog(level, "[rtmp stream: '%s'] " format, \
			obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

#define OPT_DROP_THRESHOLD "drop_threshold_ms"
#define OPT_PFRAME_DROP_THRESHOLD "pframe_drop_threshold_ms"
#define OPT_MAX_SHUTDOWN_TIME_SEC "max_shutdown_time_sec"
#define OPT_BIND_IP "bind_ip"
#define OPT_NEWSOCKETLOOP_ENABLED "new_socket_loop_enabled"
#define OPT_LOWLATENCY_ENABLED "low_latency_mode_enabled"
#define OPT_DYN_BITRATE "dynamic_bitrate"
#define OPT_DYN_BITRATE_DOWN "dynamic_bitrate_down"
#define OPT_DYN_BITRATE_UP "dynamic_bitrate_up"
#define OPT_DYN_BITRATE_THRESHOLD "dynamic_bitrate_threshold"
#define OPT_DYN_BITRATE_REC_TIME_MS "dynamic_bitrate_recovery_time_ms"
#define OPT_DYN_BITRATE_DEC_TIME_MS "dynamic_bitrate_decrease_time_ms"
#define OPT_DYN_BITRATE_FLOOR "dynamic_bitrate_floor"

//#define TEST_FRAMEDROPS

#ifdef TEST_FRAMEDROPS
#define SEC_TO_NS 1000000000ULL
#define DROPTEST_MAX_KBPS 3000
#define DROPTEST_MAX_BYTES (DROPTEST_MAX_KBPS * 1000 / 8)

struct droptest_info {
	uint64_t ts;
	size_t size;
};
#endif

struct rtmp_stream {
	obs_output_t     *output;

	pthread_mutex_t  packets_mutex;
	struct circlebuf packets;
	bool             sent_headers;

	bool             got_first_video;
	int64_t          start_dts_offset;

	volatile bool    connecting;
	pthread_t        connect_thread;

	volatile bool    active;
	volatile bool    disconnected;
	pthread_t        send_thread;

	int              max_shutdown_time_sec;

	os_sem_t         *send_sem;
	os_event_t       *stop_event;
	uint64_t         stop_ts;
	uint64_t         shutdown_timeout_ts;

	struct dstr      path, key;
	struct dstr      username, password;
	struct dstr      encoder_name;
	struct dstr      bind_ip;

	/* frame drop variables */
	int64_t          drop_threshold_usec;
	int64_t          pframe_drop_threshold_usec;
	int              min_priority;
	float            congestion;

	int64_t          last_dts_usec;

	uint64_t         total_bytes_sent;
	int              dropped_frames;

#ifdef TEST_FRAMEDROPS
	struct circlebuf droptest_info;
	size_t           droptest_size;
#endif

	/* dynamic bitrate variables */
	bool             enable_dyn_br;
	int              dynamic_br;
	int              initial_br;
	uint64_t         last_adj_time;
	float            last_congest;
	float            mean_congest;
	size_t           congest_idx;
	bool             inc_just_attempted;
	bool             allow_inc_earlier;
	int              br_dec_rate;   /* percentage at which bitrate will
					   decrease every dec_poll_time. */
	int              br_inc_rate;   /* percentage at which bitrate will
					   increase every inc_poll_time. */
	uint64_t         inc_poll_time; /* time in milliseconds after which a
					   bitrate increase can be attempted
					   (if congestion has stopped). */
	uint64_t         dec_poll_time; /* time in milliseconds between allowed
					   decreases in bitrate (if congestion
					   has been detected). */
	int              dyn_threshold; /* congestion threshold percentage in
					   which bitrate is decreased. */
	int              br_floor;      /* minimum percentage of initial
					   bitrate that it can be lowered to
					   (50% on default). */
	enum br_state    br_state;      /* stores the dynamic bitrate state for
					   UI status bar. */

	RTMP             rtmp;

	bool             new_socket_loop;
	bool             low_latency_mode;
	bool             disable_send_window_optimization;
	bool             socket_thread_active;
	pthread_t        socket_thread;
	uint8_t          *write_buf;
	size_t           write_buf_len;
	size_t           write_buf_size;
	pthread_mutex_t  write_buf_mutex;
	os_event_t       *buffer_space_available_event;
	os_event_t       *buffer_has_data_event;
	os_event_t       *socket_available_event;
	os_event_t       *send_thread_signaled_exit;
};

#ifdef _WIN32
void *socket_thread_windows(void *data);
#endif
