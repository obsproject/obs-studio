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
#include <obs.h>

struct rtmp_stream {
	obs_output_t  output;
	obs_encoder_t video_encoder;
	obs_encoder_t audio_encoder;
	obs_service_t service;

	bool active;
};

EXPORT const char *rtmp_stream_getname(const char *locale);
EXPORT void *rtmp_stream_create(const char *settings, obs_output_t output);
EXPORT void rtmp_stream_destroy(struct rtmp_stream *stream);
EXPORT void rtmp_stream_update(struct rtmp_stream *stream,
		const char *settings);
EXPORT bool rtmp_stream_start(struct rtmp_stream *stream);
EXPORT void rtmp_stream_stop(struct rtmp_stream *stream);
EXPORT bool rtmp_stream_active(struct rtmp_stream *stream);
