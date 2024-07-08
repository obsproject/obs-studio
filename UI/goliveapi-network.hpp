#pragma once

#include <obs.hpp>
#include <QString>

#include "models/multitrack-video.hpp"

/** Returns either GO_LIVE_API_PRODUCTION_URL or a command line override. */
QString MultitrackVideoAutoConfigURL(obs_service_t *service);

class QWidget;

GoLiveApi::Config DownloadGoLiveConfig(QWidget *parent, QString url,
				       const GoLiveApi::PostData &post_data,
				       const QString &multitrack_video_name);
