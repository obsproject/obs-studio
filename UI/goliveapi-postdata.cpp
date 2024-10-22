#include "goliveapi-postdata.hpp"

#include <nlohmann/json.hpp>

#include "system-info.hpp"

#include "models/multitrack-video.hpp"

GoLiveApi::PostData constructGoLivePost(QString streamKey, const std::optional<uint64_t> &maximum_aggregate_bitrate,
					const std::optional<uint32_t> &maximum_video_tracks, bool vod_track_enabled,
					const std::map<std::string, video_t *> &extra_views)
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

	if (!extra_views.empty()) {
		auto &extra_views_capability = *post_data.capabilities.extra_views;
		extra_views_capability.reserve(extra_views.size());
		for (auto &view : extra_views) {
			video_t *video = view.second;
			if (!video)
				continue;
			const struct video_output_info *voi = video_output_get_info(video);
			if (!voi)
				continue;
			extra_views_capability.push_back(
				GoLiveApi::ExtraView{view.first, voi->width, voi->height,
						     media_frames_per_second{voi->fps_num, voi->fps_den}});
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
