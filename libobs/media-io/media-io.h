/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
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
 * Media input/output components used for connecting media outputs/inputs
 * together.  An input requests a connection to an output and the output
 * sends frames to it through the callbacks in media_data_in structure.
 *
 * The id member should indicate the format/parameters used in text form.
 */

/* opaque data types */
struct media_input;
struct media_output;
struct media_data;
typedef struct media_input  *media_input_t;
typedef struct media_output *media_output_t;
typedef struct media_data   *media_t;

#include "../util/c99defs.h"
#include "video-io.h"
#include "audio-io.h"

#ifdef __cplusplus
extern "C" {
#endif

struct media_input_info {
	const char *format;

	void *obj;
	void (*on_input)(void *obj, const void *data);
};

struct media_output_info {
	const char *format;

	void *obj;
	bool (*connect)(void *obj, media_input_t input);
};

EXPORT media_input_t media_input_create(struct media_input_info *info);
EXPORT void          media_input_destroy(media_input_t input);

EXPORT media_output_t media_output_create(struct media_output_info *info);
EXPORT void           media_output_data(media_output_t out, const void *data);
EXPORT void           media_output_destroy(media_output_t output);

EXPORT media_t media_open(void);
EXPORT bool    media_add_input(media_t media, media_input_t input);
EXPORT void    media_add_output(media_t media, media_output_t output);
EXPORT void    media_remove_input(media_t media, media_input_t input);
EXPORT void    media_remove_output(media_t media, media_output_t output);
EXPORT void    media_close(media_t media);

#ifdef __cplusplus
}
#endif
