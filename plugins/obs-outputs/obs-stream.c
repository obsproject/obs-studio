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

#include "obs-stream.h"

const char *rtmp_stream_getname(const char *locale)
{
	/* TODO: locale stuff */
	return "RTMP Stream";
}

void *rtmp_stream_create(const char *settings, obs_output_t output)
{
	struct rtmp_stream *stream = bmalloc(sizeof(struct rtmp_stream));
	memset(stream, 0, sizeof(struct rtmp_stream));
}

void rtmp_stream_destroy(struct rtmp_stream *stream)
{
}

void rtmp_stream_update(struct rtmp_stream *stream, const char *settings)
{
}

bool rtmp_stream_start(struct rtmp_stream *stream)
{
}

void rtmp_stream_stop(struct rtmp_stream *stream)
{
}

bool rtmp_stream_active(struct rtmp_stream *stream)
{
}
