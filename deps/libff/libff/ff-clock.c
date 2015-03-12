/*
 * Copyright (c) 2015 John R. Bradley <jrb@turrettech.com>
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

#include "ff-clock.h"
#include "ff-threading.h"

#include <libavutil/avutil.h>

double ff_get_sync_clock(struct ff_clock *clock)
{
	return clock->sync_clock(clock->opaque);
}

struct ff_clock *ff_clock_init(struct ff_clock *clock)
{
	clock = av_mallocz(sizeof(struct ff_clock));

	if (clock == NULL)
		return NULL;

	if (pthread_mutex_init(&clock->mutex, NULL) != 0)
		goto fail;

	if (pthread_cond_init(&clock->cond, NULL) != 0)
		goto fail1;

	return clock;

fail1:
	pthread_mutex_destroy(&clock->mutex);
fail:
	av_free(clock);

	return NULL;
}

struct ff_clock *ff_clock_retain(struct ff_clock *clock)
{
	ff_atomic_inc_long(&clock->retain);

	return clock;
}

struct ff_clock *ff_clock_move(struct ff_clock **clock)
{
	struct ff_clock *retained_clock = ff_clock_retain(*clock);
	ff_clock_release(clock);

	return retained_clock;
}

void ff_clock_release(struct ff_clock **clock)
{
	if (ff_atomic_dec_long(&(*clock)->retain) == 0) {
		pthread_cond_destroy(&(*clock)->cond);
		pthread_mutex_destroy(&(*clock)->mutex);
		av_free(*clock);
	}

	*clock = NULL;
}
