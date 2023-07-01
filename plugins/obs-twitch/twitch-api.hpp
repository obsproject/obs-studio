// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <oauth.hpp>

namespace TwitchApi {
using nlohmann::json;

#ifdef OAUTH_ENABLED
#ifndef NLOHMANN_OPTIONAL_TwitchApi_HELPER
#define NLOHMANN_OPTIONAL_TwitchApi_HELPER
template<typename T>
inline std::optional<T> get_stack_optional(const json &j, const char *property)
{
	auto it = j.find(property);
	if (it != j.end() && !it->is_null()) {
		return j.at(property).get<std::optional<T>>();
	}
	return std::optional<T>();
}
#endif
#endif

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

#ifdef OAUTH_ENABLED
enum class UserType : int {
	NORMAL,
	STAFF,
	GLOBAL_MOD,
	ADMIN,
};

enum class BroadcasterType : int {
	NORMAL,
	PARTNER,
	AFFILIATE,
};

struct User {
	std::string id;
	std::string login;
	std::string displayName;
	UserType type;
	BroadcasterType broadcasterType;
	std::string description;
	std::string profileImageUrl;
	std::string offlineImageUrl;
	/* Available if "user:read:email" scope */
	std::optional<std::string> email;
	std::string createdAt;
};

struct UsersResponse {
	std::vector<User> data;
};

struct StreamKey {
	std::string streamKey;
};

struct StreamKeyResponse {
	std::vector<StreamKey> data;
};

inline void from_json(const json &j, UserType &e)
{
	if (j == "")
		e = UserType::NORMAL;
	else if (j == "staff")
		e = UserType::STAFF;
	else if (j == "global_mod")
		e = UserType::GLOBAL_MOD;
	else if (j == "admin")
		e = UserType::ADMIN;
	else {
		throw std::runtime_error("Unknown \"type\" value");
	}
}

inline void from_json(const json &j, BroadcasterType &e)
{
	if (j == "")
		e = BroadcasterType::NORMAL;
	else if (j == "partner")
		e = BroadcasterType::PARTNER;
	else if (j == "affiliate")
		e = BroadcasterType::AFFILIATE;
	else {
		throw std::runtime_error("Unknown \"broadcaster_type\" value");
	}
}

inline void from_json(const json &j, User &s)
{
	s.id = j.at("id").get<std::string>();
	s.login = j.at("login").get<std::string>();
	s.displayName = j.at("display_name").get<std::string>();
	s.type = j.at("type").get<UserType>();
	s.broadcasterType = j.at("broadcaster_type").get<BroadcasterType>();
	s.description = j.at("description").get<std::string>();
	s.profileImageUrl = j.at("profile_image_url").get<std::string>();
	s.offlineImageUrl = j.at("offline_image_url").get<std::string>();
	s.email = get_stack_optional<std::string>(j, "email");
	s.createdAt = j.at("created_at").get<std::string>();
}

inline void from_json(const json &j, UsersResponse &s)
{
	s.data = j.at("data").get<std::vector<User>>();
}

inline void from_json(const json &j, StreamKey &s)
{
	s.streamKey = j.at("stream_key").get<std::string>();
}

inline void from_json(const json &j, StreamKeyResponse &s)
{
	s.data = j.at("data").get<std::vector<StreamKey>>();
}
#endif
}
