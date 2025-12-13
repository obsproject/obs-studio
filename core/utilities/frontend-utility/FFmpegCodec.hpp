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

struct FFmpegFormat;

struct FFmpegCodec {
	const char *name;
	const char *long_name;
	int id;

	FFmpegCodecType type;

	FFmpegCodec() = default;

	FFmpegCodec(const char *name, int id, FFmpegCodecType type = UNKNOWN)
		: name(name),
		  long_name(nullptr),
		  id(id),
		  type(type)
	{
	}

	FFmpegCodec(const AVCodec *codec) : name(codec->name), long_name(codec->long_name), id(codec->id)
	{
		switch (codec->type) {
		case AVMEDIA_TYPE_AUDIO:
			type = AUDIO;
			break;
		case AVMEDIA_TYPE_VIDEO:
			type = VIDEO;
			break;
		default:
			type = UNKNOWN;
		}
	}

	bool operator==(const FFmpegCodec &codec) const
	{
		if (id != codec.id)
			return false;

		return strequal(name, codec.name);
	}
};
Q_DECLARE_METATYPE(FFmpegCodec)

std::vector<FFmpegCodec> GetFormatCodecs(const FFmpegFormat &format, bool ignore_compatibility);

bool FFCodecAndFormatCompatible(const char *codec, const char *format);
bool IsBuiltinCodec(const char *codec);
bool ContainerSupportsCodec(const std::string &container, const std::string &codec);
