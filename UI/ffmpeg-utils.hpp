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

#include <qmetatype.h>
#include <string>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
}

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

struct FFmpegFormat {
	const char *name;
	const char *long_name;
	const char *mime_type;
	const char *extensions;
	AVCodecID audio_codec;
	AVCodecID video_codec;
	const AVCodecTag *const *codec_tags;

	FFmpegFormat() = default;

	const char *GetDefaultName(FFmpegCodecType codec_type) const;

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

struct FFmpegCodec {
	const char *name;
	const char *long_name;
	int id;

	bool alias;
	const char *base_name;

	FFmpegCodecType type;

	FFmpegCodec() = default;

	bool operator==(const FFmpegCodec &codec) const
	{
		if (id != codec.id)
			return false;

		return strequal(name, codec.name);
	}
};
Q_DECLARE_METATYPE(FFmpegCodec)

std::vector<FFmpegFormat> GetSupportedFormats();
std::vector<FFmpegCodec> GetFormatCodecs(const FFmpegFormat &format,
					 bool ignore_compatibility);

bool FFCodecAndFormatCompatible(const char *codec, const char *format);
bool IsBuiltinCodec(const char *codec);
bool ContainerSupportsCodec(const std::string &container,
			    const std::string &codec);
