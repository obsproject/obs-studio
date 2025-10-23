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

const OBS::Canvas &OBSBasic::AddCanvas(const std::string &name, obs_video_info *ovi, int flags)
{
	OBSCanvas canvas = obs_canvas_create(name.c_str(), ovi, flags);
	auto &it = canvases.emplace_back(canvas);
	OnEvent(OBS_FRONTEND_EVENT_CANVAS_ADDED);
	return it;
}

bool OBSBasic::RemoveCanvas(OBSCanvas canvas)
{
	bool removed = false;
	if (!canvas)
		return removed;

	auto canvas_it = std::find(std::begin(canvases), std::end(canvases), canvas);
	if (canvas_it != std::end(canvases)) {
		// Move canvas to a temporary object to delay removal of the canvas and calls to its signal handlers
		// until after erase() completes. This is to avoid issues with recursion coming from the
		// CanvasRemoved() signal handler.
		OBS::Canvas tmp = std::move(*canvas_it);
		canvases.erase(canvas_it);
		removed = true;
	}

	if (removed)
		OnEvent(OBS_FRONTEND_EVENT_CANVAS_REMOVED);

	return removed;
}

void OBSBasic::ClearCanvases()
{
	// Delete canvases one-by-one to ensure OBS_FRONTEND_EVENT_CANVAS_REMOVED is sent for each
	while (!canvases.empty()) {
		RemoveCanvas(OBSCanvas(canvases.back()));
	}
}
