#ifndef __ZIXI_CONSTANTS_H_
#define __ZIXI_CONSTANTS_H_

#define ZIXI_SERVICE_PROP_SERVICE "zixi_service"
#define ZIXI_SERVICE_PROP_URL "zixi_url"
#define ZIXI_SERVICE_PROP_LATENCY_ID "zixi_latency_id"
#define ZIXI_SERVICE_PROP_ENCRYPTION_TYPE "zixi_enc_type"
#define ZIXI_SERVICE_PROP_ENCRYPTION_KEY  "zixi_enc_key"
#define ZIXI_SERVICE_PROP_ENCRYPTION_USE  "zixi_enc_use"
#define ZIXI_SERVICE_PROP_PASSWORD			"zixi_stream_password"
#define ZIXI_SERVICE_PROP_ENABLE_BONDING	"zixi_enable_bonding"
#define ZIXI_SERVICE_PROP_USE_ENCODER_FEEDBACK "zixi_use_encoder_feedback"

#define ZIXI_DEFAULT_LATENCY_ID 6
#define ZIXI_SERVICE_PROP_USE_AUTO_RTMP_OUT "zixi_use_auto_rtmp_out"
#define ZIXI_SERVICE_PROP_AUTO_RTMP_URL		"zixi_auto_rtmp_url"
#define ZIXI_SERVICE_PROP_AUTO_RTMP_CHANNEL "zixi_auto_rtmp_channel"
#define ZIXI_SERVICE_PROP_AUTO_RTMP_USERNAME "zixi_auto_rtmp_username"
#define ZIXI_SERVICE_PROP_AUTO_RTMP_PASSWORD "zixi_auto_rtmp_password"
#ifdef ZIXI_ADVANCED
#define ZIXI_SERVICE_ADV_USE "zixi_advanced_settings"
#define ZIXI_SERVICE_FEC_OVERHEAD "zixi_advanced_fec_overhead"
#define ZIXI_SERVICE_FEC_MS "zixi_advanced_fec_ms"
#define ZIXI_SERVICE_PROP_ZIXI_LOG_LEVEL "zixi_log_level"
// #define ZIXI_LOW_LATENCY_MUXING "zixi_low_latency_mux"
#endif

#define TIME_BETWEEN_AUTO_BOND_SCAN_US 1000000000

struct zixi_settings{
	char *							url;
	char *							encryption_key;
	char *							password;
	unsigned int					latency_id;
	unsigned int					encryption_id;
	unsigned int					max_bitrate;
	bool							enable_bonding;
	bool							encoder_feedback;

	bool							auto_rtmp_out;
	char*							rtmp_url;
	char*							rtmp_channel;
	char*							rtmp_password;
	char*							rtmp_username;

} ;

#endif // __ZIXI_CONSTANTS_H_