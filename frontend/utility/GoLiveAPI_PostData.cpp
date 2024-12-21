#include "GoLiveAPI_PostData.hpp"
#include "models/multitrack-video.hpp"

#include <utility/system-info.hpp>

#include <nlohmann/json.hpp>

GoLiveApi::PostData constructGoLivePost(QString streamKey, const std::optional<uint64_t> &maximum_aggregate_bitrate,
					const std::optional<uint32_t> &maximum_video_tracks, bool vod_track_enabled)
{
	GoLiveApi::PostData post_data{};
	post_data.service = "IVS";
	post_data.schema_version = "2024-06-04";
	post_data.authentication = streamKey.toStdString();

	system_info(post_data.capabilities);

	auto &client = post_data.client;

	client.name = "obs-studio";
	client.version = obs_get_version_string();

	auto add_codec = [&](const char *codec) {
		auto it = std::find(std::begin(client.supported_codecs), std::end(client.supported_codecs), codec);
		if (it != std::end(client.supported_codecs))
			return;

		client.supported_codecs.push_back(codec);
	};

	const char *encoder_id = nullptr;
	for (size_t i = 0; obs_enum_encoder_types(i, &encoder_id); i++) {
		auto codec = obs_get_encoder_codec(encoder_id);
		if (!codec)
			continue;

		if (qstricmp(codec, "h264") == 0) {
			add_codec("h264");
#ifdef ENABLE_HEVC
		} else if (qstricmp(codec, "hevc")) {
			add_codec("h265");
#endif
		} else if (qstricmp(codec, "av1")) {
			add_codec("av1");
		}
	}

	auto &preferences = post_data.preferences;
	preferences.vod_track_audio = vod_track_enabled;

	obs_video_info ovi;
	if (obs_get_video_info(&ovi)) {
		preferences.width = ovi.output_width;
		preferences.height = ovi.output_height;
		preferences.framerate.numerator = ovi.fps_num;
		preferences.framerate.denominator = ovi.fps_den;

		preferences.canvas_width = ovi.base_width;
		preferences.canvas_height = ovi.base_height;

		preferences.composition_gpu_index = ovi.adapter;
	}

	if (maximum_aggregate_bitrate.has_value())
		preferences.maximum_aggregate_bitrate = maximum_aggregate_bitrate.value();
	if (maximum_video_tracks.has_value())
		preferences.maximum_video_tracks = maximum_video_tracks.value();

	return post_data;
}
