#pragma once

#include "models/multitrack-video.hpp"

#include <obs.hpp>

#include <QString>

#include <optional>

GoLiveApi::PostData constructGoLivePost(QString streamKey, const std::optional<uint64_t> &maximum_aggregate_bitrate,
					const std::optional<uint32_t> &maximum_video_tracks, bool vod_track_enabled);
