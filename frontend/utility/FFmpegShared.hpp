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

#include <string.h>

enum FFmpegCodecType { AUDIO, VIDEO, UNKNOWN };

/* This needs to handle a few special cases due to how the format is used in the UI:
 * - strequal(nullptr, "") must be true
 * - strequal("", nullptr) must be true
 * - strequal(nullptr, nullptr) must be true
 */
static bool strequal(const char *a, const char *b)
{
	if (!a && !b)
		return true;
	if (!a && *b == 0)
		return true;
	if (!b && *a == 0)
		return true;
	if (!a || !b)
		return false;

	return strcmp(a, b) == 0;
}
