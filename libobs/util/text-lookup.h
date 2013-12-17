/******************************************************************************
  Copyright (c) 2013 by Hugh Bailey <obs.jim@gmail.com>

  This software is provided 'as-is', without any express or implied
  warranty. In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

     1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.

     2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.

     3. This notice may not be removed or altered from any source
     distribution.
******************************************************************************/

#pragma once

/*
 * Text Lookup interface
 *
 *   Used for storing and looking up localized strings.  Stores locazation
 * strings in a radix/trie tree to efficiently look up associated strings via a
 * unique string identifier name.
 */

#include "c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* opaque typdef */
struct text_lookup;
typedef struct text_lookup *lookup_t;

/* functions */
EXPORT lookup_t text_lookup_create(const char *path);
EXPORT bool text_lookup_add(lookup_t lookup, const char *path);
EXPORT void text_lookup_destroy(lookup_t lookup);
EXPORT bool text_lookup_getstr(lookup_t lookup, const char *lookup_val,
		const char **out);

#ifdef __cplusplus
}
#endif
