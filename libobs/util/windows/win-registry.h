/*
 * Copyright (c) 2023 Lain Bailey <lain@obsproject.com>
 * Copyright (c) 2017 Ryan Foster <RytoEX@gmail.com>
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

#include <windows.h>
#include "../c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

struct reg_dword {
	LSTATUS status;
	DWORD size;
	DWORD return_value;
};

EXPORT void get_reg_dword(HKEY hkey, LPCWSTR sub_key, LPCWSTR value_name, struct reg_dword *info);

#ifdef __cplusplus
}
#endif
