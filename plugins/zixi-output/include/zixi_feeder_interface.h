#pragma once

#include "./zixi_definitions.h"
#include <stdint.h>

#if defined(IOS) || defined(MAC)
#include <stdio.h>
#endif

#ifndef DLL_EXPORT

#ifdef WIN32
#define ZIXI_FILE_HANDLE void*
#define INVALID_FILE_HANDLE (void*)-1
#define DLL_EXPORT __declspec(dllexport)
#else

#if defined (ANDROID)
#define ZIXI_FILE_HANDLE int
#define INVALID_FILE_HANDLE -1
#else

#include <stdio.h>
#define ZIXI_FILE_HANDLE FILE*
#define INVALID_FILE_HANDLE NULL
#endif // ANDROID
  #if __GNUC__ >= 4
    #define DLL_EXPORT __attribute__ ((visibility ("default")))
  #else
    #define DLL_EXPORT
  #endif
#endif

#endif
/**
@mainpage  ZIXI Feeder Interface
*/

// network interface configuration structure for bonded mode
typedef struct 
{
	char*			nic_ip;			// ip of local nic to use
	char*			device;			// device name to use (eg. "eth0")

	bool			backup;			// backup nics will only be used in case primary links have insufficient bandwidth
	unsigned int	bitrate_limit;	// set bitrate limite for this interface, 0 - unlimited [bits/sec]

	unsigned short local_port;		// local UDP port, 0 for automatic port selection
} zixi_nic_config;

typedef struct
{
	ZIXI_VIDEO_CODECS		video_codec;			// video codec to mux
	ZIXI_AUDIO_CODECS		audio_codec;			// audio codec to mux
	char					audio_channels;			// number of audio channels (1 = mono, 2=stereo)
}zixi_elementary_stream_config;




// configuration structure
typedef struct 
{
	char*					user_id;				//	User/Device ID for authorization (null terminated)
	char*					password;				//	Stream password for authorization (null terminated)

	char*					sz_stream_id;			//	Stream name on the broadcaster
	int						stream_id_max_length;	//	sz_stream_id buffer size
	
	char**					sz_hosts;				//	Destination hosts. Array of zero terminated strings. Broadcasters are connected in round-robin.
	int*					hosts_len;				//	Array of buffer lengths of sz_hosts
	int						num_hosts;				//	Number of entries in sz_hosts and hosts_len
	unsigned short*			port;					//	Array of destination broadcaster/decoder ports, size of num_hosts. Default=2088
	int						max_latency_ms;			//	Maximal latency for error correction, in milliseconds
	ZIXI_LATENCY_MODE		deprecated_latency_mode;//	Deprecated. For 1.7 compatibility
	int						max_bitrate;			//	Maximal stream bit rate, in bits/second
	char					rtp;					//	Source is rtp stream (rtp header will be stripped, no effect if not rtp stream)
	ZIXI_ADAPTIVE_MODE		limited;				//	Throttle bitrate - adapt encoder or FEC bitrate (use ZIXI_ENCODER_SETBITRATE_FUNCTION for callbacks)
	char					stop_on_drop;			//	Stop transmission on unrecoverable error until the next GOP. Only useful when allowed latency is shorter than a GOP, and encoder feedback is off. (default=0)
	char					reconnect;				//	Allows reconnection in case of a disconnection during streaming, this might cause the send function to block for several seconds
	int						fec_overhead;			//	FEC overhead in % on top of the original stream bitrate
	int						fec_block_ms;			//	Maximum time of FEC block. default=200ms
	char					content_aware_fec;		//	Uneven FEC allocation based on content. default=0
	ZIXI_ENCRYPTION			enc_type;				//	Encryption type. Constants defined in zixi_defilitions.h
	char*					sz_enc_key;				//	Encryption key - zero terminated string of hex digits. For eg. "1234567890abcdef1234567890abcdef"
	char					use_compression;		//	Reduce redundancies in the original stream. default=1
	char					fast_connect;			//	Allows clients to connect faster, increases latency. default=0
	ZIXI_IO_FUNCTION		read_function;			//	Optional external read function
	void*					read_param;				//	parameter for read function
	ZIXI_IO_FUNCTION		write_function;			//	Optional external write function
	void*					write_param;			//	parameter for write function
	ZIXI_RELEASE_FUNCTION	release_function;		//	Optional external release function
	void*					release_param;			//	parameter for write function
	int						smoothing_latency;		//	Smoothing latency by PCR, for jittery input. default=0
	unsigned int			timeout;				//	Reconnection timeout, in milliseconds, minimum is 5000, 0 for default
	
	zixi_nic_config*		local_nics;				//	Array of local network interfaces to use
	int						num_local_nics;			//	Number of entries in local_ips.  Use bonding mode when num_local_ips>1.
	char					force_bonding;			//	use bonding mode on a single NIC. default=0
													//  Note: Bonding requires FEC, fec_overhead must be > 0, recommeded value is 10

	int						max_delay_packets;		//	delay packets, allows smoother output bitrate. maximum number of packets to delay. default=0
	char					enforce_bitrate;		//	max_bitrate is enforcing - don't transmit faster than the max_bitrate parameter, or send operations will fail. max_delay_packets should be none zero. default=0
	bool					force_padding;			//  force piecewise CBR on the network - when encoder feedback is on, and 'use_compression' is on
	bool					elementary_streams;		//  data will be sent as elementary stream, use zixi_send_elementary_frame
	int						elementary_streams_max_va_diff_ms; // maximal difference between audio pts and video dts. default is 250ms
	zixi_elementary_stream_config elementary_streams_config;	// video and audio codecs configuration, used when elementary_streams=1

	bool					expect_high_jitter;		//	When true - jitter has less impact on congestion estimation. Relevant when running over highly jittery networks with diverse arrival times. default=false.

	bool					ignore_dtls_cert_error;	//	Ignore certificate error on DTLS connection. Use with caution, only when server is trusted.
	bool					replaceable;			//	Indicates that this stream can be replaced by other streams with the same id on supporting broadcasters (1.11+)

	ZIXI_PROTOCOL			protocol;				//	protocol type: udp/mmt/rist/http/https. When using rist/http/https - only the first host and port parameters are used, and the rest of the paremteters are ignored.
} zixi_stream_config;

typedef void (*ZIXI_ENCODER_SETBITRATE_FUNCTION)(int total_bps, bool force_iframe, void* param);
typedef void (*ZIXI_ASYNC_OPEN_CALLBACK)(ZIXI_STATUS status, int error, void *out_stream_handle, void* param);

// configuration structure for encoder feedback
typedef struct
{
	unsigned int max_bitrate;					// in bps, maximal threshold
	unsigned int min_bitrate;					// in bps, minimal threshold
	unsigned int update_interval;				// in milliseconds, 0 - default, callback will not be called unless update_interval milliseconds have passed since last call
	unsigned int aggressiveness;				// between 0 - 100, recommended 20
	ZIXI_ENCODER_SETBITRATE_FUNCTION setter;	// user callback function
	void *param;								// pointer to user data (will be passed as userdata when setter is invoked)
} encoder_control_info;

// configuration structure for automatic remote RTMP output
typedef struct _zixi_rtmp_out_config
{
	// optional parameters - set to NULL to disable
	char*			stream_name;						
	char*			url;					// rtmp://host:port/app
	char*			user;
	char*			password;
	char*			backup_url;				// alternative RTMP url target
	unsigned char	hot_backup;				// when 1, both 'url' and 'backup_url' will be connected. otherwise- either 'url' or 'backup_url' will connect

	// video and audio metadata fields
	// -------------------------------

	unsigned int	bitrate;				// bits/second
	unsigned int	max_va_diff;			// maximal number of frames saved for video/audio interleaving. default - set to 1000
	unsigned char	reconnection_period;	// seconds

	unsigned short	video_width;			// pixels
	unsigned short	video_height;			// pixels
	unsigned int	video_bitrate;			// bits/second
	unsigned int	audio_bitrate;			// bits/second
	unsigned int	audio_samplerate;		// samples/second, 0 for video only
	unsigned char	audio_channels;			// 1-mono, 2-stereo
	unsigned char	audio_sample_size;		// bits per sample
	unsigned char	audio_aot;				// AAC audio object type, according to ISO/IEC 13818-7. 0=Main profile, 1=AAC-LC, 2=SSR
	unsigned char	video_frame_rate;		// frames/second
	unsigned char	video_avc_profile;		// 66-baseline, 77-main, 100-high
	unsigned char	video_avc_level;		// video level times 10, for example, 51 means level=5.1
	unsigned short	video_gop_size;			// frames/gop
	ZIXI_VIDEO_CODECS	video_codec;		//h264 and hevc supported
	ZIXI_AUDIO_CODECS	audio_codec;		//aac and opus supported

/*
	_zixi_rtmp_out_config() : stream_name(0), url(0), user(0), password(0), backup_url(0), hot_backup(false), bitrate(0), max_va_diff(1000), reconnection_period(3), 
		video_width(0), video_height(0), video_bitrate(0), audio_bitrate(0), audio_samplerate(0), audio_channels(0), audio_sample_size(0), audio_aot(0), video_frame_rate(0),
		video_avc_profile(0), video_avc_level(0), video_gop_size(0) {}
*/

} zixi_rtmp_out_config;

// configuration structure for file upload
typedef struct
{
	char*					user_id;				//	User/Device ID for authorization (null terminated)
	char*					password;				//	Password for authorization (null terminated)

	char*					file_path;				//	Local file path. when uploading data from memory, use file_path as target file name
	char*					host;					//	Remote broadcaster address
	unsigned short			port;					//	Remote broadcaster port (default 2088)
	int						mtu;					//  Set network MTU (default: 1462, min: 576)
	char*					target_path;			//	Target upload path on remote broadcaster
	uint64_t				initial_bitrate;		// 	initial upload bitrate, in bits/second. if 0 initial bitrate = max bitrate
	uint64_t				max_bitrate;			// 	Maximum upload bitrate, in bits/second
	char					growing_file;			//  Indicate that the file is still open and growing.  Call 'zixi_finalize_upload' when file is done growing.
	ZIXI_ENCRYPTION			enc_type;				//	Encryption type. Constants defined in zixi_defilitions.h
	char*					sz_enc_key;				//	Encryption key - zero terminated string of hex digits. For eg. "1234567890abcdef1234567890abcdef"
	bool					mmt;					//	Use MMT protocol
	bool					overwrite;				//	Overwrite existing file on server side if it exists
	bool					delete_partial;			//  Set to true to delete partial uploaded file on disconnect
	ZIXI_FILE_HANDLE		file_handle;			//  if not set to INVALID_FILE_HANDLE - will use the file handle and not the path

	unsigned char*			data_ptr;				// upload data located in memory. growing_file must be 0
	long long				data_size;				// data size in bytes
	bool					ignore_dtls_cert_error;	//	Ignore certificate error on DTLS connection. Use with caution, only when server is trusted.
} zixi_upload_config;

typedef struct
{
	ZIXI_ASYNC_OPEN_CALLBACK callback;
	void *param;
} async_open_info;


#ifndef ZIXI_NO_FUNCTION_HEADERS

#ifdef __cplusplus
extern "C"
{
#endif

//--------------------------------------------------------
//			Configuration functions
//--------------------------------------------------------


/**
@brief	prepare configuration structs with default values

		must be called prior to zixi_open_stream

@param	stream[optional]
@param	rtmp[optional]
@param	upload[optional]

@ return	ZIXI_ERROR_OK
*/
DLL_EXPORT int zixi_prepare_configuration(zixi_stream_config* stream, zixi_rtmp_out_config* rtmp, zixi_upload_config* upload);

/**
@brief	configure logging.

		can be called during operation to change the log level or log function

@param		log_level	- log detail level ,
						  [-1] to turn off ,
						  [0] to log everything (significantly hurt performance - only for deep debugging) ,
						  [1-5] different log levels (3 recommended)
@param		log_func	- logging callback function
@param		user_data	- user data that will be returned as the first parameter in log_func

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
*/
DLL_EXPORT int zixi_configure_logging(ZIXI_LOG_LEVELS log_level, ZIXI_LOG_FUNC log_func, void *user_data);

/**
@brief		Deprecated - use user_id and password parameters in zixi_stream_config

@param		user		- user unique ID
@param		user_len	- 'user' buffer length
@param		session		- session token or password
@param		session_len	- 'session' buffer length

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
*/
DLL_EXPORT int zixi_configure_credentials(const char *user, int user_len, const char *session, int session_len);

/**
@brief		creates a new stream

@param		parameters				- stream parameters
@param		enc_ctrl				- encoder controlling functions to get & adjust source bitrate. Should be null for autonomous encoder.
@param		out_stream_handle[out]	- stream handle when connection succeeds. NULL on failure

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_TIMEOUT
@return		ZIXI_ERROR_RESOLVING_FAILED
@return		ZIXI_ERROR_NETWORK
@return		ZIXI_ERROR_AUTHORIZATION_FAILED
@return		ZIXI_ERROR_SERVER_FULL
@return		ZIXI_ERROR_LICENSING_FAILED
@return		ZIXI_ERROR_VERSION
@return		ZIXI_ERROR_CERTIFICATE
@return		ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_open_stream(zixi_stream_config parameters, encoder_control_info* enc_ctrl, void **out_stream_handle);

/**
@brief		creates a new stream

@param		parameters				- stream parameters
@param		async_info				- callback handler for connection notification
@param		enc_ctrl				- encoder controlling functions to get & adjust source bitrate. Should be null for autonomous encoder.
@param		out_stream_handle[out]	- stream handle when connection succeeds. NULL on failiure

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_open_stream_async(zixi_stream_config parameters, async_open_info async_info, encoder_control_info* enc_ctrl, void **out_stream_handle);

/**
@brief		creates a new multiplexed connection, use zixi_add_multiplexed_stream to configure the individual streams

@param		parameters				- stream parameters
@param		enc_ctrl				- encoder controlling functions to get & adjust source bitrate. Should be null for autonomous encoder.
@param		streams_count			- number of expected streams (set to 0 if unknown)
@param		out_stream_handle[out]	- stream handle when connection succeeds. NULL on failure

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_TIMEOUT
@return		ZIXI_ERROR_RESOLVING_FAILED
@return		ZIXI_ERROR_NETWORK
@return		ZIXI_ERROR_AUTHORIZATION_FAILED
@return		ZIXI_ERROR_SERVER_FULL
@return		ZIXI_ERROR_LICENSING_FAILED
@return		ZIXI_ERROR_VERSION
@return		ZIXI_ERROR_CERTIFICATE
@return		ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_open_multiplexed_stream(zixi_stream_config parameters, encoder_control_info* enc_ctrl, unsigned int streams_count, void **out_stream_handle);

/**
@brief		add a new multiplexed stream with rtmp output to an existing multiplexed connection

@param		stream_handle			- handle for a multiplexed stream created by zixi_open_multiplexed_stream
@param		stream_id				- sub stream id, the final id on the server will be <main stream id>_<sub stream id>
@param		rtmp_out				- rtmp output parameters
@param		multiplex_index[out]	- use this index when sending packets via zixi_send_multiplexed_packet
@param		elementary_streams		- user will provide elementary streams (call to zixi_send_multiplexed_frame)
@param		stream_config			- provide video/audio codecs

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@retrun		ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_add_multiplexed_stream(void* stream_handle, char* stream_id, zixi_rtmp_out_config* rtmp_out, int* multiplex_index, bool elementary_streams, zixi_elementary_stream_config stream_config);

/**
@brief		creates a new stream with rtmp output on the broadcaster side

@param		parameters			- stream parameters
@param		enc_ctrl				- encoder controlling functions to get & adjust source bitrate. Should be null for autonomous encoder.
@param		rtmp_out				- rtmp output parameters
@param		out_stream_handle[out]	- stream handle when connection succeeds. NULL on failiure

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_TIMEOUT
@return		ZIXI_ERROR_RESOLVING_FAILED
@return		ZIXI_ERROR_NETWORK
@return		ZIXI_ERROR_AUTHORIZATION_FAILED
@return		ZIXI_ERROR_SERVER_FULL
@return		ZIXI_ERROR_LICENSING_FAILED
@return		ZIXI_ERROR_VERSION
@return		ZIXI_ERROR_CERTIFICATE
*/
DLL_EXPORT int zixi_open_stream_with_rtmp(zixi_stream_config parameters, encoder_control_info* enc_ctrl, zixi_rtmp_out_config* rtmp_out, void **out_stream_handle);

/**
@brief		creates a new upload

@param		parameters			- upload parameters
@param		out_stream_handle[out]	- stream handle when connection succeeds. NULL on failure

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_TIMEOUT
@return		ZIXI_ERROR_RESOLVING_FAILED
@return		ZIXI_ERROR_NETWORK
@return		ZIXI_ERROR_NOT_FOUND
@return		ZIXI_ERROR_FILE_LOCAL
@return		ZIXI_ERROR_FILE_REMOTE
@return		ZIXI_WARNING_REMOTE_FILE_EXISTS
@return		ZIXI_ERROR_AUTHORIZATION_FAILED
@return		ZIXI_ERROR_NOT_SUPPORTED
@return		ZIXI_ERROR_FAILED
@return		ZIXI_ERROR_SERVER_FULL
@return		ZIXI_ERROR_CERTIFICATE
@return		ZIXI_ERROR_LICENSING_FAILED
*/
DLL_EXPORT int zixi_upload_file(zixi_upload_config parameters, void **out_stream_handle);

/**
@brief		indicates that a growing file no longer grows

@param		stream_handle	- stream handle returned from zixi_add_stream

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
*/
DLL_EXPORT int zixi_finalize_upload(void* stream_handle);

/**
@brief		sets the target bitrate for a file upload - must not be lower than max_bitrate configured in initialization

@param		stream_handle - stream handle returned from zixi_upload_file

@return		ZIXI_ERROR_OK
			ZIXI_ERROR_INVALID_PARAMETER - invalid file upload handle
			ZIXI_ERROR_FAILED			 - failed to set max bitrate
*/
DLL_EXPORT int zixi_set_upload_file_target_bitrate(void * stream_handle, uint64_t target_bitrate);

/**
@brief		closes an open stream

@param		stream_handle	- stream handle returned from zixi_add_stream

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
*/
DLL_EXPORT int zixi_close_stream(void *stream_handle);

/**
@brief		update network interfaces to use for bonded streams, can be used to change IP addresses during a session

@param		stream_handle	- stream handle returned from zixi_add_stream
@param		local_ips		- vector of nic ips
@param		count		- number of nics

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_update_local_ips(void* stream_handle, zixi_nic_config* local_nics, int count);

/**
@brief		update network interfaces to use for bonded streams, this method will use all avilable IPs on the machine

@param		stream_handle	- stream handle returned from zixi_add_stream
@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_set_automatic_ips(void* stream_handle);

/**
@brief		sends one packet to destination

@param		stream_handle	- stream handle returned from zixi_add_stream
@param	    ts_frame_buffer	- transport stream data
@param	    buffer_length	- size of ts_frame_buffer, 188 to 1316 bytes

@return     ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return     ZIXI_ERROR_NOT_READY
@return		ZIXI_ERROR_NOT_CONNECTED
@return     ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_send_frame(void *stream_handle, char *ts_frame_buffer, int buffer_length, unsigned int timestamp);

/**
@brief		sends one packet to destination on a multiplexed stream

@param		stream_handle	- stream handle returned from zixi_open_multiplexed_stream
@param		index		- multiplex index returned from zixi_add_multiplexed_stream
@param		buffer		- transport stream data
@param		buffer_length	- size of ts_frame_buffer, 188 to 1316 bytes

@return     ZIXI_ERROR_OK
@retrun		ZIXI_ERROR_INVALID_PARAMETER
@return     ZIXI_ERROR_NOT_READY
@return		ZIXI_ERROR_NOT_CONNECTED
@return     ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_send_multiplexed_packet(void *stream_handle, int index, char *buffer, int buffer_length, unsigned int timestamp);

/**
@brief		sends one frame to destination on a multiplexed stream

@param		stream_handle	- stream handle returned from zixi_open_multiplexed_stream
@param		index			- multiplex index returned from zixi_add_multiplexed_stream
@param		frame			- elementary stream frame data
@param		frame_length	- length of frame in bytes
@param	    pts				- frame presentation timestamp, in 90khz units
@param	    dts				- frame decoding timestamp, in 90khz units. must be monotonically increasing
@param	    video			- true for video frame, false for audio


@return     ZIXI_ERROR_OK
@retrun		ZIXI_ERROR_INVALID_PARAMETER
@return     ZIXI_ERROR_NOT_READY
@return		ZIXI_ERROR_NOT_CONNECTED
@return     ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_send_multiplexed_frame(void *stream_handle, int index, char* frame, int frame_length, uint64_t pts, uint64_t dts, bool video);

/**
@brief		send message to rtmp clients on this stream

@param		stream_handle	- stream handle returned from zixi_add_stream
@param		index			- multiplex index returned from zixi_add_multiplexed_stream, or -1 if multiplexing isn't used
@param		stream_id		- transport stream data
@param		message_type	- rtmp message type
@param		data			- message data to send
@param		data_size		- data size must me less than 1400 bytes

@return     ZIXI_ERROR_OK
@return     ZIXI_ERROR_INVALID_PARAMETER
@return     ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_send_rtmp_message(void *stream_handle, int index, int stream_id, unsigned char message_type, unsigned char* data, int data_size);

/**
@brief		returns stream statistics

@param		stream_handle	- stream handle returned from zixi_add_stream
@param		conn_stats[out]	- pointer to connection statistics structure
@param		net_stats[out]	- pointer to network statistics structure
@param		error_correction_stats[out]	- pointer to error correction statistics structure

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_FAILED
@return		Any zixi error code applicable for initialized stream, as last error on the stream.
*/
DLL_EXPORT int zixi_get_stats(void *stream_handle, ZIXI_CONNECTION_STATS* conn_stats, ZIXI_NETWORK_STATS *net_stats, ZIXI_ERROR_CORRECTION_STATS *error_correction_stats);

/**
@brief		returns stream statistics on a link in a bonded connection

@param		stream_handle	- stream handle returned from zixi_add_stream
@param		link_index	- link index for bonded connections
@param		conn_stats[out]	- pointer to connection statistics structure
@param		net_stats[out]	- pointer to network statistics structure
@param		error_correction_stats[out]	- pointer to error correction statistics structure

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_FAILED
*/
DLL_EXPORT int zixi_get_bonded_stats(void *stream_handle, int link_index, ZIXI_CONNECTION_STATS* conn_stats, ZIXI_NETWORK_STATS *net_stats, ZIXI_ERROR_CORRECTION_STATS *error_correction_stats);

/**
@brief		returns upload progress

@param		stream_handle			- stream handle returned from zixi_add_stream
@param		bytes_transfered[out]	- number of byte uploaded successfully
@param		file_size[out]			- total number of bytes in file. For growing file the value is undefined before calling 'zixi_finalize_upload'
@param		status[out]			- connection status

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return		ZIXI_ERROR_NOT_FOUND;
@return		Any zixi error code applicable for initialized stream, as last error on the stream.
*/
DLL_EXPORT int zixi_get_upload_progress(void *stream_handle, long long* bytes_transfered, long long* bytes_total, ZIXI_STATUS* status);

/**
@brief		returns the internal thread id

@param		stream_handle	- stream handle returned from zixi_add_stream
@param		thread_id[out]	- pointer to returned thread id

@return		ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
*/
DLL_EXPORT int zixi_get_thread(void* stream_handle, int* thread_id);

/**
@brief		returns the zixi feeder library version

@param		major	- major version number
@param		minor	- minor version number (incremented each time we update our internal protocol)
@param		build	- build number

@return  	ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
*/
DLL_EXPORT int zixi_version(int* major, int* minor, int* minor_minor, int* build);



#ifdef ZIXI_FEEDER_SUPPORT_RTMP
/**
@brief		opens rtmp connection

@param		config	- zixi_rtmp_out_config structure, with rtmp connection required and optional parameters
@param		handler	- rtmp handler used for rtmp functions
@param		fix_ts	- true if video elementary stream uses 0x00000001 to separate NALUs, it will be convereted to NALU size.

@return  	ZIXI_ERROR_OK
@return		ZIXI_ERROR_FAILED
@return		ZIXI_ERROR_NOT_CONNECTED
*/

DLL_EXPORT int zixi_open_rtmp_connection(const zixi_rtmp_out_config* config, void** /*out*/ handler );

/**
@brief		send frame (h264, aac) data on rtmp connection

@param		handler	- received from zixi_open_rtmp_connection call
@param		data	- pointer to h264/aac frame
@param		length	- h264/aac frame length
@param		ts		- timestamp in miliseconds
@param		composition_offset - (presentation timestamp - decode timestamp) difference for h264 protocol (must be non negative)
@param		video	- video == true or audio == false

@return		ZIXI_ERROR_FAILED
@return		ZIXI_ERROR_NOT_CONNECTED
@return  	ZIXI_ERROR_OK
*/
DLL_EXPORT int zixi_send_rtmp_frame(void* /*out*/ handler, unsigned char* data, unsigned int length, uint64_t ts, uint32_t composition_offset, bool video);

/**
@brief		closes rtmp connection

@param		handler	- received from zixi_open_rtmp_connection call

@return		ZIXI_ERROR_FAILED
@return  	ZIXI_ERROR_OK
*/
DLL_EXPORT int zixi_close_rtmp_connection(void* handler);

/**
@brief		reports rtmp connection active bitrate (3s average)

@param		handler	- received from zixi_open_rtmp_connection call

@return		ZIXI_ERROR_NOT_CONNECTED
@return		ZIXI_ERROR_FAILED
@return		rtmp connection bitrate
*/
DLL_EXPORT int zixi_get_rtmp_bitrate(void* handler);

/**
@brief		reports rtmp connection buffer level (i.e. unsent data)

@param		handler	- received from zixi_open_rtmp_connection call

@return		ZIXI_ERROR_NOT_CONNECTED
@return		ZIXI_ERROR_FAILED
@return		rtmp connection buffer level
*/
DLL_EXPORT int zixi_get_rtmp_buffer(void* handler);

/**
@brief		reports number of dropped frames on rtmp connection (if connection is too slow) drops occurs after raching maximum sending queue buffer of 10 *1024 rtmp chunks

@param		handler	- received from zixi_open_rtmp_connection call

@return		ZIXI_ERROR_NOT_CONNECTED
@return		ZIXI_ERROR_FAILED
@return		number of dropped frames
*/
DLL_EXPORT int zixi_get_rtmp_drops(void* handler);

/**
@brief		reports number of frames in queue of rtmp connection

@param		handler			- received from zixi_open_rtmp_connection call
@param		video_frames	- video frames in queue
@param		audio_frames	- audio frames in queue

@return		ZIXI_ERROR_OK if successed

*/
DLL_EXPORT int zixi_get_rtmp_frames(void* handler, unsigned int* video_frames, unsigned int* audio_frames);

/**
@brief		reports rtmp connection sent bytes

@param		handler	- received from zixi_open_rtmp_connection call

@return		ZIXI_ERROR_NOT_CONNECTED
@return		ZIXI_ERROR_FAILED
@return		sent bytes
*/
DLL_EXPORT long long zixi_get_rtmp_total_sent(void* handler);


/**
@brief		reports rtmp remote adress

@param		handler	- received from zixi_open_rtmp_connection call

@return		true on success, false otherwise.
*/
DLL_EXPORT bool zixi_get_rtmp_remote_ip(void* handler, char addr[16]);

#endif //ZIXI_FEEDER_SUPPORT_RTMP

/**
@brief		sends one frame to destination

@param		stream_handle	- stream handle returned from zixi_open_stream, zixi_open_stream_with_rtmp
@param	    frame_buffer	- elementary data
@param	    buffer_length	- size of frame_buffer
@param	    video			- true for video frame, false for audio
@param	    pts				- frame presentation timestamp, in 90khz units
@param	    dts				- frame decoding timestamp, in 90khz units. must be monotonically increasing 

@return     ZIXI_ERROR_OK
@return		ZIXI_ERROR_INVALID_PARAMETER
@return     ZIXI_ERROR_NOT_READY
@return     ZIXI_ERROR_FAILED
@return		ZIXI_ERROR_NOT_CONNECTED
@return		ZIXI_WARNING_OVER_LIMIT
*/
DLL_EXPORT int zixi_send_elementary_frame(void *stream_handle, char *frame_buffer, int buffer_length, bool video, uint64_t pts, uint64_t dts);

#ifdef __cplusplus
};	//	extern "C"
#endif

#endif
