// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "generated/services-json.hpp"

static inline std::string
ServerProtocolToStdString(const OBSServices::ServerProtocol &protocol)
{
	nlohmann::json j;
	OBSServices::to_json(j, protocol);
	return j.get<std::string>();
}

static inline OBSServices::ServerProtocol
StdStringToServerProtocol(const std::string &protocol)
{
	nlohmann::json j = protocol;
	OBSServices::ServerProtocol prtcl;
	OBSServices::from_json(j, prtcl);
	return prtcl;
}

static inline std::string SupportedVideoCodecToStdString(
	const OBSServices::ProtocolSupportedVideoCodec &codec)
{
	nlohmann::json j;
	OBSServices::to_json(j, codec);
	return j.get<std::string>();
}

static inline std::string SupportedAudioCodecToStdString(
	const OBSServices::ProtocolSupportedAudioCodec &codec)
{
	nlohmann::json j;
	OBSServices::to_json(j, codec);
	return j.get<std::string>();
}
