/******************************************************************************
    Copyright (C) 2014 by Hugh Bailey <obs.jim@gmail.com>

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

#include "obs-app.hpp"
#include "window-basic-properties.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "display-helpers.hpp"

using namespace std;

OBSBasicProperties::OBSBasicProperties(QWidget *parent, OBSSource source_)
	: QDialog       (parent),
	  main          (qobject_cast<OBSBasic*>(parent)),
	  resizeTimer   (0),
	  ui            (new Ui::OBSBasicProperties),
	  source        (source_),
	  removedSignal (obs_source_signalhandler(source), "remove",
	                 OBSBasicProperties::SourceRemoved, this)
{
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	OBSData settings = obs_source_getsettings(source);
	obs_data_release(settings);

	view = new OBSPropertiesView(settings,
			obs_source_properties(source, App()->GetLocale()),
			source, (PropertiesUpdateCallback)obs_source_update);

	layout()->addWidget(view);
	layout()->setAlignment(view, Qt::AlignRight);
	view->show();
}

void OBSBasicProperties::SourceRemoved(void *data, calldata_t params)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicProperties*>(data),
			"close");

	UNUSED_PARAMETER(params);
}

void OBSBasicProperties::DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	OBSBasicProperties *window = static_cast<OBSBasicProperties*>(data);

	if (!window->source)
		return;

	uint32_t sourceCX = obs_source_getwidth(window->source);
	uint32_t sourceCY = obs_source_getheight(window->source);

	int   x, y;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	gs_matrix_push();
	gs_matrix_scale3f(scale, scale, 1.0f);
	gs_matrix_translate3f(-x, -y, 0.0f);
	obs_source_video_render(window->source);
	gs_matrix_pop();
}

void OBSBasicProperties::resizeEvent(QResizeEvent *event)
{
	if (isVisible()) {
		if (resizeTimer)
			killTimer(resizeTimer);
		resizeTimer = startTimer(100);
	}

	UNUSED_PARAMETER(event);
}

void OBSBasicProperties::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == resizeTimer) {
		killTimer(resizeTimer);
		resizeTimer = 0;

		QSize size = ui->preview->size();
		obs_display_resize(display, size.width(), size.height());
	}
}

void OBSBasicProperties::Init()
{
	gs_init_data init_data = {};

	show();
	App()->processEvents();

	QSize previewSize = ui->preview->size();
	init_data.cx      = uint32_t(previewSize.width());
	init_data.cy      = uint32_t(previewSize.height());
	init_data.format  = GS_RGBA;
	QTToGSWindow(ui->preview->winId(), init_data.window);

	display = obs_display_create(&init_data);

	if (display)
		obs_display_add_draw_callback(display,
				OBSBasicProperties::DrawPreview, this);
}
