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

#include "OBSBasic.hpp"

void OBSBasic::CanvasRemoved(void *data, calldata_t *params)
{
	obs_canvas_t *canvas = static_cast<obs_canvas_t *>(calldata_ptr(params, "canvas"));
	QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "RemoveCanvas", Q_ARG(OBSCanvas, OBSCanvas(canvas)));
}

OBSCanvas OBSBasic::AddCanvas(const std::string &name, obs_video_info *ovi, int flags)
{
	if (name.empty())
		return nullptr;

	OBSCanvas canvas = obs_canvas_create(name.c_str(), ovi, flags);
	canvases.emplace_back(OBSFrontendCanvas{canvas.Get()});
	OnEvent(OBS_FRONTEND_EVENT_CANVAS_ADDED);
	return canvas;
}

bool OBSBasic::RemoveCanvas(OBSCanvas canvas)
{
	if (!canvas)
		return false;

	auto canvas_it = std::find_if(std::begin(canvases), std::end(canvases),
				      [&](const OBSFrontendCanvas &fc) { return fc.canvas.Get() == canvas.Get(); });
	bool found = canvas_it != std::end(canvases);
	if (found) {
		canvases.erase(canvas_it);
		OnEvent(OBS_FRONTEND_EVENT_CANVAS_REMOVED);
	}

	return found;
}

obs_data_array_t *OBSBasic::SaveCanvases() const
{
	obs_data_array_t *savedCanvases = obs_data_array_create();

	for (auto &canvas : canvases) {
		OBSDataAutoRelease data = obs_data_create();
		OBSDataAutoRelease canvas_data = obs_save_canvas(canvas.canvas);
		if (!canvas_data)
			continue;

		obs_data_set_obj(data, "info", canvas_data);
		obs_data_array_push_back(savedCanvases, data);
	}

	return savedCanvases;
}

void OBSBasic::LoadSavedCanvases(obs_data_array_t *canvases)
{
	auto cb = [](obs_data_t *data, void *param) -> void {
		auto this_ = static_cast<OBSBasic *>(param);

		OBSDataAutoRelease canvas_data = obs_data_get_obj(data, "info");
		if (canvas_data) {
			obs_canvas_t *canvas = obs_load_canvas(canvas_data);
			this_->canvases.emplace_back(OBSFrontendCanvas{canvas});
		}
	};

	obs_data_array_enum(canvases, cb, this);
}

void OBSBasic::ClearCanvases()
{
	if (canvases.empty())
		return;

	std::vector<OBSCanvas> cv(canvases.size());
	std::transform(canvases.begin(), canvases.end(), cv.begin(), [](const auto &fc) { return fc.canvas.Get(); });

	for (auto &canvas : cv) {
		obs_canvas_remove(canvas);
	}
}
