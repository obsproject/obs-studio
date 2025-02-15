/******************************************************************************
    Copyright (C) 2019 by Peter Geis <pgwipeout@gmail.com>

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

#include "c99defs.h"

#if (defined(_MSC_VER) || defined(__MINGW32__)) && ((defined(_M_X64) && !defined(_M_ARM64EC)) || defined(_M_IX86))
#include <emmintrin.h>
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define SIMDE_ENABLE_NATIVE_ALIASES
PRAGMA_WARN_PUSH
#include "simde/x86/sse2.h"
PRAGMA_WARN_POP
#endif
