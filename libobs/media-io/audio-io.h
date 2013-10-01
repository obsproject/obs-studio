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

#ifndef AUDIO_IO_H
#define AUDIO_IO_H

#include "../util/c99defs.h"
#include "media-io.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Base audio output component.  Use this to create an audio output track
 * for the media.
 */

struct audio_output;
typedef struct audio_output *audio_t;

enum audio_type {
	AUDIO_FORMAT_UNKNOWN,
	AUDIO_FORMAT_8BIT,
	AUDIO_FORMAT_16BIT,
	AUDIO_FORMAT_24BIT,
	AUDIO_FORMAT_32BIT,
	AUDIO_FORMAT_FLOAT,
};

enum speaker_setup {
	SPEAKERS_UNKNOWN,
	SPEAKERS_MONO,
	SPEAKERS_STEREO,
	SPEAKERS_2POINT1,
	SPEAKERS_QUAD,
	SPEAKERS_4POINT1,
	SPEAKERS_5POINT1,
	SPEAKERS_5POINT1_SURROUND,
	SPEAKERS_7POINT1,
	SPEAKERS_7POINT1_SURROUND,
	SPEAKERS_SURROUND,
};

struct audio_data {
	void            *data;
	uint32_t        frames;
	uint32_t        speakers;
	uint32_t        samples_per_sec;
	enum audio_type type;
	uint64_t        timestamp;
};

struct audio_info {
	const char         *name;
	const char         *format;

	uint32_t           channels;
	uint32_t           samples_per_sec;
	enum audio_type    type;
	enum speaker_setup speakers;
};

#define AUDIO_OUTPUT_SUCCESS       0
#define AUDIO_OUTPUT_INVALIDPARAM -1
#define AUDIO_OUTPUT_FAIL         -2

EXPORT int audio_output_open(audio_t *audio, media_t media,
		struct audio_info *info);
EXPORT void audio_output_data(audio_t audio, struct audio_data *data);
EXPORT void audio_output_close(audio_t audio);

#ifdef __cplusplus
}
#endif

#endif
