// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "restream-service.hpp"

#include "restream-config.hpp"

static const char *SUPPORTED_VIDEO_CODECS[] = {"h264", NULL};

void RestreamService::InfoFreeTypeData(void *typeData)
{
	if (typeData)
		delete reinterpret_cast<RestreamService *>(typeData);
}

const char *RestreamService::InfoGetName(void *)
{
	return "Restream.io";
}

void *RestreamService::InfoCreate(obs_data_t *settings, obs_service_t *service)
{
	return reinterpret_cast<void *>(new RestreamConfig(settings, service));
	;
}

void RestreamService::InfoDestroy(void *data)
{
	if (data)
		delete reinterpret_cast<RestreamConfig *>(data);
}

void RestreamService::InfoUpdate(void *data, obs_data_t *settings)
{
	RestreamConfig *priv = reinterpret_cast<RestreamConfig *>(data);
	if (priv)
		priv->Update(settings);
}

const char *RestreamService::InfoGetConnectInfo(void *data, uint32_t type)
{
	RestreamConfig *priv = reinterpret_cast<RestreamConfig *>(data);
	if (priv)
		return priv->ConnectInfo(type);
	return nullptr;
}

const char *RestreamService::InfoGetProtocol(void *data)
{
	RestreamConfig *priv = reinterpret_cast<RestreamConfig *>(data);
	if (priv)
		return priv->Protocol();
	return nullptr;
}

const char **RestreamService::InfoGetSupportedVideoCodecs(void *)
{
	return SUPPORTED_VIDEO_CODECS;
}

bool RestreamService::InfoCanTryToConnect(void *data)
{
	RestreamConfig *priv = reinterpret_cast<RestreamConfig *>(data);
	if (priv)
		return priv->CanTryToConnect();
	return false;
}

obs_properties_t *RestreamService::InfoGetProperties(void *data)
{
	if (data)
		return reinterpret_cast<RestreamConfig *>(data)->GetProperties();
	return nullptr;
}

bool RestreamService::InfoCanBandwidthTest(void *data)
{
	if (data)
		return reinterpret_cast<RestreamConfig *>(data)
			->CanBandwidthTest();
	return false;
}

void RestreamService::InfoEnableBandwidthTest(void *data, bool enabled)
{
	if (data)
		return reinterpret_cast<RestreamConfig *>(data)
			->EnableBandwidthTest(enabled);
}

bool RestreamService::InfoBandwidthTestEnabled(void *data)
{
	if (data)
		return reinterpret_cast<RestreamConfig *>(data)
			->BandwidthTestEnabled();
	return false;
}
