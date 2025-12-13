/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

/* TODO: incomplete/may not be necessary */

#ifdef __cplusplus
extern "C" {
#endif

/* wrapped opaque data types */
struct input_subsystem;
typedef struct input_subsystem input_t;

EXPORT int input_getbuttonstate(input_t *input, uint32_t button);

#ifdef __cplusplus
}
#endif
