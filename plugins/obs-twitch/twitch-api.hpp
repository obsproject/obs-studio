// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <oauth.hpp>

namespace TwitchApi {
using nlohmann::json;

/* NOTE: "Reserved for internal use" and "_id" values are stripped */
struct Ingest {
	std::string name;
	std::string url_template;
	std::string url_template_secure;
};

struct IngestResponse {
	std::vector<Ingest> ingests;
};

inline void from_json(const json &j, Ingest &s)
{
	s.name = j.at("name").get<std::string>();
	s.url_template = j.at("url_template").get<std::string>();
	s.url_template_secure = j.at("url_template_secure").get<std::string>();
}

inline void from_json(const json &j, IngestResponse &s)
{
	s.ingests = j.at("ingests").get<std::vector<Ingest>>();
}

}
