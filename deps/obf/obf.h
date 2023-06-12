// SPDX-FileCopyrightText: 2023 Lain Bailey <lain@obsproject.com>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void deobfuscate_str(char *str, uint64_t val);

#ifdef __cplusplus
}
#endif
