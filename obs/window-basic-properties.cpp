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

#include <QCloseEvent>
#include <QScreen>
#include <QWindow>

using namespace std;

OBSBasicProperties::OBSBasicProperties(QWidget *parent, OBSSource source_)
	: QDialog       (parent),
	  main          (qobject_cast<OBSBasic*>(parent)),
	  resizeTimer   (0),
	  ui            (new Ui::OBSBasicProperties),
	  source        (source_),
	  removedSignal (obs_source_get_signal_handler(source), "remove",
	                 OBSBasicProperties::SourceRemoved, this)
{
	ui->setupUi(this);

	OBSData settings = obs_source_get_settings(source);
	obs_data_release(settings);

	view = new OBSPropertiesView(settings,
			obs_source_properties(source), source,
			(PropertiesUpdateCallback)obs_source_update);

	layout()->addWidget(view);
	layout()->setAlignment(view, Qt::AlignBottom);
	view->setMinimumHeight(150);
	view->show();

	connect(windowHandle(), &QWindow::screenChanged, [this]() {
		if (resizeTimer)
			killTimer(resizeTimer);
		resizeTimer = startTimer(100);
	});

	const char *name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name)));
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

	uint32_t sourceCX = max(obs_source_get_width(window->source), 1u);
	uint32_t sourceCY = max(obs_source_get_height(window->source), 1u);

	int   x, y;
	int   newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY),
			-100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	obs_source_video_render(window->source);

	gs_projection_pop();
	gs_viewport_pop();
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

		QSize size = GetPixelSize(ui->preview);
		obs_display_resize(display, size.width(), size.height());
	}
}

void OBSBasicProperties::closeEvent(QCloseEvent *event)
{
	QDialog::closeEvent(event);
	if (!event->isAccepted())
		return;

	// remove draw callback in case our drawable surfaces go away before
	// the destructor gets called
	obs_display_remove_draw_callback(display,
			OBSBasicProperties::DrawPreview, this);
}

void OBSBasicProperties::Init()
{
	gs_init_data init_data = {};

	show();
	App()->processEvents();

	QSize previewSize = GetPixelSize(ui->preview);
	init_data.cx      = uint32_t(previewSize.width());
	init_data.cy      = uint32_t(previewSize.height());
	init_data.format  = GS_RGBA;
	QTToGSWindow(ui->preview->winId(), init_data.window);

	display = obs_display_create(&init_data);

	if (display)
		obs_display_add_draw_callback(display,
				OBSBasicProperties::DrawPreview, this);
}
