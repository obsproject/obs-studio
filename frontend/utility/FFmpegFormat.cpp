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

#include "FFmpegFormat.hpp"
#include "FFmpegCodec.hpp"

using namespace std;

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
	const AVCodecID codec_id = codec_type == VIDEO ? video_codec : audio_codec;
	if (codec_type == UNKNOWN || codec_id == AV_CODEC_ID_NONE)
		return {};

	if (auto codec = avcodec_find_encoder(codec_id))
		return {codec};

	/* Fall back to using the format name as the encoder,
	 * this works for some formats such as FLV. */
	return FFmpegCodec{name, codec_id, codec_type};
}
