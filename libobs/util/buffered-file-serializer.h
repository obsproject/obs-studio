/*
 * Copyright (c) 2024 Dennis Sädtler <dennis@obsproject.com>
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

#include "serializer.h"

#ifdef __cplusplus
extern "C" {
#endif

EXPORT bool buffered_file_serializer_init_defaults(struct serializer *s,
						   const char *path);
EXPORT bool buffered_file_serializer_init(struct serializer *s,
					  const char *path, size_t max_bufsize,
					  size_t chunk_size);
EXPORT void buffered_file_serializer_free(struct serializer *s);

#ifdef __cplusplus
}
#endif
