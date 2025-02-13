/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "OBSSourceWidgetView.hpp"

#include <utility/display-helpers.hpp>
#include <widgets/OBSSourceWidget.hpp>

OBSSourceWidgetView::OBSSourceWidgetView(OBSSourceWidget *widget, obs_source_t *source)
	: OBSQTDisplay(widget, Qt::Widget)
{
	setSource(source);
	show();
}

OBSSourceWidgetView::~OBSSourceWidgetView()
{
	obs_display_remove_draw_callback(GetDisplay(), obsRender, this);

	OBSSource source = getSource();
	if (source) {
		obs_source_dec_showing(source);
	}
}

void OBSSourceWidgetView::setSourceWidth(int width)
{
	if (sourceWidth() == width) {
		return;
	}

	sourceWidth_ = width;
	emit viewReady();
}

void OBSSourceWidgetView::setSourceHeight(int height)
{
	if (sourceHeight() == height) {
		return;
	}

	sourceHeight_ = height;
	emit viewReady();
}

void OBSSourceWidgetView::setForceLinearSRGB(bool enable)
{
	forceLinearSRGB = enable;
}

OBSSource OBSSourceWidgetView::getSource()
{
	return OBSGetStrongRef(weakSource);
}

void OBSSourceWidgetView::obsRender(void *data, uint32_t cx, uint32_t cy)
{
	OBSSourceWidgetView *view = reinterpret_cast<OBSSourceWidgetView *>(data);

	OBSSource source = view->getSource();
	if (!source) {
		return;
	}

	uint32_t sourceCX = std::max(obs_source_get_width(source), 1u);
	uint32_t sourceCY = std::max(obs_source_get_height(source), 1u);

	int x, y;
	int newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();

	bool previous = false;
	if (view->forceLinearSRGB) {
		previous = gs_set_linear_srgb(true);
	}

	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);
	obs_source_video_render(source);

	if (view->forceLinearSRGB) {
		gs_set_linear_srgb(previous);
	}
	gs_projection_pop();
	gs_viewport_pop();

	view->setSourceWidth(sourceCX);
	view->setSourceHeight(sourceCY);
}

void OBSSourceWidgetView::setSource(obs_source_t *source)
{
	if (weakSource) {
		obs_source_t *prevSource = OBSGetStrongRef(weakSource);
		if (prevSource) {
			obs_source_dec_showing(prevSource);
		}
	}

	weakSource = OBSGetWeakRef(source);
	obs_source_inc_showing(source);

	enum obs_source_type type = obs_source_get_type(source);
	bool isDrawableType = type == OBS_SOURCE_TYPE_INPUT || type == OBS_SOURCE_TYPE_SCENE ||
			      type == OBS_SOURCE_TYPE_TRANSITION;

	auto addDrawCallback = [this]() {
		obs_display_add_draw_callback(GetDisplay(), obsRender, this);
	};

	uint32_t caps = obs_source_get_output_flags(source);
	if ((caps & OBS_SOURCE_VIDEO) != 0) {
		if (isDrawableType) {
			connect(this, &OBSQTDisplay::DisplayCreated, this, addDrawCallback);
		}
	}
}
