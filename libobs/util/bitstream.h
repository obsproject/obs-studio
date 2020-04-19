#pragma once

#include "serializer.h"

struct bitstream_reader {
	uint8_t pos;
	uint8_t subPos;
	uint8_t *buf;
	size_t len;
};

void bitstream_reader_init(struct bitstream_reader *r, uint8_t *data,
			   size_t len);
uint8_t bitstream_reader_read_bits(struct bitstream_reader *r, int bits);
uint8_t bitstream_reader_r8(struct bitstream_reader *r);
uint16_t bitstream_reader_r16(struct bitstream_reader *r);
