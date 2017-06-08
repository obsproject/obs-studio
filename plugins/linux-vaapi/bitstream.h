#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct bitstream;
typedef struct bitstream bitstream_t;

void bs_free(bitstream_t *bs);
bitstream_t *bs_create();
size_t bs_size(bitstream_t *bs);
uint8_t *bs_data(bitstream_t *bs);
void bs_flush(bitstream_t *bs);
void bs_append_trailing_bits(bitstream_t *bs);
void bs_append_bits(bitstream_t *bs, uint32_t bit_cnt, uint64_t val);
void bs_append_bool(bitstream_t *bs, bool val);
void bs_append_ue(bitstream_t *bs, uint32_t val);
void bs_append_se(bitstream_t *bs, int32_t val);
void bs_append_bs(bitstream_t *bs, bitstream_t *src);
void bs_begin_nalu(bitstream_t *bs, uint32_t nalu_type,
		uint32_t nal_ref_idc);
void bs_end_nalu(bitstream_t *bs);
void bs_reset(bitstream_t *bs);
