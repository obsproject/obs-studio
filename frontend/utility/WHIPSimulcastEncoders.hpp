/******************************************************************************
    Copyright (C) 2025 by Sean DuBois <sean@pion.ly>

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

struct WHIPSimulcastEncoders {
public:
	void Create(const char *encoderId, int rescaleFilter, int whipSimulcastTotalLayers, uint32_t outputWidth,
		    uint32_t outputHeight)
	{
		if (rescaleFilter == OBS_SCALE_DISABLE) {
			rescaleFilter = OBS_SCALE_BICUBIC;
		}

		if (whipSimulcastTotalLayers <= 1) {
			return;
		}

		auto widthStep = outputWidth / whipSimulcastTotalLayers;
		auto heightStep = outputHeight / whipSimulcastTotalLayers;
		std::string encoder_name = "whip_simulcast_0";

		for (auto i = whipSimulcastTotalLayers - 1; i > 0; i--) {
			uint32_t width = widthStep * i;
			width -= width % 2;

			uint32_t height = heightStep * i;
			height -= height % 2;

			encoder_name[encoder_name.size() - 1] = std::to_string(i).at(0);
			auto whip_simulcast_encoder =
				obs_video_encoder_create(encoderId, encoder_name.c_str(), nullptr, nullptr);

			if (whip_simulcast_encoder) {
				obs_encoder_set_video(whip_simulcast_encoder, obs_get_video());
				obs_encoder_set_scaled_size(whip_simulcast_encoder, width, height);
				obs_encoder_set_gpu_scale_type(whip_simulcast_encoder, (obs_scale_type)rescaleFilter);
				whipSimulcastEncoders.push_back(whip_simulcast_encoder);
				obs_encoder_release(whip_simulcast_encoder);
			} else {
				blog(LOG_WARNING,
				     "Failed to create video streaming WHIP Simulcast encoders (BasicOutputHandler)");
			}
		}
	}

	void Update(obs_data_t *videoSettings, int videoBitrate)
	{
		auto bitrateStep = videoBitrate / static_cast<int>(whipSimulcastEncoders.size() + 1);
		for (auto &whipSimulcastEncoder : whipSimulcastEncoders) {
			videoBitrate -= bitrateStep;
			obs_data_set_int(videoSettings, "bitrate", videoBitrate);
			obs_encoder_update(whipSimulcastEncoder, videoSettings);
		}
	}

	void SetVideoFormat(enum video_format format)
	{
		for (auto enc : whipSimulcastEncoders)
			obs_encoder_set_preferred_video_format(enc, format);
	}

	void SetStreamOutput(obs_output_t *streamOutput)
	{
		for (size_t i = 0; i < whipSimulcastEncoders.size(); i++)
			obs_output_set_video_encoder2(streamOutput, whipSimulcastEncoders[i], i + 1);
	}

private:
	std::vector<OBSEncoder> whipSimulcastEncoders;
};
