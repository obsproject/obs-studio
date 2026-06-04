#include "GoLiveAPI_PostData.hpp"
#include "models/multitrack-video.hpp"

#include <utility/system-info.hpp>

#include <nlohmann/json.hpp>

GoLiveApi::PostData constructGoLivePost(QString streamKey, const std::optional<uint64_t> &maximum_aggregate_bitrate,
					const std::optional<uint32_t> &maximum_video_tracks, bool vod_track_enabled,
					const std::vector<OBSCanvasAutoRelease> &canvases)
{
	GoLiveApi::PostData post_data{};
	post_data.service = "IVS";
	post_data.schema_version = "2025-01-25";
	post_data.authentication = streamKey.toStdString();

	system_info(post_data.capabilities);

	auto &client = post_data.client;

	client.name = "obs-studio";
	client.version = obs_get_version_string();

	const char *encoder_id = nullptr;
	for (size_t i = 0; obs_enum_encoder_types(i, &encoder_id); i++) {
		auto codec = obs_get_encoder_codec(encoder_id);
		if (!codec)
			continue;

		if (qstricmp(codec, "h264") == 0) {
			client.supported_codecs.emplace("h264");
#ifdef ENABLE_HEVC
		} else if (qstricmp(codec, "hevc") == 0) {
			client.supported_codecs.emplace("h265");
#endif
		} else if (qstricmp(codec, "av1") == 0) {
			client.supported_codecs.emplace("av1");
		}
	}

	auto &preferences = post_data.preferences;
	preferences.vod_track_audio = vod_track_enabled;

	obs_video_info ovi;
	if (obs_get_video_info(&ovi))
		preferences.composition_gpu_index = ovi.adapter;

	for (const auto &canvas : canvases) {
		if (obs_canvas_get_video_info(canvas, &ovi)) {
			preferences.canvases.emplace_back(GoLiveApi::Canvas{ovi.output_width,
									    ovi.output_height,
									    ovi.base_width,
									    ovi.base_height,
									    {ovi.fps_num, ovi.fps_den}});
		}
	}

	obs_audio_info2 oai2;
	if (obs_get_audio_info2(&oai2)) {
		preferences.audio_samples_per_sec = oai2.samples_per_sec;
		preferences.audio_channels = get_audio_channels(oai2.speakers);
		preferences.audio_fixed_buffering = oai2.fixed_buffering;
		preferences.audio_max_buffering_ms = oai2.max_buffering_ms;
	}

	if (maximum_aggregate_bitrate.has_value())
		preferences.maximum_aggregate_bitrate = maximum_aggregate_bitrate.value();

	if (maximum_video_tracks.has_value()) {
		/* Cap to maximum supported number of output encoders. */
		preferences.maximum_video_tracks =
			std::min(maximum_video_tracks.value(), static_cast<uint32_t>(MAX_OUTPUT_VIDEO_ENCODERS));
	}

	return post_data;
}
