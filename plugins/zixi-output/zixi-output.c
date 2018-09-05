#include <obs-module.h>
#include <obs-avc.h>
#include <obs-encoder.h>
#include <util/platform.h>
#include <util/circlebuf.h>
#include <util/dstr.h>
#include <util/threading.h>
#include <inttypes.h>

#include "zixi-constants.h"

#include "include/zixi_feeder_interface.h"

#define do_log(level, format, ...) \
	blog(level, "[zixi stream: '%s'] " format, \
			obs_output_get_name(stream->output), ##__VA_ARGS__)

#define warn(format, ...)  do_log(LOG_WARNING, format, ##__VA_ARGS__)
#define info(format, ...)  do_log(LOG_INFO,    format, ##__VA_ARGS__)
#define debug(format, ...) do_log(LOG_DEBUG,   format, ##__VA_ARGS__)

#define TIME_TO_CLEAR_CONGESTION_NS 5000000000
#define STATS_QUERY_INTERVAL_NS		1000000000

struct zixi_stream_source_control {
	bool			encoder_feedback;
	unsigned int	last_sent_encoder_feedback;
	volatile bool	safe_to_event;
	volatile bool   can_send_encoder_feedback;
	unsigned int	minimal_bitrate;
	float			decimation_factor;
	uint64_t		total_raw_frames;
	uint64_t		sent_to_encoder_frames;
};

struct zixi_stream {
	obs_output_t		*output;

	pthread_mutex_t  packets_mutex;
	struct circlebuf packets;

	volatile bool    connecting;
	pthread_t        connect_thread;

	volatile bool    active;
	volatile bool    disconnected;
	pthread_t        send_thread;

	os_sem_t         *send_sem;
	os_event_t       *stop_event;

	struct dstr      url;
	struct dstr		 key;
	struct dstr      encoder_name;
	struct dstr		 host;
	struct dstr		 password;
	struct dstr		 channel_name;
	short			 port;
	int				 latency_id;
	int				 encryption_type;
	unsigned int	 audio_bitrate;
	unsigned int	 video_bitrate;
	unsigned int     max_video_bitrate;

	unsigned int	 packet_alloc;
	unsigned int	 packet_free;

	/* bonding */
	bool				 bonding;
	
	/* encoder feedback */
	bool								encoder_feedback_enabled;
	struct zixi_stream_source_control	encoder_control;
	pthread_mutex_t						encoder_control_mutex;

	/* frame drop variables */
	int64_t          drop_threshold_usec;
	int64_t          min_drop_dts_usec;
	int              min_priority;

	int64_t          last_dts_usec;

	uint64_t         total_bytes_sent;
	int              dropped_frames;

	void *		     zixi_handle;

	uint64_t		 auto_bonding_last_time_scan;

	/* auto rtmp */
	bool			use_auto_rtmp;
	struct dstr		auto_rtmp_url;
	struct dstr		auto_rtmp_channel;
	struct dstr		auto_rtmp_username;
	struct dstr		auto_rtmp_password;
	uint32_t		audio_encoder_channels;
	uint32_t		audio_encoder_sample_rate;

	bool is_x265;


	/* congestion reporting */
	uint64_t	last_statistics_query_ts;
	uint64_t	last_dropped_packets;
	uint64_t	now_dropped_packets;
	uint64_t	congested_start_ts;

#ifdef ZIXI_ADVANCED
	bool	use_advanced;
//	bool	low_latency_muxing;
	int		fec_overhead;
	int		fec_size_ms;
	int		zixi_log_level;
#endif
#ifdef H264_DUMP
	FILE*	file;
#endif
};
static char MY_MACHINE_ID[255] = { 0 };
static void fill_machine_id();

static inline unsigned int zixi_convert_encryption(int encryption) {
	if (encryption == 0)
		return 3;
	else if (encryption == 4)
		return 4;
	else
		return encryption - 1;
}
static inline bool stopping(struct zixi_stream *stream)
{
	return os_event_try(stream->stop_event) != EAGAIN;
}
static inline bool connecting(struct zixi_stream* stream)
{
	return os_atomic_load_bool(&stream->connecting);
}
static inline bool active(struct zixi_stream *stream)
{
	return os_atomic_load_bool(&stream->active);
}

static inline bool disconnected(struct zixi_stream *stream)
{
	return os_atomic_load_bool(&stream->disconnected);
}

static inline size_t num_buffered_packets(struct zixi_stream *stream)
{
	return stream->packets.size / sizeof(struct encoder_packet);
}
static inline bool reset_semaphore(struct zixi_stream *stream)
{
	os_sem_destroy(stream->send_sem);
	return os_sem_init(&stream->send_sem, 0) == 0;
}

static void zixi_auto_bonding_scan(struct zixi_stream* stream){
	char ** strs = NULL;
	int count = 0;
	if (stream->bonding && zixi_set_automatic_ips(stream->zixi_handle) == ZIXI_ERROR_OK) {
		
	}
}
static void zixi_encoder_feedback(int total_bps, bool force_iframe, void* param){
	struct zixi_stream* stream = (struct zixi_stream*)param;
	
}
static inline bool get_next_packet(struct zixi_stream *stream,
struct encoder_packet *packet)
{
	bool new_packet = false;

	pthread_mutex_lock(&stream->packets_mutex);
	if (stream->packets.size) {
		circlebuf_pop_front(&stream->packets, packet,
			sizeof(struct encoder_packet));
		new_packet = true;
	}
	pthread_mutex_unlock(&stream->packets_mutex);

	return new_packet;
}
static inline void free_packets(struct zixi_stream *stream)
{
	size_t num_packets;

	pthread_mutex_lock(&stream->packets_mutex);

	num_packets = num_buffered_packets(stream);
	if (num_packets)
		info("Freeing %d remaining packets", (int)num_packets);

	while (stream->packets.size) {
		struct encoder_packet packet;
		circlebuf_pop_front(&stream->packets, &packet, sizeof(packet));
		obs_encoder_packet_release(&packet);
	}
	pthread_mutex_unlock(&stream->packets_mutex);
}

static uint64_t zixi_convert_ts(uint64_t ts, int64_t num, int64_t den) {
	uint64_t res = ts * 90000;
	res /= den;
	return res;
}

static int send_packet(struct zixi_stream *stream, struct encoder_packet *packet, bool is_header, size_t idx)
{
	size_t  size;
	int     recv_size = 0;
	int     ret = 0;
	
	size = packet->size;
#ifdef H264_DUMP
	uint8_t *data;
	unsigned int written = 0;
	unsigned int t = 0;
	do {
		t = fwrite(packet->data + written, 1, packet->size - written, stream->file);
		written += t;
	} while (written < packet->size);
	fflush(stream->file);
#endif
	
	if (packet->type == OBS_ENCODER_AUDIO) {
		packet->pts = zixi_convert_ts(packet->pts, packet->timebase_num, packet->timebase_den);
		packet->dts = zixi_convert_ts(packet->dts, packet->timebase_num, packet->timebase_den);
	} else {
		if (packet->dts >= 0)
			packet->dts = zixi_convert_ts(packet->dts, packet->timebase_num, packet->timebase_den);
		else
			packet->dts = 0;

		packet->pts = zixi_convert_ts(packet->pts, packet->timebase_num, packet->timebase_den);
	}
	
	//	info("zixi_send -> %s [%u / %u]", packet->type == OBS_ENCODER_VIDEO ? "video" : "audio", packet->pts, packet->dts);
	ret = zixi_send_elementary_frame(stream->zixi_handle, packet->data, packet->size, packet->type == OBS_ENCODER_VIDEO, packet->pts, packet->dts);
	
	if ((os_gettime_ns() - stream->last_statistics_query_ts) > STATS_QUERY_INTERVAL_NS) {
		ZIXI_ERROR_CORRECTION_STATS stats = { 0 };
		if (zixi_get_stats(stream->zixi_handle, NULL, NULL, &stats) == ZIXI_ERROR_OK){
			stream->last_statistics_query_ts = os_gettime_ns();
			stream->last_dropped_packets = stream->now_dropped_packets;
			stream->now_dropped_packets = stats.not_recovered;
			//info("get statistics %d -> %d", stream->last_dropped_packets, stream->now_dropped_packets);
		}
	}
	if (ret != ZIXI_ERROR_OK && ret != ZIXI_WARNING_OVER_LIMIT && ret != ZIXI_ERROR_NOT_READY) {
		warn("zixi_send_elementary_frame %d", ret);
		os_atomic_set_bool(&stream->encoder_control.safe_to_event, false);
	} else {
		ret = ZIXI_ERROR_OK;
	}

	stream->packet_free++;
	obs_encoder_packet_release(packet);
	if (ret == ZIXI_ERROR_OK)
		stream->total_bytes_sent += size;
	if (ret > 0)
		ret *= -1;
	
	
	if (stream->auto_bonding_last_time_scan == -1 || (os_gettime_ns() - stream->auto_bonding_last_time_scan) > TIME_BETWEEN_AUTO_BOND_SCAN_US) {
		uint64_t d = os_gettime_ns() - stream->auto_bonding_last_time_scan;
		stream->auto_bonding_last_time_scan = os_gettime_ns();
		zixi_auto_bonding_scan(stream);
	} 
	return ret;
}
static bool send_remaining_packets(struct zixi_stream *stream)
{
	struct encoder_packet packet;
	uint64_t begin_time_ns = os_gettime_ns();

	while (get_next_packet(stream, &packet)) {
		if (send_packet(stream, &packet, false, packet.track_idx) < 0) {
			return false;
		}
			
		obs_encoder_packet_release(&packet);
		
		/* Just disconnect if it takes too long to shut down */
		//if ((os_gettime_ns() - begin_time_ns) > max_ns) {
		//	info("Took longer than %d second(s) to shut down, "
		//		"automatically stopping connection",
		//		stream->max_shutdown_time_sec);
		//	return false;
		//}
	}

	return true;
}
static void drop_frames(struct zixi_stream *stream)
{
	struct circlebuf new_buf = { 0 };
	int              drop_priority = 0;
	uint64_t         last_drop_dts_usec = 0;
	int              num_frames_dropped = 0;

	debug("Previous packet count: %d", (int)num_buffered_packets(stream));

	circlebuf_reserve(&new_buf, sizeof(struct encoder_packet) * 8);

	while (stream->packets.size) {
		struct encoder_packet packet;
		circlebuf_pop_front(&stream->packets, &packet, sizeof(packet));

		last_drop_dts_usec = packet.dts_usec;

		/* do not drop audio data or video keyframes */
		if (packet.type == OBS_ENCODER_AUDIO ||
			packet.drop_priority == OBS_NAL_PRIORITY_HIGHEST) {
			circlebuf_push_back(&new_buf, &packet, sizeof(packet));

		}
		else {
			if (drop_priority < packet.drop_priority)
				drop_priority = packet.drop_priority;

			num_frames_dropped++;
			obs_encoder_packet_release(&packet);
		}
	}

	circlebuf_free(&stream->packets);
	stream->packets = new_buf;
	stream->min_priority = drop_priority;
	stream->min_drop_dts_usec = last_drop_dts_usec;

	stream->dropped_frames += num_frames_dropped;
	debug("New packet count: %d", (int)num_buffered_packets(stream));
}
static void zixi_log_callback(void * user_data, int level, const char * what) {
	blog(LOG_INFO, what);
}

static bool init_connect(struct zixi_stream* stream) {
	obs_service_t *service;
	obs_data_t *settings;
	info("zixi_init_connect");
	if (stopping(stream)){
		info("zixi_init_connect need to join send_thread");
		pthread_join(stream->send_thread, NULL);
		info("zixi_init_connect need to send_thread joinned");
	}

	free_packets(stream);

	service = obs_output_get_service(stream->output);
	if (!service){
		info("zixi_init_connect failed to get service");
		return false;
	}

	settings = obs_service_get_settings(service);
	stream->bonding = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_ENABLE_BONDING);
	info("zixi-output: bonding from service %s",  stream->bonding ? "bonding on": "bonding off");
	
	os_atomic_set_bool(&stream->disconnected, false);
	stream->total_bytes_sent = 0;
	stream->dropped_frames = 0;
	stream->min_drop_dts_usec = 0;
	stream->min_priority = 0;
	stream->encoder_control.encoder_feedback = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_USE_ENCODER_FEEDBACK);
	stream->encoder_control.total_raw_frames = 0;
	stream->encoder_control.sent_to_encoder_frames = 0;
	stream->encoder_control.decimation_factor = 1.0f;
	dstr_copy(&stream->url, obs_data_get_string(settings, ZIXI_SERVICE_PROP_URL));
	dstr_copy(&stream->password, obs_data_get_string(settings, ZIXI_SERVICE_PROP_PASSWORD));
	stream->latency_id = obs_data_get_int(settings, ZIXI_SERVICE_PROP_LATENCY_ID);
	stream->encryption_type = zixi_convert_encryption(obs_data_get_int(settings, ZIXI_SERVICE_PROP_ENCRYPTION_TYPE));
	if (stream->encryption_type != ZIXI_NO_ENCRYPTION)
		dstr_copy(&stream->key, obs_data_get_string(settings, ZIXI_SERVICE_PROP_ENCRYPTION_KEY));
		
	stream->use_auto_rtmp = obs_data_get_bool(settings, ZIXI_SERVICE_PROP_USE_AUTO_RTMP_OUT);
	if (stream->use_auto_rtmp) {
		dstr_copy(&stream->auto_rtmp_url, obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_URL));
		dstr_copy(&stream->auto_rtmp_username, obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_USERNAME));
		dstr_copy(&stream->auto_rtmp_password, obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_PASSWORD));
		dstr_copy(&stream->auto_rtmp_channel, obs_data_get_string(settings, ZIXI_SERVICE_PROP_AUTO_RTMP_CHANNEL));
	}
#ifdef ZIXI_ADVANCED
	stream->use_advanced = obs_data_get_bool(settings, ZIXI_SERVICE_ADV_USE);
	if (stream->use_advanced) {
		stream->fec_overhead = obs_data_get_int(settings, ZIXI_SERVICE_FEC_OVERHEAD);
		stream->fec_size_ms = obs_data_get_int(settings, ZIXI_SERVICE_FEC_MS);
		stream->zixi_log_level = obs_data_get_int(settings, ZIXI_SERVICE_PROP_ZIXI_LOG_LEVEL);
//		stream->low_latency_muxing = obs_data_get_bool(settings, ZIXI_LOW_LATENCY_MUXING);
	}
#endif
	obs_data_release(settings);
	info("zixi_init_connect done");
	return true;
}

static void * send_thread(void * data) {
	struct zixi_stream * stream = (struct zixi_stream *)data;

	os_set_thread_name("zixi-stream: send_thread");
	info("zixi send thread started");
	stream->encoder_control.can_send_encoder_feedback = true;
	while (os_sem_wait(stream->send_sem) == 0) {
		struct encoder_packet packet;

		if (stopping(stream))
			break;
		if (!get_next_packet(stream, &packet))
			continue;

		if (send_packet(stream, &packet, false, packet.track_idx) < 0) {
			os_atomic_set_bool(&stream->disconnected, true);
			break;
		}
	}
	info("zixi send thread work done");
	if (!disconnected(stream)) {
		info("zixi send thread setting disconnected");
		os_atomic_set_bool(&stream->disconnected, true);
		//send_remaining_packets(stream);
	}

	if (disconnected(stream)) {
		info("Disconnected from %s", stream->url.array);
		free_packets(stream);
	} else {
		info("User stopped the stream");
	}

	info("zixi send thread zixi_close_stream");
	zixi_close_stream(stream->zixi_handle);
	stream->zixi_handle = NULL;
	info("zixi send thread zixi_close_stream done");

	if (!stopping(stream)) {
		info("zixi send thread detaching");
		pthread_detach(stream->send_thread);
		info("zixi send thread notify output disconnected");
		obs_output_signal_stop(stream->output, OBS_OUTPUT_DISCONNECTED);
	}

	os_event_reset(stream->stop_event);
	os_atomic_set_bool(&stream->active, false);
	info("zixi send thread done");
	pthread_exit(NULL);
	return NULL;
}
static int init_send(struct zixi_stream * stream) {
	int ret;
	size_t idx = 0;
	info ("zixi init send");
	reset_semaphore(stream);
#ifdef H264_DUMP
	stream->file = fopen("c:/dev/dump.h264", "w");
#endif
	info ("zixi init send creating send thread");
	
	// Race we could be called here after the following
	// init connect [Takes loads of time]
	// UI kills 
	// connect is returning now
	if (!stopping(stream)) {
		ret = pthread_create(&stream->send_thread, NULL, send_thread, stream);
		if (ret != 0) {

#ifdef H264_DUMP
			fclose(stream->file);
#endif
			zixi_close_stream(stream->zixi_handle);
			stream->zixi_handle = NULL;
			warn("Failed to create send thread");
			return OBS_OUTPUT_ERROR;
		}

		os_atomic_set_bool(&stream->active, true);
		info("zixi init send notify start data capture");
		obs_output_begin_data_capture(stream->output, 0);
		info("zixi init send done");
	}
	return OBS_OUTPUT_SUCCESS;
}

static inline int zixi_latency_from_id(unsigned int id) {
	int r = 2000;
	static unsigned int LATENCIES[] = { 100, 200, 300, 500, 100, 1500, 2000, 2500, 3000, 4000, 5000, 6000, 8000, 10000, 12000, 14000, 16000 };
	if (id >= 0 && id <= 16) {
		r = LATENCIES[id];
	}
	return r;
}

static bool zixi_input_control(void * p) {
	struct zixi_stream * stream = (struct zixi_stream*)p;
	bool ret = false;
	pthread_mutex_lock(&stream->encoder_control_mutex);
	float new_factor = ((float)(stream->encoder_control.sent_to_encoder_frames + 1)) / (stream->encoder_control.total_raw_frames + 1);
	if (new_factor <= stream->encoder_control.decimation_factor) {
		stream->encoder_control.sent_to_encoder_frames++;
		ret = true;
	} else {
		stream->dropped_frames++;
	}

	stream->encoder_control.total_raw_frames++;
	if (!ret) {
		info("zixi_input_control - dropping source frame %u/%u", stream->encoder_control.sent_to_encoder_frames, stream->encoder_control.total_raw_frames);
	}
	pthread_mutex_unlock(&stream->encoder_control_mutex);
	
	return ret;	
}
static bool zixi_parse_url(struct dstr * url, char ** host, short * port, char ** channel_name) {
	bool ret = false;
	bool have_port = false;
	bool have_channel_name = false;
	int  port_start, port_end;
	int	 channel_name_start, channel_name_end;
	int  host_start, host_end;
	// check prefix
	if (url && url->len > 7) {
		ret = url->array[0] == 'z' &&
			url->array[1] == 'i' &&
			url->array[2] == 'x' &&
			url->array[3] == 'i' &&
			url->array[4] == ':' &&
			url->array[5] == '/' &&
			url->array[6] == '/';
	}

	if (ret) {
		int start = 7;
		int iter = -1;
		ret = false;
		for ( iter = start; iter < url->len && !ret; iter++) {
			ret = url->array[iter] == ':';
		}
		if (ret) {
			host_start = start;
			host_end = iter -1;
			have_port = true;
			start = iter + 1;
			ret = false;
		}
		for (iter = start; iter < url->len && !ret; iter++) {
			ret = url->array[iter] == '/';
		}
		if (ret) {
			have_channel_name = true;
			if (!have_port) {
				host_start = 7;
				host_end = iter - 1;
				channel_name_start = iter;
				channel_name_end = url->len;
			} else {
				port_start = start - 1;
				port_end = iter - 1;
				channel_name_start = iter;
				channel_name_end = url->len;

			}
			if (host) {
				*host = bzalloc(host_end - host_start + 1);
				memcpy(*host, &url->array[host_start], host_end - host_start);
				(*host)[host_end - host_start] = 0;
			}
			if (channel_name) {
				*channel_name = bzalloc(channel_name_end - channel_name_start + 1);
				memcpy(*channel_name, &url->array[channel_name_start], channel_name_end - channel_name_start);
				(*channel_name)[channel_name_end - channel_name_start] = 0;
			}
			if (port) {
				if (have_port) {
					char temp[16]; // shouldnt be longer...
					memcpy(temp, &url->array[port_start], port_end - port_start);
					temp[port_end - port_start] = 0;
					*port = atoi(temp);
				} else
					*port = 2088;
			}
		}

	}

	return ret;
}
static int try_connect(struct zixi_stream* stream) {
	char * url_host = NULL;
	char * url_channel_name = NULL;
	short url_port = 2088;
	if (dstr_is_empty(&stream->url)) { 
		warn("URL is empty");
		return OBS_OUTPUT_BAD_PATH;
	}

	if (!zixi_parse_url(&stream->url, &url_host, &url_port, &url_channel_name)){
		warn("Failed to parse URL!");
		if (url_host) {
			bfree(url_host);
		}
		if (url_channel_name) {
			bfree(url_channel_name);
		}
		return OBS_OUTPUT_BAD_PATH;
	}
	
	dstr_copy(&stream->host, url_host);
	dstr_copy(&stream->channel_name, url_channel_name);
	bfree(url_host);
	bfree(url_channel_name);
	stream->port = url_port;
	info("Connecting to ZIXI URL %s...", stream->url.array);
	dstr_copy(&stream->encoder_name, "FMLE/3.0 (compatible; obs-studio/");

#ifdef HAVE_OBSCONFIG_H
	dstr_cat(&stream->encoder_name, OBS_VERSION);
#else
	dstr_catf(&stream->encoder_name, "%d.%d.%d",
		LIBOBS_API_MAJOR_VER,
		LIBOBS_API_MINOR_VER,
		LIBOBS_API_PATCH_VER);
#endif

	dstr_cat(&stream->encoder_name, "; FMSc/1.0)");
	fill_machine_id();
	zixi_stream_config cfg = { 0 };
	encoder_control_info * encoder_info = NULL;
	cfg.user_id = MY_MACHINE_ID;
	cfg.enc_type = stream->encryption_type;
	
	if (cfg.enc_type != ZIXI_NO_ENCRYPTION) {
		cfg.sz_enc_key = bzalloc(stream->key.len + 1);
		memcpy(cfg.sz_enc_key, stream->key.array, stream->key.len + 1);
	} else {
		cfg.sz_enc_key = NULL;
	}
	
	cfg.max_latency_ms = zixi_latency_from_id(stream->latency_id);
	cfg.port = (unsigned short *)malloc(1 * sizeof(unsigned short));
	cfg.port[0] = (unsigned short)stream->port;
	cfg.sz_stream_id = malloc(stream->channel_name.len + 1);
	memcpy(cfg.sz_stream_id, stream->channel_name.array, stream->channel_name.len + 1);
	cfg.stream_id_max_length = stream->channel_name.len;

	if (stream->password.len > 0)  {
		cfg.password = malloc(stream->password.len + 1);
		memcpy(cfg.password, stream->password.array, stream->password.len + 1);
	} else
		cfg.password = NULL;

	cfg.sz_hosts = (char **)malloc(1 * sizeof(char*));
	cfg.hosts_len = (int *)malloc(1 * sizeof(int));
	cfg.sz_hosts[0] = (char*)malloc(stream->host.len + 1);
	memcpy(cfg.sz_hosts[0], stream->host.array ,stream->host.len + 1);
	cfg.hosts_len[0] = stream->host.len;


	cfg.max_delay_packets = ((stream->video_bitrate + stream->audio_bitrate)) / (5* 8 * 188 * 7);
	cfg.max_bitrate = (int)(stream->max_video_bitrate + stream->audio_bitrate + 300000);

	cfg.reconnect = 0;
	cfg.num_hosts = 1;
	cfg.use_compression = 1;
	cfg.elementary_streams = 1;
	//cfg.mmt = 0;

	cfg.limited = ZIXI_ADAPTIVE_NONE;
	cfg.fec_overhead = 0;
	cfg.content_aware_fec = 0;
	cfg.fec_block_ms = 100;

	char * mega_log  = malloc(2555);
	int major, mid, minor,build;
	zixi_version(&major, &mid, &minor, &build);
	info("zixi-version: %d.%d.%d.%d", major, mid, minor, build);
	info("zixi-output::try_connect bonding is %s", stream->bonding?"bonding on":"bonding off");
	
	cfg.force_bonding = stream->bonding;
	cfg.local_nics = NULL;
	cfg.num_local_nics = 0;
	cfg.force_padding = false;
	stream->encoder_feedback_enabled = false;
	if (stream->encoder_control.encoder_feedback) {
		info("%s", "encoder_feedback");
		stream->encoder_feedback_enabled = true;
		os_atomic_set_bool(&stream->encoder_control.safe_to_event, true);
		stream->encoder_control.last_sent_encoder_feedback = 0;
		cfg.limited = ZIXI_ADAPTIVE_ENCODER;
		encoder_info = bzalloc(sizeof(encoder_control_info));
		encoder_info->aggressiveness = 20;
		encoder_info->max_bitrate = (stream->video_bitrate + stream->audio_bitrate) * 105 / 100;
		encoder_info->min_bitrate = encoder_info->max_bitrate / 4;
		encoder_info->param = stream;
		encoder_info->update_interval = 2000;
		encoder_info->setter = zixi_encoder_feedback;
		cfg.fec_block_ms = 100;
		cfg.fec_overhead = 5;
		cfg.force_padding = true;
	} else {
		if (cfg.max_latency_ms < 2000 || (stream->bonding)) {
			stream->encoder_control.encoder_feedback = false;
			cfg.fec_overhead = 30;
			cfg.content_aware_fec = 0;
			cfg.fec_block_ms = 100;
			cfg.limited = ZIXI_ADAPTIVE_FEC;
			info("%s latency is %d -> FEC", "no encoder_feedback ", cfg.max_latency_ms);
		} else if (cfg.max_latency_ms < 6000) {
			float factor = 1 - (((float)(cfg.max_latency_ms - 2000)) / 4000);
			cfg.limited = ZIXI_ADAPTIVE_FEC;
			cfg.fec_block_ms = 100;
			cfg.fec_overhead = (int )( factor * 30);
			cfg.content_aware_fec = 0;
			info("%s latency %d -> %d", "no encoder_feedback ", cfg.max_latency_ms, cfg.fec_overhead);
		} else if (stream->bonding) {
			cfg.limited = ZIXI_ADAPTIVE_FEC;
			cfg.fec_block_ms = 100;
			cfg.fec_overhead = 20;
			cfg.content_aware_fec = 0;
			info("%s latency %d WITH bonding -> 20", "no encoder_feedback ", cfg.max_latency_ms);
		} else {
			cfg.limited = ZIXI_ADAPTIVE_NONE;
			cfg.fec_block_ms = 0;
			cfg.fec_overhead = 0;
			cfg.content_aware_fec = 0;
			info("%s latency %d WITH bonding -> no fec + 10", "no fec", cfg.max_latency_ms);
		}
	} 
	
	cfg.enforce_bitrate = false;
	info("Bitrate is set @%u\n", cfg.max_bitrate);
	free(mega_log);
	
	if (stream->is_x265) {
		cfg.elementary_streams_config.video_codec = ZIXI_VIDEO_CODEC_HEVC;
	} else {
		cfg.elementary_streams_config.video_codec = ZIXI_VIDEO_CODEC_H264;
	}

	cfg.elementary_streams_max_va_diff_ms = 1000;
	cfg.elementary_streams_config.audio_codec = ZIXI_AUDIO_CODEC_AAC;
	cfg.elementary_streams_config.audio_channels = 2;
#ifdef ZIXI_ADVANCED
	if (stream->use_advanced) {
		zixi_configure_logging(stream->zixi_log_level, zixi_log_callback, NULL);
	} else {
#endif
		zixi_configure_logging(ZIXI_LOG_ERRORS, zixi_log_callback, NULL);
#ifdef ZIXI_ADVANCED
	}
#endif
	//zixi_configure_logging(0, zixi_log_callback, NULL);
	int zixi_ret = -1;
	stream->encoder_control.decimation_factor = 1.0f;
#ifdef ZIXI_ADVANCED
	if (stream->use_advanced) {
		cfg.fec_block_ms = stream->fec_size_ms;
		cfg.fec_overhead = stream->fec_overhead;
		info("Advanced settings fec %d, %d ms", cfg.fec_block_ms, cfg.fec_overhead);
	}
#endif
	if (stream->use_auto_rtmp) {
		zixi_rtmp_out_config rtmp_cfg = { 0 };
		rtmp_cfg.max_va_diff = 10000;
		rtmp_cfg.bitrate = stream->video_bitrate + stream->audio_bitrate;

		rtmp_cfg.url = malloc(stream->auto_rtmp_url.len + 1);
		memcpy(rtmp_cfg.url, stream->auto_rtmp_url.array, stream->auto_rtmp_url.len);
		rtmp_cfg.url[stream->auto_rtmp_url.len] = 0;

		rtmp_cfg.stream_name = malloc(stream->auto_rtmp_channel.len +1);
		memcpy(rtmp_cfg.stream_name, stream->auto_rtmp_channel.array, stream->auto_rtmp_channel.len);
		rtmp_cfg.stream_name[stream->auto_rtmp_channel.len] = 0;

		if (stream->auto_rtmp_password.array) {
			rtmp_cfg.password = malloc(stream->auto_rtmp_password.len +1);
			memcpy(rtmp_cfg.password, stream->auto_rtmp_password.array, stream->auto_rtmp_password.len);
			rtmp_cfg.password[stream->auto_rtmp_password.len] = 0;
		} else {
			rtmp_cfg.password = NULL;
		}
		if (stream->auto_rtmp_username.array) {
			rtmp_cfg.user = malloc(stream->auto_rtmp_username.len +1);
			memcpy(rtmp_cfg.user, stream->auto_rtmp_username.array, stream->auto_rtmp_username.len);
			rtmp_cfg.user[stream->auto_rtmp_username.len] = 0;
		} else {
			rtmp_cfg.user = NULL;
		}
		zixi_ret = zixi_open_stream_with_rtmp(cfg, encoder_info, &rtmp_cfg, &stream->zixi_handle);
		free(rtmp_cfg.url);
		free(rtmp_cfg.stream_name);
		if (rtmp_cfg.user) {
			free(rtmp_cfg.user);
		}
		if (rtmp_cfg.password) {
			free(rtmp_cfg.password);
		}
	} else {
		zixi_ret = zixi_open_stream(cfg, encoder_info, &stream->zixi_handle);
	}
	

	if (encoder_info)
		bfree(encoder_info);

	free(cfg.sz_hosts[0]);
	free(cfg.sz_hosts);
	free(cfg.hosts_len);
	free(cfg.port);
	free(cfg.sz_stream_id);
	if (cfg.password) 
		free(cfg.password);
	
	if (zixi_ret) {
		warn("Zixi returned %d - no init!", zixi_ret);
		return zixi_ret;
	}

	return init_send(stream);
}
static void * connect_thread(void * data) {
	struct zixi_stream * stream = (struct zixi_stream *)data;

	os_set_thread_name("zixi-stream: connect_thread");
	info("zixi connect thread started");
	if (!init_connect(stream)) {
		obs_output_signal_stop(stream->output, OBS_OUTPUT_BAD_PATH);
		return NULL;
	}

	int ret = try_connect(stream);

	if (ret != OBS_OUTPUT_SUCCESS) {
		obs_output_signal_stop(stream->output, ret);
		info("Connection to %s failed: %d", stream->url.array, ret);
	}

	if (!stopping(stream))
		pthread_detach(stream->connect_thread);

	os_atomic_set_bool(&stream->connecting, false);
	info("zixi connect thread done");
	pthread_exit(NULL);
	return NULL;
}

static const char *zixi_stream_getname(void *unused){
	return obs_module_text("ZIXIStream");
}

static void zixi_stream_destroy(void * data) {
	struct zixi_stream* stream = (struct zixi_stream*)data;
	info("zixi_stream_destroy start");
	info("zixi_stream_destroy alloc/free %d/%d", stream->packet_alloc, stream->packet_free);
	if (stopping(stream) && !connecting(stream)) {
		info("zixi_stream_destroy joinning connect thread");
		pthread_join(stream->connect_thread, NULL);
		info("zixi_stream_destroy connect thread joinned");
	} else if (connecting(stream) || active(stream)) {
		if (stream->connecting) {
			info("zixi_stream_destroy joinning connect thread 2");
			pthread_join(stream->connect_thread, NULL);
			info("zixi_stream_destroy connect thread joinned 2");
		}

		os_event_signal(stream->stop_event);

		if (active(stream)) {
			os_sem_post(stream->send_sem);
			obs_output_end_data_capture(stream->output);
			info("zixi_stream_destroy joinning send thread");
			pthread_join(stream->send_thread, NULL);
			info("zixi_stream_destroy send thread joinned");
		} else {
			info("zixi_stream_destroy stream not active bb");
		}
	}

	if (stream) {
		free_packets(stream);
		dstr_free(&stream->url);
		dstr_free(&stream->key);
		dstr_free(&stream->encoder_name);
		dstr_free(&stream->host);
		dstr_free(&stream->channel_name);
		dstr_free(&stream->password);
		os_event_destroy(stream->stop_event);
		os_sem_destroy(stream->send_sem);
		pthread_mutex_destroy(&stream->packets_mutex);
		circlebuf_free(&stream->packets);
		pthread_mutex_destroy(&stream->encoder_control_mutex);
#ifdef H264_DUMP
		fclose(stream->file);
#endif
		bfree(stream);
	}
}

static void *zixi_stream_create(obs_data_t* settings, obs_output_t* output) {
	struct zixi_stream * stream = bzalloc(sizeof(struct zixi_stream));
	stream->is_x265 = false;
	stream->output = output;
	stream->packet_alloc = 0;
	stream->packet_free = 0;
	stream->drop_threshold_usec = 1000000000;
	pthread_mutex_init_value(&stream->packets_mutex);
	pthread_mutex_init_value(&stream->encoder_control_mutex);
	//zixi_configure_logging(ZIXI_LOG_ALL, zixi_log_callback, NULL);
	if (pthread_mutex_init(&stream->packets_mutex, NULL) != 0)
		goto fail;
	if (pthread_mutex_init(&stream->encoder_control_mutex, NULL) != 0)
		goto fail;
	if (os_event_init(&stream->stop_event, OS_EVENT_TYPE_MANUAL) != 0) {
		pthread_mutex_destroy(&stream->packets_mutex);
		goto fail;
	}
    
    info("os_event_init post");
	//UNUSED_PARAMETER(settings);
	return stream;
fail:
	zixi_stream_destroy(stream);
	return NULL;
}

static void zixi_stream_stop(void * data, uint64_t what) {
	struct zixi_stream* stream = (struct zixi_stream*)data;
	info("zixi_stream_stop start");
	if (stopping(stream)) {
		info("zixi_stream_stop already started");
		return;
	}

	if (connecting(stream)) {
		info("zixi_stream_stop connecting -> joining connect thread");
		pthread_join(stream->connect_thread, NULL);
		info("zixi_stream_stop connecting -> connect thread joined");
	}

	info("zixi_stream_stop connecting signal stop");
	os_event_signal(stream->stop_event);

	if (active(stream)) {
		stream->encoder_control.can_send_encoder_feedback = false;
		obs_output_end_data_capture(stream->output);
		os_sem_post(stream->send_sem);
		info("zixi_stream_stop connecting -> notify end of capture");
		pthread_join(stream->send_thread, NULL);
		info("zixi_stream_stop connecting -> send thread done");
	}
	info("zixi_stream_stop done");
}

static bool is_mf_qsv(obs_output_t* output) {
	obs_encoder_t* encoder = obs_output_get_video_encoder(output);
	const char * encoder_name = obs_encoder_get_id(encoder);
	return strcmp(encoder_name, "mf_h264_qsv") == 0;
}
static bool	is_nvenc_h265(obs_output_t* output) {
	obs_encoder_t* encoder = obs_output_get_video_encoder(output);
	const char * encoder_name = obs_encoder_get_id(encoder);
	return strcmp(encoder_name, "obs_nvenc_h265") == 0;
}
static bool	is_msdk_h265(obs_output_t* output) {
	obs_encoder_t* encoder = obs_output_get_video_encoder(output);
	const char * encoder_name = obs_encoder_get_id(encoder);
	return strcmp(encoder_name, "obs_msdk_h265") == 0;
}
static bool is_lib_h265(obs_output_t* output) {
	obs_encoder_t* encoder = obs_output_get_video_encoder(output);
	const char * encoder_name = obs_encoder_get_id(encoder);
	return strcmp(encoder_name, "obs_x265") == 0;
}
static bool is_lib_h264(obs_output_t* output) {
    obs_encoder_t* encoder = obs_output_get_video_encoder(output);
	const char * encoder_name = obs_encoder_get_id(encoder);
	return strcmp(encoder_name, "obs_x264") == 0;
}
static bool zixi_stream_start(void * data) {
	struct zixi_stream* stream = (struct zixi_stream*)data;
	int vbitrate = 0;
	int abitrate = 0;
	int max_vbitrate = 0;
	int num_tracks = 0;

	if (!obs_output_can_begin_data_capture(stream->output, 0))
		return false;
	obs_encoder_t* encoder = obs_output_get_video_encoder(stream->output);
	if (is_lib_h265(stream->output) || is_msdk_h265(stream->output) || is_nvenc_h265(stream-> output)) {
		obs_data_t* settings = obs_encoder_get_settings(encoder);
		obs_data_set_bool(settings, "repeat_headers", true);
		vbitrate = obs_data_get_int(settings, "bitrate") * 1000;
		max_vbitrate = 1.5 * vbitrate;
		stream->is_x265 = true;
		obs_data_release(settings);
	} else if (is_lib_h264(stream->output)) {
		obs_data_t* settings = obs_encoder_get_settings(encoder);
		obs_data_set_bool(settings, "repeat_headers", true);
		vbitrate = obs_data_get_int(settings, "bitrate") * 1000;
		max_vbitrate = 1.5 * vbitrate;
		obs_data_release(settings);
	} else if (is_mf_qsv(stream->output)) {
		obs_data_t* settings = obs_encoder_get_settings(encoder);
		vbitrate = obs_data_get_int(settings, "mf_h264_bitrate") * 1000;
		bool use_max_bitrate = obs_data_get_bool(settings, "mf_h264_use_max_bitrate");
		if (use_max_bitrate) {
			max_vbitrate = obs_data_get_int(settings, "mf_h264_max_bitrate") * 1000;
		} else {
			max_vbitrate = 1.5 * vbitrate;
		}
		obs_data_release(settings);
	} else {
		obs_data_t* settings = obs_encoder_get_settings(encoder);
		vbitrate = obs_data_get_int(settings, "bitrate") * 1000;
		max_vbitrate = obs_data_get_int(settings, "max_bitrate") * 1000;
		if (max_vbitrate < (1.5 * vbitrate)) {
			max_vbitrate = 1.5 * vbitrate;
		}
		obs_data_release(settings);
	}
	
	stream->max_video_bitrate = max_vbitrate;
	stream->encoder_control.encoder_feedback = false;
	
	audio_t* audio = obs_get_audio();
	uint32_t channels = audio_output_get_channels(audio);
	uint32_t sample_rate = audio_output_get_sample_rate(audio);

	stream->audio_encoder_channels = channels;
	stream->audio_encoder_sample_rate = sample_rate;

	for (;;) {
		obs_encoder_t *aencoder = obs_output_get_audio_encoder(
			stream->output, num_tracks);
		if (!aencoder)
			break;

		obs_data_t* settings = obs_encoder_get_settings(aencoder);
		abitrate += ((int)obs_data_get_int(settings, "bitrate")) * 1000;
		obs_data_release(settings);
		num_tracks++;
	}

	stream->audio_bitrate = abitrate;
	stream->video_bitrate = vbitrate;
    if (!obs_output_initialize_encoders(stream->output, 0))
		return false;

	info("starting connect thread");
	os_atomic_set_bool(&stream->connecting, true);
	return pthread_create(&stream->connect_thread, NULL,
		connect_thread, stream) == 0;
}

static bool add_packet(struct zixi_stream * stream, struct encoder_packet *packet) {
	circlebuf_push_back(&stream->packets, packet,
		sizeof(struct encoder_packet));
	stream->last_dts_usec = packet->dts_usec;
	return true;
}

static void check_drop_frames(struct zixi_stream * stream) {
	struct encoder_packet first;
	int64_t buffer_duration_usec;

	if (num_buffered_packets(stream) < 5)
		return;

	circlebuf_peek_front(&stream->packets, &first, sizeof(first));

	/* do not drop frames if frames were just dropped within this time */
	if (first.dts_usec < stream->min_drop_dts_usec)
		return;

	/* if the amount of time stored in the buffered packets waiting to be
	* sent is higher than threshold, drop frames */
	buffer_duration_usec = stream->last_dts_usec - first.dts_usec;

	if (buffer_duration_usec > stream->drop_threshold_usec) {
		drop_frames(stream);
		debug("dropping %" PRId64 " worth of frames",
			buffer_duration_usec);
	}
}
static bool add_video_packet(struct zixi_stream * stream, struct encoder_packet *video_packet) {
	check_drop_frames(stream);

	if (video_packet->priority < stream->min_priority) {
		stream->dropped_frames++;
		return false;
	}
	else
		stream->min_priority = 0;
	return add_packet(stream, video_packet);
}


static int freq_to_adts(unsigned int freq) {
	int r = 0xf;
	switch (freq){
	case 96000:
		r = 0;
		break;
	case 88200:
		r = 1;
		break;
	case 64000:
		r = 2;
		break;
	case 48000:
		r = 3;
		break;
	case 44100:
		r = 4;
		break;
	case 32000:
		r = 5;
		break;
	}
	return r;
}
static void zixi_add_adts_headers(struct zixi_stream * stream,
	struct encoder_packet * out,
	struct encoder_packet * in) {
	char adts[7];
	size_t new_size = in->size + 7;
	adts[0] = 0xff;
	adts[1] = 0xf0;
	adts[1] |= 0x08;
	adts[1] |= 0x01;

	adts[2] = 0;
	adts[2] = 0x01 << 6;
	adts[2] |= 0x04 << 2;

	adts[3] = 0;
	adts[3] = stream->audio_encoder_channels << 6;
	/*adts[3] = freq_to_adts(stream->audio_encoder_sample_rate) << 2;*/
	adts[3] |= (new_size & 0x1FFF) >> 11;
	adts[4] = (new_size & 0x07FF) >> 3;
	adts[5] = ((new_size & 0x7) << 5) | 0x1F;
	adts[6] = 0xFC;
	out->drop_priority = in->drop_priority;
	out->dts = in->dts;
	out->pts = in->pts;
	out->dts_usec = in->dts_usec;
	out->encoder = in->encoder;
	out->keyframe = in->keyframe;
	out->priority = in->priority;
	out->timebase_den = in->timebase_den;
	out->timebase_num = in->timebase_num;
	out->track_idx = in->track_idx;
	out->type = in->type;

	out->size = new_size;
	out->data = bzalloc(new_size + sizeof(long));
	memcpy(out->data + sizeof(long), adts, 7);
	memcpy(out->data + 7 + sizeof(long) , in->data, in->size);
	((long*)out->data)[0] = 1;
	out->data = (long*)out->data + 1;
}

static void zixi_stream_data(void *data, struct encoder_packet *packet) {
	struct zixi_stream*  stream = (struct zixi_stream*)data;
	struct encoder_packet new_packet = { 0 };
	bool	added_packet = false;

	if (disconnected(stream))
		return;

	if (packet->type == OBS_ENCODER_VIDEO)
		obs_encoder_packet_ref(&new_packet, packet);
	else
		zixi_add_adts_headers(stream, &new_packet, packet);

	stream->packet_alloc++;
	pthread_mutex_lock(&stream->packets_mutex);
	if (!disconnected(stream) && !stopping(stream) )
		added_packet = (packet->type == OBS_ENCODER_VIDEO) ?
			add_video_packet(stream, &new_packet) :
			add_packet(stream, &new_packet);
	
	pthread_mutex_unlock(&stream->packets_mutex);

	if (added_packet) {
		os_sem_post(stream->send_sem);
	} else {
		obs_encoder_packet_release(&new_packet);
		stream->packet_free++;
	}
	obs_encoder_packet_release(packet);

}
static void zixi_stream_defaults(obs_data_t *defaults)
{

}
static obs_properties_t *zixi_stream_properties(void *unused)
{
	UNUSED_PARAMETER(unused);

	obs_properties_t *props = obs_properties_create();

	//obs_properties_add_int(props, OPT_DROP_THRESHOLD,		obs_module_text("RTMPStream.DropThreshold"),
	//200, 10000, 100);

	return props;
}

static uint64_t zixi_stream_total_bytes_sent(void *data)
{
	struct zixi_stream *stream = data;
	return stream->total_bytes_sent;
}

static int zixi_stream_dropped_frames(void *data)
{
	struct zixi_stream *stream = data;
	return stream->dropped_frames;
}

static float zixi_get_congestion(void *data){
	struct zixi_stream *stream = data;
	float r = 0.0;
	// info("zixi_get_congestion -> %d vs %d", stream->last_dropped_packets, stream->now_dropped_packets);
	if (stream->last_dropped_packets < stream->now_dropped_packets) {
		stream->congested_start_ts = os_gettime_ns();
		r = 1.0f;
	} else if ((os_gettime_ns() - stream->congested_start_ts) < TIME_TO_CLEAR_CONGESTION_NS) {
		r = 1.0f;
	}
	if (stream->encoder_feedback_enabled && r != 1.0f) {
		float ratio = (float)stream->encoder_control.last_sent_encoder_feedback /
			stream->video_bitrate;
		r = 1.0f - ratio;
	}
	
	return r;
}
struct obs_output_info zixi_output = {
	.id = "zixi_output",
	.flags = OBS_OUTPUT_AV |
	OBS_OUTPUT_ENCODED |
	OBS_OUTPUT_SERVICE |
	OBS_OUTPUT_MULTI_TRACK,
	.encoded_video_codecs = "h264",
	.encoded_audio_codecs = "aac",
	.get_name = zixi_stream_getname,
	.create = zixi_stream_create,
	.destroy = zixi_stream_destroy,
	.start = zixi_stream_start,
	.stop = zixi_stream_stop,
	.get_congestion = zixi_get_congestion,
	.encoded_packet = zixi_stream_data,
	.get_defaults = zixi_stream_defaults,
	.get_properties = zixi_stream_properties,
	.get_total_bytes = zixi_stream_total_bytes_sent,
	.get_dropped_frames = zixi_stream_dropped_frames
};

#ifdef WIN32
#include <Windows.h> // GetComputerName
#else
#include <unistd.h> // gethostname
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

static void fill_machine_id() {
	static bool init = false;
	if (!init) {
		init = true;
		size_t namelen = MAXHOSTNAMELEN;
#ifndef WIN32
		char machine_name[MAXHOSTNAMELEN + 1];

		if (gethostname(machine_name, namelen) == -1)
			memcpy(machine_name, "UNKNOWN", 7);
#else
		char machine_name[MAXHOSTNAMELEN + 1];

		if (GetComputerNameA(machine_name, (LPDWORD)&namelen) == 0)
			memcpy(machine_name, "UNKNOWN", 7);
#endif

		snprintf(MY_MACHINE_ID, 255, "obs_%s", machine_name);
	}
}
