// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "youtube-service.hpp"

#include "youtube-config.hpp"

static const char *SUPPORTED_VIDEO_CODECS[] = {"h264", "hevc", "av1", NULL};

void YouTubeService::InfoFreeTypeData(void *typeData)
{
	if (typeData)
		delete reinterpret_cast<YouTubeService *>(typeData);
}

const char *YouTubeService::InfoGetName(void *)
{
	return "YouTube";
}

void *YouTubeService::InfoCreate(obs_data_t *settings, obs_service_t *service)
{
	return reinterpret_cast<void *>(new YouTubeConfig(settings, service));
	;
}

void YouTubeService::InfoDestroy(void *data)
{
	if (data)
		delete reinterpret_cast<YouTubeConfig *>(data);
}

void YouTubeService::InfoUpdate(void *data, obs_data_t *settings)
{
	YouTubeConfig *priv = reinterpret_cast<YouTubeConfig *>(data);
	if (priv)
		priv->Update(settings);
}

const char *YouTubeService::InfoGetConnectInfo(void *data, uint32_t type)
{
	YouTubeConfig *priv = reinterpret_cast<YouTubeConfig *>(data);
	if (priv)
		return priv->ConnectInfo(type);
	return nullptr;
}

const char *YouTubeService::InfoGetProtocol(void *data)
{
	YouTubeConfig *priv = reinterpret_cast<YouTubeConfig *>(data);
	if (priv)
		return priv->Protocol();
	return nullptr;
}

const char **YouTubeService::InfoGetSupportedVideoCodecs(void *)
{
	return SUPPORTED_VIDEO_CODECS;
}

bool YouTubeService::InfoCanTryToConnect(void *data)
{
	YouTubeConfig *priv = reinterpret_cast<YouTubeConfig *>(data);
	if (priv)
		return priv->CanTryToConnect();
	return false;
}

obs_properties_t *YouTubeService::InfoGetProperties(void *data)
{
	if (data)
		return reinterpret_cast<YouTubeConfig *>(data)->GetProperties();
	return nullptr;
}
