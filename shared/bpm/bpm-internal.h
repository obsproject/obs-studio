#ifndef BPM_INTERNAL_H
#define BPM_INTERNAL_H
#ifdef __cplusplus
extern "C" {
#endif
#include "bpm.h"
#include "caption/mpeg.h"
#include "obs-av1.h"
#include "util/array-serializer.h"
#include "util/platform.h"
#include "util/threading.h"

struct counter_data {
	uint32_t diff;
	uint32_t ref;
	uint32_t curr;
};

#define RFC3339_MAX_LENGTH (64)
struct metrics_time {
	struct timespec tspec;
	char rfc3339_str[RFC3339_MAX_LENGTH];
	bool valid;
};

// Broadcast Performance Metrics SEI types
enum bpm_sei_types {
	BPM_TS_SEI = 0, // BPM Timestamp SEI
	BPM_SM_SEI,     // BPM Session Metrics SEI
	BPM_ERM_SEI,    // BPM Encoded Rendition Metrics SEI
	BPM_MAX_SEI
};

#define SEI_UUID_SIZE 16
static const uint8_t bpm_ts_uuid[SEI_UUID_SIZE] = {0x0a, 0xec, 0xff, 0xe7,
						   0x52, 0x72, 0x4e, 0x2f,
						   0xa6, 0x2f, 0xd1, 0x9c,
						   0xd6, 0x1a, 0x93, 0xb5};
static const uint8_t bpm_sm_uuid[SEI_UUID_SIZE] = {0xca, 0x60, 0xe7, 0x1c,
						   0x6a, 0x8b, 0x43, 0x88,
						   0xa3, 0x77, 0x15, 0x1d,
						   0xf7, 0xbf, 0x8a, 0xc2};
static const uint8_t bpm_erm_uuid[SEI_UUID_SIZE] = {0xf1, 0xfb, 0xc1, 0xd5,
						    0x10, 0x1e, 0x4f, 0xb5,
						    0xa6, 0x1e, 0xb8, 0xce,
						    0x3c, 0x07, 0xb8, 0xc0};

// Broadcast Performance Metrics timestamp types
enum bpm_ts_type {
	BPM_TS_RFC3339 = 1, // RFC3339 timestamp string
	BPM_TS_DURATION,    // Duration since epoch in milliseconds (64-bit)
	BPM_TS_DELTA        // Delta timestamp in nanoseconds (64-bit)
};

// Broadcast Performance Metrics timestamp event tags
enum bpm_ts_event_tag {
	BPM_TS_EVENT_CTS = 1, // Composition Time Event
	BPM_TS_EVENT_FER,     // Frame Encode Request Event
	BPM_TS_EVENT_FERC,    // Frame Encode Request Complete Event
	BPM_TS_EVENT_PIR      // Packet Interleave Request Event
};

// Broadcast Performance Session Metrics types
enum bpm_sm_type {
	BPM_SM_FRAMES_RENDERED = 1, // Frames rendered by compositor
	BPM_SM_FRAMES_LAGGED,       // Frames lagged by compositor
	BPM_SM_FRAMES_DROPPED,      // Frames dropped due to network congestion
	BPM_SM_FRAMES_OUTPUT // Total frames output (sum of all video encoder rendition sinks)
};

// Broadcast Performance Encoded Rendition Metrics types
enum bpm_erm_type {
	BPM_ERM_FRAMES_INPUT = 1, // Frames input to the encoder rendition
	BPM_ERM_FRAMES_SKIPPED,   // Frames skippped by the encoder rendition
	BPM_ERM_FRAMES_OUTPUT // Frames output (encoded) by the encoder rendition
};

struct metrics_data {
	pthread_mutex_t metrics_mutex;
	struct counter_data rendition_frames_input;
	struct counter_data rendition_frames_output;
	struct counter_data rendition_frames_skipped;
	struct counter_data session_frames_rendered;
	struct counter_data session_frames_output;
	struct counter_data session_frames_dropped;
	struct counter_data session_frames_lagged;
	struct array_output_data sei_payload[BPM_MAX_SEI];
	bool sei_rendered[BPM_MAX_SEI];
	struct metrics_time
		cts; // Composition timestamp (i.e. when the frame was created)
	struct metrics_time ferts;  // Frame encode request timestamp
	struct metrics_time fercts; // Frame encode request complete timestamp
	struct metrics_time pirts;  // Packet Interleave Request timestamp
};

struct output_metrics_link {
	obs_output_t *output;
	struct metrics_data *metrics_tracks[MAX_OUTPUT_VIDEO_ENCODERS];
};

static pthread_mutex_t bpm_metrics_mutex;
/* This DARRAY is used for creating an association between the output_t
 * and the BPM metrics track data for each output that requires BPM injection.
 */
static DARRAY(struct output_metrics_link) bpm_metrics;

#ifdef __cplusplus
}
#endif
#endif
