/******************************************************************************
    Copyright (C) 2022 by Hugh Bailey <obs.jim@gmail.com>

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

#include "util/c99defs.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
	OBS_NAL_PRIORITY_DISPOSABLE = 0,
	OBS_NAL_PRIORITY_LOW = 1,
	OBS_NAL_PRIORITY_HIGH = 2,
	OBS_NAL_PRIORITY_HIGHEST = 3,
};

EXPORT const uint8_t *obs_nal_find_startcode(const uint8_t *p,
					     const uint8_t *end);

#ifdef __cplusplus
}
#endif
