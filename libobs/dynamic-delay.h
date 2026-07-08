// dynamic-delay.h
#ifndef DYNAMIC_DELAY_H
#define DYNAMIC_DELAY_H

#include "obs.h"
#include "util/threading.h"

#ifdef __cplusplus
extern "C" {
#endif

struct dynamic_delay_buffer {
	struct encoder_packet *packets; // circular buffer of packets
	size_t capacity;                // total number of slots
	size_t head;                    // next write position
	size_t tail;                    // next read position
	uint64_t buffered_time_ms;      // total buffered duration in ms
	uint64_t max_time_ms;           // configured maximum delay
	uint64_t memory_bytes;          // total memory consumed by packet data in bytes
	uint64_t max_memory_bytes;      // hard limit for memory consumption (e.g. 500 MB)
	pthread_mutex_t mutex;          // protect buffer access
};

// Initialize buffer with max_time_ms (e.g., 180000 ms for 3 min), approximate capacity based on bitrate, and hard memory limit.
struct dynamic_delay_buffer *dynamic_delay_create(uint64_t max_time_ms, uint64_t avg_bitrate_kbps,
						  uint64_t max_memory_bytes);

// Free resources.
void dynamic_delay_destroy(struct dynamic_delay_buffer *buf);

// Push a packet into the buffer. If full or exceeding max_time_ms/max_memory_bytes, oldest packets are automatically dropped/popped to make room.
bool dynamic_delay_push(struct dynamic_delay_buffer *buf, const struct encoder_packet *packet);

// Pop the oldest packet from the buffer. Returns true if a packet was retrieved.
bool dynamic_delay_pop(struct dynamic_delay_buffer *buf, struct encoder_packet *out_packet);

// Clear the buffer (drop all buffered packets).
void dynamic_delay_clear(struct dynamic_delay_buffer *buf);

// Query current buffered duration in ms.
uint64_t dynamic_delay_buffered_ms(const struct dynamic_delay_buffer *buf);

// Query current memory usage in bytes.
uint64_t dynamic_delay_get_memory_usage(const struct dynamic_delay_buffer *buf);

// Query current number of packets in buffer.
size_t dynamic_delay_get_packet_count(const struct dynamic_delay_buffer *buf);

#ifdef __cplusplus
}
#endif

#endif // DYNAMIC_DELAY_H
