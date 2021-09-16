/*
 * Copyright (c) 2013 Hugh Bailey <obs.jim@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "c99defs.h"
#include "rng.h"
#include "platform.h"

/* generates a cryptographically secure random uint64 in the range
   min-max (inclusive) */
uint64_t random_uint64_bounded(uint64_t min, uint64_t max)
{
	uint64_t num;
	uint64_t range;

	assert(min <= max);

	if (min == max)
		return min;

	// take first sample
	os_random_bytes(&num, sizeof(num));

	range = max - min;

	if (range == UINT64_MAX)
		return num;

	// make it inclusive of max
	range++;

	// powers of 2 are bias-free
	if (range & (range - 1)) {
		uint64_t max_bias_limit = UINT64_MAX - (UINT64_MAX % range) - 1;

		// avoid bias by re-sampling if necessary
		while (num > max_bias_limit) {
			os_random_bytes(&num, sizeof(num));
		}
	}

	num %= range;
	num += min;

	return num;
}

/* cryptographically secure random int between 0 and UINT64_MAX (inclusive) */
uint64_t random_uint64(void)
{
	uint64_t num;
	os_random_bytes(&num, sizeof(num));
	return num;
}
