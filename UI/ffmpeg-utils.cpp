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

#include "ffmpeg-utils.hpp"

#include <unordered_map>
#include <unordered_set>

extern "C" {
#include <libavformat/avformat.h>
}

using namespace std;

static void GetCodecsForId(const FFmpegFormat &format,
			   vector<FFmpegCodec> &codecs, enum AVCodecID id,
			   bool ignore_compaibility)
{

	const AVCodec *codec = nullptr;
	void *i = 0;

	while ((codec = av_codec_iterate(&i)) != nullptr) {
		if (codec->id != id)
			continue;
		// Not an encoding codec
		if (!av_codec_is_encoder(codec))
			continue;
		// Skip if not supported and compatibility check not disabled
		if (!ignore_compaibility &&
		    !av_codec_get_tag(format.codec_tags, codec->id)) {
			continue;
		}

		FFmpegCodec d{codec->name, codec->long_name, codec->id};

		const AVCodec *base_codec = avcodec_find_encoder(codec->id);
		if (strcmp(base_codec->name, codec->name) != 0) {
			d.alias = true;
			d.base_name = base_codec->name;
		}

		switch (codec->type) {
		case AVMEDIA_TYPE_AUDIO:
			d.type = FFmpegCodecType::AUDIO;
			break;
		case AVMEDIA_TYPE_VIDEO:
			d.type = FFmpegCodecType::VIDEO;
			break;
		default:
			d.type = FFmpegCodecType::UNKNOWN;
		}

		codecs.push_back(d);
	}
}

static std::vector<const AVCodecDescriptor *> GetCodecDescriptors()
{
	std::vector<const AVCodecDescriptor *> codecs;

	const AVCodecDescriptor *desc = nullptr;
	while ((desc = avcodec_descriptor_next(desc)) != nullptr)
		codecs.push_back(desc);

	return codecs;
}

vector<FFmpegCodec> GetFormatCodecs(const FFmpegFormat &format,
				    bool ignore_compatibility)
{
	vector<FFmpegCodec> codecs;
	auto codecDescriptors = GetCodecDescriptors();

	if (codecDescriptors.empty())
		return codecs;

	for (const AVCodecDescriptor *codec : codecDescriptors)
		GetCodecsForId(format, codecs, codec->id, ignore_compatibility);

	return codecs;
}

static inline bool is_output_device(const AVClass *avclass)
{
	if (!avclass)
		return 0;

	switch (avclass->category) {
	case AV_CLASS_CATEGORY_DEVICE_VIDEO_OUTPUT:
	case AV_CLASS_CATEGORY_DEVICE_AUDIO_OUTPUT:
	case AV_CLASS_CATEGORY_DEVICE_OUTPUT:
		return true;
	default:
		return false;
	}
}

vector<FFmpegFormat> GetSupportedFormats()
{
	vector<FFmpegFormat> formats;
	const AVOutputFormat *output_format;

	void *i = 0;
	while ((output_format = av_muxer_iterate(&i)) != nullptr) {
		if (is_output_device(output_format->priv_class))
			continue;

		formats.push_back({
			output_format->name,
			output_format->long_name,
			output_format->mime_type,
			output_format->extensions,
			output_format->audio_codec,
			output_format->video_codec,
			output_format->codec_tag,
		});
	}

	return formats;
}

static const char *get_encoder_name(const char *format_name,
				    enum AVCodecID codec_id)
{
	const AVCodec *codec = avcodec_find_encoder(codec_id);
	if (codec == nullptr && codec_id == AV_CODEC_ID_NONE)
		return nullptr;
	else if (codec == nullptr)
		return format_name;
	else
		return codec->name;
}

const char *FFmpegFormat::GetDefaultName(FFmpegCodecType codec_type) const
{

	switch (codec_type) {
	case FFmpegCodecType::AUDIO:
		return get_encoder_name(name, audio_codec);
	case FFmpegCodecType::VIDEO:
		return get_encoder_name(name, video_codec);
	default:
		return nullptr;
	}
}

bool FFCodecAndFormatCompatible(const char *codec, const char *format)
{
	if (!codec || !format)
		return false;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(59, 0, 100)
	AVOutputFormat *output_format;
#else
	const AVOutputFormat *output_format;
#endif
	output_format = av_guess_format(format, NULL, NULL);
	if (!output_format)
		return false;

	const AVCodecDescriptor *codec_desc =
		avcodec_descriptor_get_by_name(codec);
	if (!codec_desc)
		return false;

#if LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(60, 0, 100)
	return avformat_query_codec(output_format, codec_desc->id,
				    FF_COMPLIANCE_EXPERIMENTAL) == 1;
#else
	return avformat_query_codec(output_format, codec_desc->id,
				    FF_COMPLIANCE_NORMAL) == 1;
#endif
}

static const unordered_set<string> builtin_codecs = {
	"h264", "hevc", "av1",       "prores",    "aac",       "opus",
	"alac", "flac", "pcm_s16le", "pcm_s24le", "pcm_f32le",
};

bool IsBuiltinCodec(const char *codec)
{
	return builtin_codecs.count(codec) > 0;
}

static const unordered_map<string, unordered_set<string>> codec_compat = {
	// Technically our muxer supports HEVC and AV1 as well, but nothing else does
	{"flv",
	 {
		 "h264",
		 "aac",
	 }},
	{"mpegts",
	 {
		 "h264",
		 "hevc",
		 "aac",
		 "opus",
	 }},
	{"hls",
	 // Also using MPEG-TS in our case, but no Opus support
	 {
		 "h264",
		 "hevc",
		 "aac",
	 }},
	{"mp4",
	 {
		 "h264",
		 "hevc",
		 "av1",
		 "aac",
		 "opus",
		 "alac",
		 "flac",
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(60, 5, 100)
		 // PCM in MP4 is only supported in FFmpeg > 6.0
		 "pcm_s16le",
		 "pcm_s24le",
		 "pcm_f32le",
#endif
	 }},
	{"fragmented_mp4",
	 {
		 "h264",
		 "hevc",
		 "av1",
		 "aac",
		 "opus",
		 "alac",
		 "flac",
#if LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(60, 5, 100)
		 "pcm_s16le",
		 "pcm_s24le",
		 "pcm_f32le",
#endif
	 }},
	{"mov",
	 {
		 "h264",
		 "hevc",
		 "prores",
		 "aac",
		 "alac",
		 "pcm_s16le",
		 "pcm_s24le",
		 "pcm_f32le",
	 }},
	{"fragmented_mov",
	 {
		 "h264",
		 "hevc",
		 "prores",
		 "aac",
		 "alac",
		 "pcm_s16le",
		 "pcm_s24le",
		 "pcm_f32le",
	 }},
	// MKV supports everything
	{"mkv", {}},
};

bool ContainerSupportsCodec(const string &container, const string &codec)
{
	auto iter = codec_compat.find(container);
	if (iter == codec_compat.end())
		return false;

	auto codecs = iter->second;
	// Assume everything is supported
	if (codecs.empty())
		return true;

	return codecs.count(codec) > 0;
}
