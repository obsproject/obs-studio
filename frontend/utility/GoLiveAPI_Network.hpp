#pragma once

#include "models/multitrack-video.hpp"

#include <obs.hpp>

#include <QString>

/** Returns either GO_LIVE_API_PRODUCTION_URL or a command line override. */
QString MultitrackVideoAutoConfigURL(obs_service_t *service);

class QWidget;

GoLiveApi::Config DownloadGoLiveConfig(QWidget *parent, QString url, const GoLiveApi::PostData &post_data,
				       const QString &multitrack_video_name);
