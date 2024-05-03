#pragma once

#include <obs.hpp>
#include <optional>
#include <map>
#include <QString>
#include "models/multitrack-video.hpp"

GoLiveApi::PostData constructGoLivePost(QString streamKey, const std::optional<uint64_t> &maximum_aggregate_bitrate,
					const std::optional<uint32_t> &maximum_video_tracks, bool vod_track_enabled,
					const std::map<std::string, video_t *> &extra_views);
