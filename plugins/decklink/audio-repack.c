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
	Swap channel between LFE and FC, and
	squash data array

	| FL | FR |LFE | FC | BL | BR |emp |emp |
    |    |       x      |    |
	| FL | FR | FC |LFE | BL | BR |
 */
int repack_8to6ch_swap23(struct audio_repack *repack,
		const uint8_t *bsrc, uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;

	const uint32_t size = frame_count * repack->base_src_size;

	const __m128i *src = (__m128i *)bsrc;
	const __m128i *esrc = src + frame_count;
	uint32_t *dst = (uint32_t *)repack->packet_buffer;
	while (src != esrc) {
		__m128i target = _mm_load_si128(src++);
		__m128i buf = _mm_shufflelo_epi16(target, _MM_SHUFFLE(2, 3, 1, 0));
		_mm_storeu_si128((__m128i *)dst, buf);
		dst += 3;
	}

	return 0;
}

/*
	Swap channel between LFE and FC

	| FL | FR |LFE | FC | BL | BR |SBL |SBR |
	  |    |       x      |    |    |    |
	| FL | FR | FC |LFE | BL | BR |SBL |SBR |
 */
int repack_8ch_swap23(struct audio_repack *repack,
		const uint8_t *bsrc, uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;

	const uint32_t size = frame_count * repack->base_src_size;

	const __m128i *src = (__m128i *)bsrc;
	const __m128i *esrc = src + frame_count;
	__m128i *dst = (__m128i *)repack->packet_buffer;
	while (src != esrc) {
		__m128i target = _mm_load_si128(src++);
		__m128i buf = _mm_shufflelo_epi16(target, _MM_SHUFFLE(2, 3, 1, 0));
		_mm_store_si128(dst++, buf);
	}

	return 0;
}

int audio_repack_init(struct audio_repack *repack,
		audio_repack_mode_t repack_mode, uint8_t sample_bit)
{
	memset(repack, 0, sizeof(*repack));

	if (sample_bit != 16)
		return -1;

	switch (repack_mode) {
	case repack_mode_8to6ch_swap23:
		repack->base_src_size  = 8 * (16 / 8);
		repack->base_dst_size  = 6 * (16 / 8);
		repack->extra_dst_size = 2;
		repack->repack_func    = &repack_8to6ch_swap23;
		break;

	case repack_mode_8ch_swap23:
		repack->base_src_size  = 8 * (16 / 8);
		repack->base_dst_size  = 8 * (16 / 8);
		repack->extra_dst_size = 0;
		repack->repack_func    = &repack_8ch_swap23;
		break;

	default: return -1;
	}

	return 0;
}

void audio_repack_free(struct audio_repack *repack)
{
	if (repack->packet_buffer)
		bfree(repack->packet_buffer);

	memset(repack, 0, sizeof(*repack));
}
