#include "audio-repack.h"

#define USE_SIMD
#ifdef USE_SIMD
#include <util/sse-intrin.h>
#endif

#define NUM_CHANNELS 8 /* max until OBS supports higher channel counts */

int check_buffer(struct audio_repack *repack, uint32_t frame_count)
{
	const uint32_t new_size =
		frame_count * repack->base_dst_size + repack->pad_dst_size;

	if (repack->packet_size < new_size) {
		repack->packet_buffer =
			brealloc(repack->packet_buffer, new_size);
		if (!repack->packet_buffer)
			return -1;
		repack->packet_size = new_size;
	}

	return 0;
}

#ifdef USE_SIMD
/*
 * Squash array of 8ch to new channel count
 * 16-bit PCM, SIMD version
 * For instance:
 * 2.1:
 *
 * | FL | FR | LFE | emp | emp | emp |emp |emp |
 * |    |    |
 * | FL | FR | LFE |
*/
int repack_squash16(struct audio_repack *repack, const uint8_t *bsrc,
		    uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;

	int squash = repack->squash_count;
	const __m128i *src = (__m128i *)bsrc;
	const __m128i *end = src + frame_count;
	uint16_t *dst = (uint16_t *)repack->packet_buffer;

	/*  Audio needs squashing in order to avoid resampling issues.
	 * The condition checks for 7.1 audio for which no squash is needed.
	 */
	if (squash > 0) {
		while (src != end) {
			__m128i target = _mm_load_si128(src++);
			_mm_storeu_si128((__m128i *)dst, target);
			dst += NUM_CHANNELS - squash;
		}
	}

	return 0;
}

/*
 * Squash array of 8ch and swap Front Center channel with LFE
 * 16-bit PCM, SIMD version
 * For instance:
 * 2.1:
 *
 * | FL | FR | FC | LFE | RL | RR | LC | RC |
 * |    |    |
 * | FL | FR | LFE | FC | RL | RR | LC | RC |
*/
int repack_squash_swap16(struct audio_repack *repack, const uint8_t *bsrc,
			 uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;
	int squash = repack->squash_count;
	const __m128i *src = (__m128i *)bsrc;
	const __m128i *end = src + frame_count;
	uint16_t *dst = (uint16_t *)repack->packet_buffer;
	while (src != end) {
		__m128i target = _mm_load_si128(src++);
		__m128i buf =
			_mm_shufflelo_epi16(target, _MM_SHUFFLE(2, 3, 1, 0));
		_mm_storeu_si128((__m128i *)dst, buf);
		dst += NUM_CHANNELS - squash;
	}
	return 0;
}

/*
 * Squash array of 8ch to new channel count
 * 32-bit PCM, SIMD version
 */
int repack_squash32(struct audio_repack *repack, const uint8_t *bsrc,
		    uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;

	int squash = repack->squash_count;
	const __m128i *src = (__m128i *)bsrc;
	const __m128i *end =
		(__m128i *)(bsrc + (frame_count * repack->base_src_size));
	uint32_t *dst = (uint32_t *)repack->packet_buffer;

	if (squash > 0) {
		while (src != end) {
			__m128i tgt_lo = _mm_loadu_si128(src++);
			__m128i tgt_hi = _mm_loadu_si128(src++);
			_mm_storeu_si128((__m128i *)dst, tgt_lo);
			dst += 4;
			_mm_storeu_si128((__m128i *)dst, tgt_hi);
			dst += 4;
			dst -= squash;
		}
	}

	return 0;
}

/*
 * Squash array of 8ch to new channel count and swap FC with LFE
 * 32-bit PCM, SIMD version
 */
int repack_squash_swap32(struct audio_repack *repack, const uint8_t *bsrc,
			 uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;
	int squash = repack->squash_count;
	const __m128i *src = (__m128i *)bsrc;
	const __m128i *end =
		(__m128i *)(bsrc + (frame_count * repack->base_src_size));
	uint32_t *dst = (uint32_t *)repack->packet_buffer;
	while (src != end) {
		__m128i tgt_lo = _mm_shuffle_epi32(_mm_loadu_si128(src++),
						   _MM_SHUFFLE(2, 3, 1, 0));
		__m128i tgt_hi = _mm_loadu_si128(src++);
		_mm_storeu_si128((__m128i *)dst, tgt_lo);
		dst += 4;
		_mm_storeu_si128((__m128i *)dst, tgt_hi);
		dst += 4;
		dst -= squash;
	}
	return 0;
}

#else

/*
 * Squash array of 8ch to new channel count
 * 16-bit or 32-bit PCM, C version
 */
int repack_squash(struct audio_repack *repack, const uint8_t *bsrc,
		  uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;
	int squash = repack->squash_count;
	const uint8_t *src = bsrc;
	const uint8_t *end = src + frame_count * repack->base_src_size;
	uint8_t *dst = repack->packet_buffer;
	uint32_t new_channel_count = NUM_CHANNELS - squash;
	if (squash > 0) {
		while (src != end) {
			memcpy(dst, src,
			       repack->bytes_per_sample * new_channel_count);
			dst += (new_channel_count * repack->bytes_per_sample);
			src += NUM_CHANNELS * repack->bytes_per_sample;
		}
	}

	return 0;
}

void shuffle_8ch(uint8_t *dst, const uint8_t *src, size_t szb, int ch1, int ch2,
		 int ch3, int ch4, int ch5, int ch6, int ch7, int ch8)
{
	/* shuffle 8 channels of audio */
	for (size_t i = 0; i < szb; i++) {
		dst[0 * szb + i] = src[ch1 * szb + i];
		dst[1 * szb + i] = src[ch2 * szb + i];
		dst[2 * szb + i] = src[ch3 * szb + i];
		dst[3 * szb + i] = src[ch4 * szb + i];
		dst[4 * szb + i] = src[ch5 * szb + i];
		dst[5 * szb + i] = src[ch6 * szb + i];
		dst[6 * szb + i] = src[ch7 * szb + i];
		dst[7 * szb + i] = src[ch8 * szb + i];
	}
}

/*
 * Squash array of 8ch to new channel count and swap FC with LFE
 * 16-bit or 32-bit PCM, C version
 */
int repack_squash_swap(struct audio_repack *repack, const uint8_t *bsrc,
		       uint32_t frame_count)
{
	if (check_buffer(repack, frame_count) < 0)
		return -1;

	int squash = repack->squash_count;
	const uint8_t *src = bsrc;
	const uint8_t *end = src + (frame_count * repack->base_src_size);
	uint8_t *dst = repack->packet_buffer;
	uint32_t new_channel_count = NUM_CHANNELS - squash;
	if (squash > 0) {
		while (src != end) {
			shuffle_8ch(dst, src, 4, 0, 1, 3, 2, 4, 5, 6, 7);
			dst += (new_channel_count * repack->bytes_per_sample);
			src += (NUM_CHANNELS * repack->bytes_per_sample);
		}
	}

	return 0;
}
#endif

int audio_repack_init(struct audio_repack *repack,
		      audio_repack_mode_t repack_mode, uint8_t bits_per_sample)
{
	memset(repack, 0, sizeof(*repack));

	if (bits_per_sample != 16 && bits_per_sample != 32)
		return -1;
	int _audio_repack_ch[9] = {2, 3, 4, 5, 6, 5, 6, 8, 8};
	int bytes_per_sample = (bits_per_sample / 8);
	repack->bytes_per_sample = bytes_per_sample;
	repack->base_src_size = NUM_CHANNELS * bytes_per_sample;
	repack->base_dst_size =
		_audio_repack_ch[repack_mode] * bytes_per_sample;
	uint32_t squash_count = NUM_CHANNELS - _audio_repack_ch[repack_mode];
	repack->pad_dst_size = squash_count * bytes_per_sample;
	repack->squash_count = squash_count;

#ifdef USE_SIMD
	if (bits_per_sample == 16) {
		repack->repack_func = &repack_squash16;
		if (repack_mode == repack_mode_8to5ch_swap ||
		    repack_mode == repack_mode_8to6ch_swap ||
		    repack_mode == repack_mode_8ch_swap)
			repack->repack_func = &repack_squash_swap16;
	} else if (bits_per_sample == 32) {
		repack->repack_func = &repack_squash32;
		if (repack_mode == repack_mode_8to5ch_swap ||
		    repack_mode == repack_mode_8to6ch_swap ||
		    repack_mode == repack_mode_8ch_swap)
			repack->repack_func = &repack_squash_swap32;
	}
#else
	repack->repack_func = &repack_squash;
	if (repack_mode == repack_mode_8to5ch_swap ||
	    repack_mode == repack_mode_8to6ch_swap ||
	    repack_mode == repack_mode_8ch_swap)
		repack->repack_func = &repack_squash_swap;
#endif

	return 0;
}

void audio_repack_free(struct audio_repack *repack)
{
	if (repack->packet_buffer)
		bfree(repack->packet_buffer);

	memset(repack, 0, sizeof(*repack));
}
