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

#include <util/c99defs.h>
#include <util/circlebuf.h>
#include <media-io/audio-io.h>
#include <media-io/video-io.h>

#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

struct ffmpeg_data {
	AVStream           *video;
	AVStream           *audio;
	AVCodec            *acodec;
	AVCodec            *vcodec;
	AVFormatContext    *output;
	struct SwsContext  *swscale;

	AVPicture          dst_picture;
	AVFrame            *vframe;
	int                frame_size;
	int                total_frames;

	struct circlebuf   excess_frames[MAX_AUDIO_PLANES];
	uint8_t            *samples[MAX_AUDIO_PLANES];
	AVFrame            *aframe;
	int                total_samples;

	const char         *filename_test;

	bool               initialized;
};

struct ffmpeg_output {
	obs_output_t       output;
	volatile bool      active;
	struct ffmpeg_data ff_data;
};

EXPORT const char *ffmpeg_output_getname(const char *locale);

EXPORT struct ffmpeg_output *ffmpeg_output_create(obs_data_t settings,
		obs_output_t output);
EXPORT void ffmpeg_output_destroy(struct ffmpeg_output *data);

EXPORT void ffmpeg_output_update(struct ffmpeg_output *data,
		obs_data_t settings);

EXPORT bool ffmpeg_output_start(struct ffmpeg_output *data);
EXPORT void ffmpeg_output_stop(struct ffmpeg_output *data);

EXPORT bool ffmpeg_output_active(struct ffmpeg_output *data);
