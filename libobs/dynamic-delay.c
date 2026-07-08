// dynamic-delay.c
#include "dynamic-delay.h"
#include "obs.h"
#include "obs-internal.h"
#include <stdlib.h>
#include <string.h>
#include "util/threading.h"

static size_t estimate_capacity(uint64_t max_time_ms, uint64_t avg_bitrate_kbps)
{
	// Approximate number of packets needed for max_time_ms.
	// bitrate kbps -> bytes per second = bitrate_kbps * 125.
	uint64_t bytes_per_sec = avg_bitrate_kbps * 125ULL;
	// Assume average packet size of 1200 bytes (typical for h264).
	uint64_t packets_per_sec = bytes_per_sec / 1200ULL;
	uint64_t total_packets = (max_time_ms * packets_per_sec) / 1000ULL;
	// Add safety margin (e.g. 3x for audio/video interleaved packets + safety)
	return (size_t)(total_packets * 3ULL + 100ULL);
}

static uint64_t get_packet_duration_ms(const struct encoder_packet *packet)
{
	if (!packet || packet->type != OBS_ENCODER_VIDEO || packet->timebase_den == 0)
		return 0;
	return (uint64_t)((packet->timebase_num * 1000LL) / packet->timebase_den);
}

struct dynamic_delay_buffer *dynamic_delay_create(uint64_t max_time_ms, uint64_t avg_bitrate_kbps,
						  uint64_t max_memory_bytes)
{
	struct dynamic_delay_buffer *buf = calloc(1, sizeof(struct dynamic_delay_buffer));
	if (!buf)
		return NULL;
	buf->max_time_ms = max_time_ms;
	buf->max_memory_bytes = (max_memory_bytes > 0) ? max_memory_bytes
						       : (500ULL * 1024ULL * 1024ULL); // default 500 MB
	buf->capacity = estimate_capacity(max_time_ms, (avg_bitrate_kbps > 0) ? avg_bitrate_kbps : 6000ULL);
	buf->packets = calloc(buf->capacity, sizeof(struct encoder_packet));
	if (!buf->packets) {
		free(buf);
		return NULL;
	}
	pthread_mutex_init(&buf->mutex, NULL);
	return buf;
}

void dynamic_delay_destroy(struct dynamic_delay_buffer *buf)
{
	if (!buf)
		return;
	pthread_mutex_lock(&buf->mutex);
	while (buf->head != buf->tail) {
		struct encoder_packet *slot = &buf->packets[buf->tail];
		if (slot->data && slot->size > 0) {
			obs_encoder_packet_release(slot);
		}
		memset(slot, 0, sizeof(struct encoder_packet));
		buf->tail = (buf->tail + 1) % buf->capacity;
	}
	pthread_mutex_unlock(&buf->mutex);
	pthread_mutex_destroy(&buf->mutex);
	free(buf->packets);
	free(buf);
}

bool dynamic_delay_push(struct dynamic_delay_buffer *buf, const struct encoder_packet *packet)
{
	if (!buf || !packet)
		return false;

	pthread_mutex_lock(&buf->mutex);

	uint64_t pkt_duration_ms = get_packet_duration_ms(packet);
	size_t pkt_size = packet->size;

	// If adding this packet exceeds max_time_ms, max_memory_bytes, or capacity, drop oldest packet(s) from tail
	while (buf->head != buf->tail &&
	       ((buf->head + 1) % buf->capacity == buf->tail ||
		(buf->buffered_time_ms + pkt_duration_ms > buf->max_time_ms && buf->buffered_time_ms > 0) ||
		(buf->memory_bytes + pkt_size > buf->max_memory_bytes && buf->memory_bytes > 0))) {

		struct encoder_packet *old_slot = &buf->packets[buf->tail];
		uint64_t old_dur = get_packet_duration_ms(old_slot);
		size_t old_size = old_slot->size;

		if (old_slot->data && old_slot->size > 0) {
			obs_encoder_packet_release(old_slot);
		}
		memset(old_slot, 0, sizeof(struct encoder_packet));
		buf->tail = (buf->tail + 1) % buf->capacity;

		if (buf->buffered_time_ms >= old_dur)
			buf->buffered_time_ms -= old_dur;
		else
			buf->buffered_time_ms = 0;

		if (buf->memory_bytes >= old_size)
			buf->memory_bytes -= old_size;
		else
			buf->memory_bytes = 0;
	}

	struct encoder_packet *slot = &buf->packets[buf->head];
	if (slot->data && slot->size > 0) {
		obs_encoder_packet_release(slot);
	}
	obs_encoder_packet_create_instance(slot, packet);
	buf->head = (buf->head + 1) % buf->capacity;

	buf->buffered_time_ms += pkt_duration_ms;
	buf->memory_bytes += pkt_size;

	pthread_mutex_unlock(&buf->mutex);
	return true;
}

bool dynamic_delay_pop(struct dynamic_delay_buffer *buf, struct encoder_packet *out_packet)
{
	if (!buf || !out_packet)
		return false;

	pthread_mutex_lock(&buf->mutex);
	if (buf->head == buf->tail) {
		pthread_mutex_unlock(&buf->mutex);
		return false; // empty
	}

	struct encoder_packet *slot = &buf->packets[buf->tail];
	obs_encoder_packet_create_instance(out_packet, slot);

	uint64_t pkt_duration_ms = get_packet_duration_ms(slot);
	size_t pkt_size = slot->size;

	obs_encoder_packet_release(slot);
	memset(slot, 0, sizeof(struct encoder_packet));
	buf->tail = (buf->tail + 1) % buf->capacity;

	if (buf->buffered_time_ms >= pkt_duration_ms)
		buf->buffered_time_ms -= pkt_duration_ms;
	else
		buf->buffered_time_ms = 0;

	if (buf->memory_bytes >= pkt_size)
		buf->memory_bytes -= pkt_size;
	else
		buf->memory_bytes = 0;

	pthread_mutex_unlock(&buf->mutex);
	return true;
}

void dynamic_delay_clear(struct dynamic_delay_buffer *buf)
{
	if (!buf)
		return;
	pthread_mutex_lock(&buf->mutex);
	while (buf->head != buf->tail) {
		struct encoder_packet *slot = &buf->packets[buf->tail];
		if (slot->data && slot->size > 0)
			obs_encoder_packet_release(slot);
		memset(slot, 0, sizeof(struct encoder_packet));
		buf->tail = (buf->tail + 1) % buf->capacity;
	}
	buf->buffered_time_ms = 0;
	buf->memory_bytes = 0;
	pthread_mutex_unlock(&buf->mutex);
}

uint64_t dynamic_delay_buffered_ms(const struct dynamic_delay_buffer *buf)
{
	if (!buf)
		return 0;
	return buf->buffered_time_ms;
}

uint64_t dynamic_delay_get_memory_usage(const struct dynamic_delay_buffer *buf)
{
	if (!buf)
		return 0;
	return buf->memory_bytes;
}

size_t dynamic_delay_get_packet_count(const struct dynamic_delay_buffer *buf)
{
	if (!buf)
		return 0;
	if (buf->head >= buf->tail)
		return buf->head - buf->tail;
	return (buf->capacity - buf->tail) + buf->head;
}
