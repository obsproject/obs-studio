// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <oauth.hpp>

namespace RestreamApi {
using nlohmann::json;

/* NOTE: Geographic, "url" and "id" values are stripped */
struct Ingest {
	std::string name;
	std::string rtmpUrl;
};

inline void from_json(const json &j, Ingest &s)
{
	s.name = j.at("name").get<std::string>();
	s.rtmpUrl = j.at("rtmpUrl").get<std::string>();
}

}
