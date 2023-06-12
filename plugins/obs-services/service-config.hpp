// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <obs-module.h>

#include "service-instance.hpp"

class ServiceConfig {
	const ServiceInstance *service;

	std::string protocol;

	BPtr<char *> supportedVideoCodecs;
	BPtr<char *> supportedAudioCodecs;

	std::string serverUrl;

	std::string streamId;

	std::string username;
	std::string password;

	std::string encryptPassphrase;

	std::string bearerToken;

public:
	ServiceConfig(obs_data_t *settings, obs_service_t *self);
	inline ~ServiceConfig(){};

	void Update(obs_data_t *settings);

	const char *Protocol() { return protocol.c_str(); };

	const char **SupportedVideoCodecs()
	{
		return (const char **)supportedVideoCodecs.Get();
	}
	const char **SupportedAudioCodecs()
	{
		return (const char **)supportedAudioCodecs.Get();
	}

	const char *ConnectInfo(uint32_t type);

	bool CanTryToConnect();
};
