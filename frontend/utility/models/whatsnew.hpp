/*
 * Copyright (c) 2023 Dennis Sädtler <dennis@obsproject.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#pragma once

#include <string>
#include <optional>

#include <nlohmann/json.hpp>

/*
 * Support for (de-)serialising std::optional
 * Adapted from https://github.com/nlohmann/json/issues/1749#issuecomment-1555093802
 */
template<typename T> struct nlohmann::adl_serializer<std::optional<T>> {
	static std::optional<T> from_json(const json &json)
	{
		return json.is_null() ? std::nullopt : std::optional{json.get<T>()};
	}

	static void to_json(json &json, std::optional<T> t)
	{
		if (t)
			json = *t;
		else
			json = nullptr;
	}
};

struct WhatsNewPlatforms {
	bool windows = false;
	bool macos = false;
	bool linux = false;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(WhatsNewPlatforms, windows, macos, linux)
};

struct WhatsNewItem {
	/* Target OBS version (patch is ignored) */
	std::string version;
	/* Beta/RC release to target */
	int Beta = 0;
	int RC = 0;
	/* URL of webpage to be displayed */
	std::string url;
	/* Increment for this version's item */
	int increment = 0;
	/* Optional OS filter */
	std::optional<WhatsNewPlatforms> os = std::nullopt;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(WhatsNewItem, version, Beta, RC, url, increment, os)
};

using WhatsNewList = std::vector<WhatsNewItem>;
