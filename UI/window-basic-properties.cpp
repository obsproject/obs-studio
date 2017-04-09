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
#include "properties-view.hpp"

#include <QCloseEvent>
#include <QScreen>
#include <QWindow>
#include <QMessageBox>

using namespace std;

OBSBasicProperties::OBSBasicProperties(QWidget *parent, OBSSource source_)
	: QDialog                (parent),
	  preview                (new OBSQTDisplay(this)),
	  main                   (qobject_cast<OBSBasic*>(parent)),
	  acceptClicked          (false),
	  source                 (source_),
	  removedSignal          (obs_source_get_signal_handler(source),
	                          "remove", OBSBasicProperties::SourceRemoved,
	                          this),
	  renamedSignal          (obs_source_get_signal_handler(source),
	                          "rename", OBSBasicProperties::SourceRenamed,
	                          this),
	  oldSettings            (obs_data_create()),
	  buttonBox              (new QDialogButtonBox(this))
{
	int cx = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
			"cx");
	int cy = (int)config_get_int(App()->GlobalConfig(), "PropertiesWindow",
			"cy");

	buttonBox->addButton(QTStr("OK"), QDialogButtonBox::AcceptRole);
	buttonBox->addButton(QTStr("Cancel"), QDialogButtonBox::RejectRole);
	buttonBox->addButton(QTStr("Defaults"), QDialogButtonBox::ResetRole);
	buttonBox->setObjectName(QStringLiteral("buttonBox"));

	if (cx > 400 && cy > 400)
		resize(cx, cy);
	else
		resize(720, 580);

	QMetaObject::connectSlotsByName(this);

	/* The OBSData constructor increments the reference once */
	obs_data_release(oldSettings);

	OBSData settings = obs_source_get_settings(source);
	obs_data_apply(oldSettings, settings);
	obs_data_release(settings);

	view = new OBSPropertiesView(settings, source,
			(PropertiesReloadCallback)obs_source_properties,
			(PropertiesUpdateCallback)obs_source_update);
	view->setMinimumHeight(150);

	preview->setMinimumSize(20, 150);
	preview->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,
				QSizePolicy::Expanding));

	// Create a QSplitter to keep a unified workflow here.
	windowSplitter = new QSplitter(Qt::Orientation::Vertical, this);
	windowSplitter->addWidget(preview);
	windowSplitter->addWidget(view);
	windowSplitter->setChildrenCollapsible(false);
	//windowSplitter->setSizes(QList<int>({ 16777216, 150 }));
	windowSplitter->setStretchFactor(0, 3);
	windowSplitter->setStretchFactor(1, 1);

	setLayout(new QVBoxLayout(this));
	layout()->addWidget(windowSplitter);
	layout()->addWidget(buttonBox);
	layout()->setAlignment(buttonBox, Qt::AlignRight | Qt::AlignBottom);

	view->show();
	installEventFilter(CreateShortcutFilter());

	const char *name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name)));

	obs_source_inc_showing(source);

	updatePropertiesSignal.Connect(obs_source_get_signal_handler(source),
			"update_properties",
			OBSBasicProperties::UpdateProperties,
			this);

	auto addDrawCallback = [this] ()
	{
		obs_display_add_draw_callback(preview->GetDisplay(),
				OBSBasicProperties::DrawPreview, this);
	};

	enum obs_source_type type = obs_source_get_type(source);
	uint32_t caps = obs_source_get_output_flags(source);
	bool drawable_type = type == OBS_SOURCE_TYPE_INPUT ||
		type == OBS_SOURCE_TYPE_SCENE;

	if (drawable_type && (caps & OBS_SOURCE_VIDEO) != 0)
		connect(preview.data(), &OBSQTDisplay::DisplayCreated,
				addDrawCallback);
}

OBSBasicProperties::~OBSBasicProperties()
{
	obs_source_dec_showing(source);
	main->SaveProject();
}

void OBSBasicProperties::SourceRemoved(void *data, calldata_t *params)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicProperties*>(data),
			"close");

	UNUSED_PARAMETER(params);
}

void OBSBasicProperties::SourceRenamed(void *data, calldata_t *params)
{
	const char *name = calldata_string(params, "new_name");
	QString title = QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name));

	QMetaObject::invokeMethod(static_cast<OBSBasicProperties*>(data),
	                "setWindowTitle", Q_ARG(QString, title));
}

void OBSBasicProperties::UpdateProperties(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicProperties*>(data)->view,
			"ReloadProperties");
}

void OBSBasicProperties::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole val = buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::AcceptRole) {
		acceptClicked = true;
		close();

		if (view->DeferUpdate())
			view->UpdateSettings();

	} else if (val == QDialogButtonBox::RejectRole) {
		obs_data_t *settings = obs_source_get_settings(source);
		obs_data_clear(settings);
		obs_data_release(settings);

		if (view->DeferUpdate())
			obs_data_apply(settings, oldSettings);
		else
			obs_source_update(source, oldSettings);

		close();

	} else if (val == QDialogButtonBox::ResetRole) {
		obs_data_t *settings = obs_source_get_settings(source);
		obs_data_clear(settings);
		obs_data_release(settings);

		if (!view->DeferUpdate())
			obs_source_update(source, nullptr);

		view->RefreshProperties();
	}
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

void OBSBasicProperties::Cleanup()
{
	config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cx",
			width());
	config_set_int(App()->GlobalConfig(), "PropertiesWindow", "cy",
			height());

	obs_display_remove_draw_callback(preview->GetDisplay(),
		OBSBasicProperties::DrawPreview, this);
}

void OBSBasicProperties::reject()
{
	if (!acceptClicked && (CheckSettings() != 0)) {
		if (!ConfirmQuit()) {
			return;
		}
	}

	Cleanup();
	done(0);
}

void OBSBasicProperties::closeEvent(QCloseEvent *event)
{
	if (!acceptClicked && (CheckSettings() != 0)) {
		if (!ConfirmQuit()) {
			event->ignore();
			return;
		}
	}

	QDialog::closeEvent(event);
	if (!event->isAccepted())
		return;

	Cleanup();
}

void OBSBasicProperties::Init()
{
	show();
}

int OBSBasicProperties::CheckSettings()
{
	OBSData currentSettings = obs_source_get_settings(source);
	const char *oldSettingsJson = obs_data_get_json(oldSettings);
	const char *currentSettingsJson = obs_data_get_json(currentSettings);

	int ret = strcmp(currentSettingsJson, oldSettingsJson);

	obs_data_release(currentSettings);
	return ret;
}

bool OBSBasicProperties::ConfirmQuit()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this,
			QTStr("Basic.PropertiesWindow.ConfirmTitle"),
			QTStr("Basic.PropertiesWindow.Confirm"),
			QMessageBox::Save | QMessageBox::Discard |
			QMessageBox::Cancel);

	switch (button) {
	case QMessageBox::Save:
		if (view->DeferUpdate())
			view->UpdateSettings();
		// Do nothing because the settings are already updated
		break;
	case QMessageBox::Discard:
		obs_source_update(source, oldSettings);
		break;
	case QMessageBox::Cancel:
		return false;
		break;
	default:
		/* If somehow the dialog fails to show, just default to
		 * saving the settings. */
		break;
	}
	return true;
}
