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

Canvas::~Canvas() noexcept
{
	if (!canvas)
		return;

	obs_canvas_remove(canvas);
	obs_canvas_release(canvas);
	canvas = nullptr;
}

std::optional<OBSDataAutoRelease> Canvas::Save() const
{
	if (!canvas)
		return std::nullopt;
	if (obs_data_t *saved = obs_save_canvas(canvas))
		return saved;

	return std::nullopt;
}

std::shared_ptr<Canvas> Canvas::Load(obs_data_t *data)
{
	if (OBSDataAutoRelease canvas_data = obs_data_get_obj(data, "info")) {
		if (obs_canvas_t *canvas = obs_load_canvas(canvas_data)) {
			return std::make_shared<Canvas>(canvas);
		}
	}

	return nullptr;
}

std::map<obs_canvas_t *, std::shared_ptr<Canvas>> Canvas::LoadCanvases(obs_data_array_t *canvases)
{
	auto cb = [](obs_data_t *data, void *param) -> void {
		auto vec = static_cast<std::map<obs_canvas_t *, std::shared_ptr<Canvas>> *>(param);
		if (auto canvas = Canvas::Load(data))
			(*vec)[canvas->canvas] = canvas;
	};

	std::map<obs_canvas_t *, std::shared_ptr<Canvas>> ret;
	obs_data_array_enum(canvases, cb, &ret);

	return ret;
}

OBSDataArrayAutoRelease Canvas::SaveCanvases(const std::map<obs_canvas_t *, std::shared_ptr<Canvas>> &canvases)
{
	OBSDataArrayAutoRelease savedCanvases = obs_data_array_create();

	for (auto kv : canvases) {
		auto canvas_data = kv.second->Save();
		if (!canvas_data)
			continue;

		OBSDataAutoRelease data = obs_data_create();
		obs_data_set_obj(data, "info", *canvas_data);
		obs_data_array_push_back(savedCanvases, data);
	}

	return savedCanvases;
}

} // namespace OBS
