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
#include <nlohmann/json.hpp>

struct JsonBranch {
	/* Internal name / ID of the branch (used in updater) */
	std::string name;
	/* Human readable name */
	std::string display_name;
	/* Description */
	std::string description;
	/* Whether updating should use the branch if selected or fall back to stable */
	bool enabled = false;
	/* Whether the branch should be displayed in the UI */
	bool visible = false;
	/* OS compatibility */
	bool windows = false;
	bool macos = false;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(JsonBranch, name, display_name, description, enabled, visible,
						    windows, macos)
};

using JsonBranches = std::vector<JsonBranch>;
