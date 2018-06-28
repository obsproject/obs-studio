#include "audio-repack.h"

#include <emmintrin.h>

int check_buffer(struct audio_repack *repack,
		uint32_t frame_count)
{
	const uint32_t new_size = frame_count * repack->base_dst_size
			+ repack->extra_dst_size;

	if (repack->packet_size < new_size) {
		repack->packet_buffer = brealloc(
				repack->packet_buffer, new_size);
		if (!repack->packet_buffer)
			return -1;

		repack->packet_size = new_size;
	}

	return 0;
}

/*
 * Squash arrays.
 * For instance:
 * 2.1:
 *
 * | FL | FR | LFE | emp | emp | emp |emp |emp |
 * |    |    |
 * | FL | FR | LFE |
*/

int repack_squash(struct audio_repack *repack,
		const uint8_t *bsrc, uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;

	int squash = repack->extra_dst_size;
	const __m128i *src = (__m128i *)bsrc;
	const __m128i *esrc = src + frame_count;
	uint16_t *dst = (uint16_t *)repack->packet_buffer;

	/*  Audio needs squashing in order to avoid resampling issues.
	 */
	while (src != esrc) {
		__m128i target = _mm_load_si128(src++);
		_mm_storeu_si128((__m128i *)dst, target);
		dst += 8 - squash;
	}

	return 0;
}

int audio_repack_init(struct audio_repack *repack,
		audio_repack_mode_t repack_mode, uint8_t sample_bit)
{
	memset(repack, 0, sizeof(*repack));

	if (sample_bit != 16)
		return -1;

	repack->base_src_size = 8 * (16 / 8);
	repack->base_dst_size = (int)repack_mode * (16 / 8);
	repack->extra_dst_size = 8 - (int)repack_mode;
	repack->repack_func = &repack_squash;

	return 0;
}

void audio_repack_free(struct audio_repack *repack)
{
	if (repack->packet_buffer)
		bfree(repack->packet_buffer);

	memset(repack, 0, sizeof(*repack));
}
