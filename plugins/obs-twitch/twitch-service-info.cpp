// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "twitch-service.hpp"

#include "twitch-config.hpp"

static const char *SUPPORTED_VIDEO_CODECS[] = {"h264", NULL};

void TwitchService::InfoFreeTypeData(void *typeData)
{
	if (typeData)
		delete reinterpret_cast<TwitchService *>(typeData);
}

const char *TwitchService::InfoGetName(void *)
{
	return "Twitch";
}

void *TwitchService::InfoCreate(obs_data_t *settings, obs_service_t *service)
{
	return reinterpret_cast<void *>(new TwitchConfig(settings, service));
	;
}

void TwitchService::InfoDestroy(void *data)
{
	if (data)
		delete reinterpret_cast<TwitchConfig *>(data);
}

void TwitchService::InfoUpdate(void *data, obs_data_t *settings)
{
	TwitchConfig *priv = reinterpret_cast<TwitchConfig *>(data);
	if (priv)
		priv->Update(settings);
}

const char *TwitchService::InfoGetConnectInfo(void *data, uint32_t type)
{
	TwitchConfig *priv = reinterpret_cast<TwitchConfig *>(data);
	if (priv)
		return priv->ConnectInfo(type);
	return nullptr;
}

const char *TwitchService::InfoGetProtocol(void *data)
{
	TwitchConfig *priv = reinterpret_cast<TwitchConfig *>(data);
	if (priv)
		return priv->Protocol();
	return nullptr;
}

const char **TwitchService::InfoGetSupportedVideoCodecs(void *)
{
	return SUPPORTED_VIDEO_CODECS;
}

bool TwitchService::InfoCanTryToConnect(void *data)
{
	TwitchConfig *priv = reinterpret_cast<TwitchConfig *>(data);
	if (priv)
		return priv->CanTryToConnect();
	return false;
}

obs_properties_t *TwitchService::InfoGetProperties(void *data)
{
	if (data)
		return reinterpret_cast<TwitchConfig *>(data)->GetProperties();
	return nullptr;
}

bool TwitchService::InfoCanBandwidthTest(void *data)
{
	if (data)
		return reinterpret_cast<TwitchConfig *>(data)
			->CanBandwidthTest();
	return false;
}

void TwitchService::InfoEnableBandwidthTest(void *data, bool enabled)
{
	if (data)
		return reinterpret_cast<TwitchConfig *>(data)
			->EnableBandwidthTest(enabled);
}

bool TwitchService::InfoBandwidthTestEnabled(void *data)
{
	if (data)
		return reinterpret_cast<TwitchConfig *>(data)
			->BandwidthTestEnabled();
	return false;
}
