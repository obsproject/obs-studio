/******************************************************************************
    Copyright (C) 2025 by Dennis Sädtler <saedtler@twitch.tv>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include <memory>
#include <optional>
#include <vector>
#include <map>

#include "obs.h"
#include "obs.hpp"

namespace OBS {
class Canvas {

public:
	Canvas(obs_canvas_t *canvas);
	Canvas(Canvas &&other) noexcept;

	~Canvas() noexcept;

	// No default or copy/move constructors
	Canvas() = delete;
	Canvas(Canvas &other) = delete;

	Canvas &operator=(Canvas &&other) = delete;

	operator obs_canvas_t *() const { return canvas; }

	[[nodiscard]] std::optional<OBSDataAutoRelease> Save() const;
	static std::shared_ptr<Canvas> Load(obs_data_t *data);
	static std::map<obs_canvas_t *, std::shared_ptr<Canvas>> LoadCanvases(obs_data_array_t *canvases);
	static OBSDataArrayAutoRelease SaveCanvases(const std::map<obs_canvas_t *, std::shared_ptr<Canvas>> &canvases);

public:
	obs_canvas_t *canvas = nullptr;
};
} // namespace OBS
