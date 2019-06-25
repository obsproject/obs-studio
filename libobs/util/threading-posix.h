/*
 * Copyright (c) 2015 Hugh Bailey <obs.jim@gmail.com>
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

#pragma once

static inline long os_atomic_inc_long(volatile long *val)
{
	return __sync_add_and_fetch(val, 1);
}

static inline long os_atomic_dec_long(volatile long *val)
{
	return __sync_sub_and_fetch(val, 1);
}

static inline long os_atomic_set_long(volatile long *ptr, long val)
{
	return __sync_lock_test_and_set(ptr, val);
}

static inline long os_atomic_load_long(const volatile long *ptr)
{
	return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}

static inline bool os_atomic_compare_swap_long(volatile long *val, long old_val,
					       long new_val)
{
	return __sync_bool_compare_and_swap(val, old_val, new_val);
}

static inline bool os_atomic_set_bool(volatile bool *ptr, bool val)
{
	return __sync_lock_test_and_set(ptr, val);
}

static inline bool os_atomic_load_bool(const volatile bool *ptr)
{
	return __atomic_load_n(ptr, __ATOMIC_SEQ_CST);
}
