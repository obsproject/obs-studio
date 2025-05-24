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

#include "FFmpegShared.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
}
#include <qmetatype.h>

struct FFmpegCodec;

struct FFmpegFormat {
	const char *name;
	const char *long_name;
	const char *mime_type;
	const char *extensions;
	AVCodecID audio_codec;
	AVCodecID video_codec;
	const AVCodecTag *const *codec_tags;

	FFmpegFormat() = default;

	FFmpegFormat(const char *name, const char *mime_type)
		: name(name),
		  long_name(nullptr),
		  mime_type(mime_type),
		  extensions(nullptr),
		  audio_codec(AV_CODEC_ID_NONE),
		  video_codec(AV_CODEC_ID_NONE),
		  codec_tags(nullptr)
	{
	}

	FFmpegFormat(const AVOutputFormat *av_format)
		: name(av_format->name),
		  long_name(av_format->long_name),
		  mime_type(av_format->mime_type),
		  extensions(av_format->extensions),
		  audio_codec(av_format->audio_codec),
		  video_codec(av_format->video_codec),
		  codec_tags(av_format->codec_tag)
	{
	}

	FFmpegCodec GetDefaultEncoder(FFmpegCodecType codec_type) const;

	bool HasAudio() const { return audio_codec != AV_CODEC_ID_NONE; }
	bool HasVideo() const { return video_codec != AV_CODEC_ID_NONE; }

	bool operator==(const FFmpegFormat &format) const
	{
		if (!strequal(name, format.name))
			return false;

		return strequal(mime_type, format.mime_type);
	}
};

Q_DECLARE_METATYPE(FFmpegFormat)

std::vector<FFmpegFormat> GetSupportedFormats();
