/******************************************************************************
    Copyright (C) 2023 by Dennis Sädtler <dennis@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

/*
 * This file (re)defines various uthash settings for use in libobs
 */

#include <uthash/uthash.h>

/* Use OBS allocator */
#undef uthash_malloc
#undef uthash_free
#define uthash_malloc(sz) bmalloc(sz)
#define uthash_free(ptr, sz) bfree(ptr)

/* Use SFH (Super Fast Hash) function instead of JEN */
#undef HASH_FUNCTION
#define HASH_FUNCTION HASH_SFH
