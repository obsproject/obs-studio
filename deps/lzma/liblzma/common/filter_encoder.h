///////////////////////////////////////////////////////////////////////////////
//
/// \file       filter_encoder.c
/// \brief      Filter ID mapping to filter-specific functions
//
//  Author:     Lasse Collin
//
//  This file has been put into the public domain.
//  You can do whatever you want with this file.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef LZMA_FILTER_ENCODER_H
#define LZMA_FILTER_ENCODER_H

#include "common.h"


// FIXME: Might become a part of the public API once finished.
// extern lzma_vli lzma_chunk_size(const lzma_filter *filters);


extern lzma_ret lzma_raw_encoder_init(
		lzma_next_coder *next, lzma_allocator *allocator,
		const lzma_filter *filters);

#endif
