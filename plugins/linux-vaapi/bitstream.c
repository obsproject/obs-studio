#include "bitstream.h"
#include <util/darray.h>

#define BS_ROUNDUP_DIV(x, sz) ((x+(sz-1))/sz)
#define BS_CACHE_BYTE_SIZE (sizeof(uint64_t))
#define BS_CACHE_BIT_SIZE (8 * BS_CACHE_BYTE_SIZE)

struct bitstream
{
	DARRAY(uint8_t) buf;
	uint64_t cache;
	size_t cache_rem;
	int zero_cnt;
	bool ep;
};
typedef struct bitstream bitstream_t;

bitstream_t *bs_create()
{
	bitstream_t *bs = bzalloc(sizeof(bitstream_t));
	bs_reset(bs);
	return bs;
}

void bs_free(bitstream_t *bs)
{
	if (bs != NULL) {
		da_free(bs->buf);
		bfree(bs);
	}
}

#if 0
#define SWAP64(x) swap64(x)
static inline uint64_t swap64(uint64_t x) {
	return ((x & 0x00000000000000ffull) << 56) |
	       ((x & 0x000000000000ff00ull) << 40) |
	       ((x & 0x0000000000ff0000ull) << 24) |
	       ((x & 0x00000000ff000000ull) << 8)  |
	       ((x & 0x000000ff00000000ull) >> 8)  |
	       ((x & 0x0000ff0000000000ull) >> 24) |
	       ((x & 0x00ff000000000000ull) >> 40) |
	       ((x & 0xff00000000000000ull) >> 56);
}
#else
#define SWAP64(x) htobe64(x)
#endif

void bs_flush(bitstream_t *bs)
{
	size_t cache_bit_cnt = BS_CACHE_BIT_SIZE - bs->cache_rem;

	// nothing to do
	if (cache_bit_cnt == 0)
		return;

	size_t cache_byte_cnt = BS_ROUNDUP_DIV(cache_bit_cnt, 8);
	bs->cache <<= (BS_CACHE_BIT_SIZE - cache_bit_cnt);
	bs->cache = SWAP64(bs->cache);

	da_push_back_array(bs->buf, &bs->cache, cache_byte_cnt);

	bs->cache = 0;
	bs->cache_rem = BS_CACHE_BIT_SIZE;
}

void bs_append_bits(bitstream_t *bs, uint32_t bit_cnt,
		uint64_t val)
{
	while(bit_cnt > 0) {
		if (bs->cache_rem == 0)
			bs_flush(bs);

		uint64_t out_cnt = bit_cnt > bs->cache_rem
				? bs->cache_rem : bit_cnt;
		uint64_t out_val = (val >> (bit_cnt - out_cnt));
		if (out_cnt < BS_CACHE_BIT_SIZE)
			out_val &= ((1ull << out_cnt) - 1);
		bs->cache <<= out_cnt;
		bs->cache |= out_val;
		bit_cnt -= out_cnt;
		bs->cache_rem -= out_cnt;
	}
}

void bs_append_bool(bitstream_t *bs, bool val)
{
	if (bs->cache_rem == 0)
		bs_flush(bs);

	bs->cache <<= 1;
	bs->cache |= val ? 1ull : 0ull;
	--bs->cache_rem;
}

void bs_append_ue(bitstream_t *bs, uint32_t val)
{
	size_t zero_cnt = 0;
	uint32_t v = val + 1;

	while (v > 1) {
		v >>= 1;
		++zero_cnt;
	}

	bs_append_bits(bs, zero_cnt, 0);
	bs_append_bits(bs, zero_cnt + 1, val + 1);
}

void bs_append_se(bitstream_t *bs, int32_t val)
{
	if (val > 0)
		bs_append_ue(bs, val * 2 - 1);
	else
		bs_append_ue(bs, -val * 2);
}

void bs_append_bs(bitstream_t *bs, bitstream_t *src)
{
	for(size_t i = 0; i < src->buf.num; i++)
		bs_append_bits(bs, 8, src->buf.array[i]);
}

void bs_begin_nalu(bitstream_t *bs, uint32_t nalu_type,
		uint32_t nal_ref_idc)
{
	bs_append_bits(bs, 32, 0x00000001);
	bs_append_bits(bs, 1, 0);
	bs_append_bits(bs, 2, nal_ref_idc);
	bs_append_bits(bs, 5, nalu_type);
	bs_flush(bs);
}

void bs_append_trailing_bits(bitstream_t *bs)
{
	bs_append_bits(bs, bs->cache_rem % 8, 0);
	bs_flush(bs);
}

void bs_end_nalu(bitstream_t *bs)
{
	bs_append_bits(bs, 1, 1);
	bs_append_trailing_bits(bs);
}

void bs_reset(bitstream_t *bs)
{
	da_resize(bs->buf, 0);
	bs->cache = 0;
	bs->cache_rem = BS_CACHE_BIT_SIZE;
	bs->zero_cnt = 0;
}

uint8_t *bs_data(bitstream_t *bs)
{
	return bs->buf.array;
}

size_t bs_size(bitstream_t *bs)
{
	return bs->buf.num;
}
