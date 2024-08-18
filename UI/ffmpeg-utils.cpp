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

vector<FFmpegCodec> GetFormatCodecs(const FFmpegFormat &format,
				    bool ignore_compatibility)
{
	vector<FFmpegCodec> codecs;
	const AVCodec *codec;
	void *i = 0;

	while ((codec = av_codec_iterate(&i)) != nullptr) {
		// Not an encoding codec
		if (!av_codec_is_encoder(codec))
			continue;
		// Skip if not supported and compatibility check not disabled
		if (!ignore_compatibility &&
		    !av_codec_get_tag(format.codec_tags, codec->id)) {
			continue;
		}

		codecs.emplace_back(codec);
	}

	return codecs;
}

static bool is_output_device(const AVClass *avclass)
{
	if (!avclass)
		return false;

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

		formats.emplace_back(output_format);
	}

	return formats;
}

FFmpegCodec FFmpegFormat::GetDefaultEncoder(FFmpegCodecType codec_type) const
{
	const AVCodecID codec_id = codec_type == VIDEO ? video_codec
						       : audio_codec;
	if (codec_type == UNKNOWN || codec_id == AV_CODEC_ID_NONE)
		return {};

	if (auto codec = avcodec_find_encoder(codec_id))
		return {codec};

	/* Fall back to using the format name as the encoder,
	 * this works for some formats such as FLV. */
	return FFmpegCodec{name, codec_id, codec_type};
}

bool FFCodecAndFormatCompatible(const char *codec, const char *format)
{
	if (!codec || !format)
		return false;

	const AVOutputFormat *output_format =
		av_guess_format(format, nullptr, nullptr);
	if (!output_format)
		return false;

	const AVCodecDescriptor *codec_desc =
		avcodec_descriptor_get_by_name(codec);
	if (!codec_desc)
		return false;

	return avformat_query_codec(output_format, codec_desc->id,
				    FF_COMPLIANCE_NORMAL) == 1;
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
		 "pcm_s16le",
		 "pcm_s24le",
		 "pcm_f32le",
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
		 "pcm_s16le",
		 "pcm_s24le",
		 "pcm_f32le",
	 }},
	// Not part of FFmpeg, see obs-outputs module
	{"hybrid_mp4",
	 {
		 "h264",
		 "hevc",
		 "av1",
		 "aac",
		 "opus",
		 "alac",
		 "flac",
		 "pcm_s16le",
		 "pcm_s24le",
		 "pcm_f32le",
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
