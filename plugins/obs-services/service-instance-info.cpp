// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "service-instance.hpp"

#include "service-config.hpp"

const char *ServiceInstance::InfoGetName(void *typeData)
{
	if (typeData)
		return reinterpret_cast<ServiceInstance *>(typeData)->GetName();
	return nullptr;
}

void *ServiceInstance::InfoCreate(obs_data_t *settings, obs_service_t *service)
{
	return reinterpret_cast<void *>(new ServiceConfig(settings, service));
}

void ServiceInstance::InfoDestroy(void *data)
{
	if (data)
		delete reinterpret_cast<ServiceConfig *>(data);
}

void ServiceInstance::InfoUpdate(void *data, obs_data_t *settings)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		priv->Update(settings);
}

const char *ServiceInstance::InfoGetConnectInfo(void *data, uint32_t type)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		return priv->ConnectInfo(type);
	return nullptr;
}

const char *ServiceInstance::InfoGetProtocol(void *data)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		return priv->Protocol();
	return nullptr;
}

const char **ServiceInstance::InfoGetSupportedVideoCodecs(void *data)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		return priv->SupportedVideoCodecs();
	return nullptr;
}

const char **ServiceInstance::InfoGetSupportedAudioCodecs(void *data)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		return priv->SupportedAudioCodecs();
	return nullptr;
}

bool ServiceInstance::InfoCanTryToConnect(void *data)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		return priv->CanTryToConnect();
	return false;
}

int ServiceInstance::InfoGetMaxCodecBitrate(void *data, const char *codec)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		return priv->GetMaxCodecBitrate(codec);
	return 0;
}

void ServiceInstance::InfoGetSupportedResolutions2(
	void *data, struct obs_service_resolution **resolutions, size_t *count,
	bool *withFps)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		priv->GetSupportedResolutions(resolutions, count, withFps);
}

int ServiceInstance::InfoGetMaxVideoBitrate(
	void *data, const char *codec, struct obs_service_resolution resolution)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		return priv->GetMaxVideoBitrate(codec, resolution);
	return 0;
}

void ServiceInstance::InfoGetDefault2(void *typeData, obs_data_t *settings)
{
	if (typeData)
		reinterpret_cast<ServiceInstance *>(typeData)->GetDefaults(
			settings);
}

obs_properties_t *ServiceInstance::InfoGetProperties2(void * /* data */,
						      void *typeData)
{
	if (typeData)
		return reinterpret_cast<ServiceInstance *>(typeData)
			->GetProperties();
	return nullptr;
}

void ServiceInstance::InfoApplySettings(void *data, obs_data_t *videoSettings,
					obs_data_t *audioSettings)
{
	ServiceConfig *priv = reinterpret_cast<ServiceConfig *>(data);
	if (priv)
		priv->ApplySettings(videoSettings, audioSettings);
}
