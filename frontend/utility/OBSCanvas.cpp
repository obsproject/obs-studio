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

#include "OBSCanvas.hpp"

#include <utility>

namespace OBS {

Canvas::Canvas(obs_canvas_t *canvas) : canvas(canvas) {}

Canvas::Canvas(Canvas &&other) noexcept
{
	canvas = std::exchange(other.canvas, nullptr);
}

Canvas::~Canvas() noexcept
{
	if (!canvas)
		return;

	obs_canvas_remove(canvas);
	obs_canvas_release(canvas);
	canvas = nullptr;
}

Canvas &Canvas::operator=(Canvas &&other) noexcept
{
	canvas = std::exchange(other.canvas, canvas);

	return *this;
}

std::optional<OBSDataAutoRelease> Canvas::Save() const
{
	if (!canvas)
		return std::nullopt;
	if (obs_data_t *saved = obs_save_canvas(canvas))
		return saved;

	return std::nullopt;
}

std::unique_ptr<Canvas> Canvas::Load(obs_data_t *data)
{
	if (OBSDataAutoRelease canvas_data = obs_data_get_obj(data, "info")) {
		if (obs_canvas_t *canvas = obs_load_canvas(canvas_data)) {
			return std::make_unique<Canvas>(canvas);
		}
	}

	return nullptr;
}

std::vector<Canvas> Canvas::LoadCanvases(obs_data_array_t *canvases)
{
	auto cb = [](obs_data_t *data, void *param) -> void {
		auto vec = static_cast<std::vector<Canvas> *>(param);
		if (auto canvas = Canvas::Load(data))
			vec->emplace_back(std::move(*canvas));
	};

	std::vector<Canvas> ret;
	obs_data_array_enum(canvases, cb, &ret);

	return ret;
}

OBSDataArrayAutoRelease Canvas::SaveCanvases(const std::vector<Canvas> &canvases)
{
	OBSDataArrayAutoRelease savedCanvases = obs_data_array_create();

	for (auto &canvas : canvases) {
		auto canvas_data = canvas.Save();
		if (!canvas_data)
			continue;

		OBSDataAutoRelease data = obs_data_create();
		obs_data_set_obj(data, "info", *canvas_data);
		obs_data_array_push_back(savedCanvases, data);
	}

	return savedCanvases;
}

} // namespace OBS
