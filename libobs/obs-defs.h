/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

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

/* Maximum number of source channels for output and per display */
#define MAX_CHANNELS 32

#define MODULE_SUCCESS           0
#define MODULE_ERROR            -1
#define MODULE_FILENOTFOUND     -2
#define MODULE_FUNCTIONNOTFOUND -3
#define MODULE_INCOMPATIBLE_VER -4

#define SOURCE_VIDEO          (1<<0) /* Source has video */
#define SOURCE_AUDIO          (1<<1) /* Source has audio */
#define SOURCE_ASYNC_VIDEO    (1<<2) /* Async video (use with SOURCE_VIDEO) */
