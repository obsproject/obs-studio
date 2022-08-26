#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct CodecContext CodecContext;

typedef struct OBSWebRTCCall OBSWebRTCCall;

typedef struct OBSWebRTCStream OBSWebRTCStream;

typedef void (*PacketCallback)(struct CodecContext, const uint8_t *,
			       unsigned int, unsigned int);

struct OBSWebRTCCall *obs_webrtc_call_init(const void *context,
					   PacketCallback cb);

void obs_webrtc_call_start(struct OBSWebRTCCall *obsrtc);

struct OBSWebRTCStream *obs_webrtc_stream_init(const char *name);

void obs_webrtc_stream_connect(struct OBSWebRTCStream *obsrtc);

void obs_webrtc_stream_data(struct OBSWebRTCStream *obsrtc, const uint8_t *data,
			    uintptr_t size, uint64_t duration);

void obs_webrtc_stream_audio(struct OBSWebRTCStream *obsrtc,
			     const uint8_t *data, uintptr_t size,
			     uint64_t duration);
