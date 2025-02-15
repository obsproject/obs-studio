/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
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

/*
 * Text Lookup interface
 *
 *   Used for storing and looking up localized strings.  Stores localization
 *   strings in a hashmap to efficiently look up associated strings via a
 *   unique string identifier name.
 */

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* opaque typedef */
struct text_lookup;
typedef struct text_lookup lookup_t;

/* functions */
EXPORT lookup_t *text_lookup_create(const char *path);
EXPORT bool text_lookup_add(lookup_t *lookup, const char *path);
EXPORT void text_lookup_destroy(lookup_t *lookup);
EXPORT bool text_lookup_getstr(lookup_t *lookup, const char *lookup_val, const char **out);

#ifdef __cplusplus
}
#endif
