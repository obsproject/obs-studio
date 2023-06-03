// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "youtube-service.hpp"

#include "youtube-config.hpp"

YouTubeService::YouTubeService()
{
	info.type_data = this;
	info.free_type_data = InfoFreeTypeData;

	info.id = "youtube";
	info.supported_protocols = "RTMPS;RTMP;HLS";

	info.get_name = InfoGetName;
	info.create = InfoCreate;
	info.destroy = InfoDestroy;
	info.update = InfoUpdate;

	info.get_connect_info = InfoGetConnectInfo;

	info.get_protocol = InfoGetProtocol;

	info.can_try_to_connect = InfoCanTryToConnect;

	info.flags = 0;

	info.get_defaults = YouTubeConfig::InfoGetDefault;
	info.get_properties = InfoGetProperties;

	obs_register_service(&info);
}

YouTubeService::~YouTubeService() {}

void YouTubeService::Register()
{
	new YouTubeService();
}
