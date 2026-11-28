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

void OBSBasic::setActiveMediator(std::shared_ptr<OBS::mediator::Canvas> mediator)
{
	if (activeCanvasMediator.lock() == mediator) {
		return;
	}

	// Disconnect old signals
	for (auto &connection : activeCanvasConnections) {
		disconnect(connection);
	}

	activeCanvasConnections.clear();

	activeCanvasMediator = mediator;
	if (!mediator) {
		return;
	}

	// Bind frontend events to mediator signals
	activeCanvasConnections.push_back(connect(mediator.get(), &OBS::mediator::Canvas::sceneOrderChanged, this,
						  [this]() { OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED); }));
	activeCanvasConnections.push_back(connect(mediator.get(), &OBS::mediator::Canvas::sceneRenamed, this,
						  [this]() { OnEvent(OBS_FRONTEND_EVENT_SCENE_LIST_CHANGED); }));

	// Connect canvas signals
	activeCanvasConnections.push_back(
		connect(mediator.get(), &OBS::mediator::Canvas::previewChanged, this, [this](OBSWeakSource weak) {
			OBSSource source = OBSGetStrongRef(weak);
			OBSScene scene = obs_scene_from_source(source);
			if (!scene) {
				return;
			}

			ui->sources->RefreshItems();
			SetCurrentScene(scene);
		}));

	emit activeCanvasChanged(mediator);
	OnEvent(OBS_FRONTEND_EVENT_CANVAS_ACTIVE_CHANGED);
}

void OBSBasic::canvasRemoved(void *data, calldata_t *params)
{
	OBSCanvas canvas = static_cast<obs_canvas_t *>(calldata_ptr(params, "canvas"));
	QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), &OBSBasic::removeCanvas, canvas);
}

const OBS::Canvas &OBSBasic::addCanvas(const std::string &name, obs_video_info *ovi, int flags)
{
	OBSCanvas canvas = obs_canvas_create(name.c_str(), ovi, flags);
	auto &it = canvases.emplace_back(canvas);
	OnEvent(OBS_FRONTEND_EVENT_CANVAS_ADDED);
	return it;
}

bool OBSBasic::removeCanvas(OBSCanvas canvas)
{
	bool removed = false;
	if (!canvas) {
		return removed;
	}

	auto canvas_it = std::find(std::begin(canvases), std::end(canvases), canvas);
	if (canvas_it != std::end(canvases)) {
		canvases.erase(canvas_it);
		removed = true;
	}

	if (removed) {
		OnEvent(OBS_FRONTEND_EVENT_CANVAS_REMOVED);
	}

	return removed;
}

void OBSBasic::clearCanvases()
{
	// Delete canvases one-by-one to ensure OBS_FRONTEND_EVENT_CANVAS_REMOVED is sent for each
	while (!canvases.empty()) {
		removeCanvas(OBSCanvas(canvases.back()));
	}
}
