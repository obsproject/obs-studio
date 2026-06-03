/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

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

#include "OBSBasicProperties.hpp"

#include <utility/display-helpers.hpp>
#include <widgets/OBSBasic.hpp>
#include <widgets/OBSSourceWidget.hpp>

#include <properties-view.hpp>
#include <qt-wrappers.hpp>
#include <vertical-scroll-area.hpp>

#include <QPushButton>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <Windows.h>
#endif

#include "moc_OBSBasicProperties.cpp"

using namespace std;

static void CreateTransitionScene(OBSSource scene, const char *text, uint32_t color);

OBSBasicProperties::OBSBasicProperties(QWidget *parent, OBSSource source_)
	: QDialog(parent),
	  ui(new Ui::OBSBasicProperties),
	  main(qobject_cast<OBSBasic *>(parent)),
	  acceptClicked(false),
	  source(source_),
	  removedSignal(obs_source_get_signal_handler(source), "remove", OBSBasicProperties::SourceRemoved, this),
	  renamedSignal(obs_source_get_signal_handler(source), "rename", OBSBasicProperties::SourceRenamed, this),
	  oldSettings(obs_data_create())
{
	int cx = (int)config_get_int(App()->GetAppConfig(), "PropertiesWindow", "cx");
	int cy = (int)config_get_int(App()->GetAppConfig(), "PropertiesWindow", "cy");

	enum obs_source_type type = obs_source_get_type(source);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);
	ui->buttonBox->button(QDialogButtonBox::Ok)->setFocus();

	if (cx > 400 && cy > 400) {
		resize(cx, cy);
	}

	/* The OBSData constructor increments the reference once */
	obs_data_release(oldSettings);

	OBSDataAutoRelease nd_settings = obs_source_get_settings(source);
	obs_data_apply(oldSettings, nd_settings);

	view = new OBSPropertiesView(nd_settings.Get(), source, (PropertiesReloadCallback)obs_source_properties,
				     (PropertiesUpdateCallback) nullptr, // No special handling required for undo/redo
				     (PropertiesVisualUpdateCb)obs_source_update);
	view->setMinimumHeight(150);

	ui->propertiesLayout->addWidget(view);

	if (type == OBS_SOURCE_TYPE_TRANSITION) {
		ui->transitionButton->setVisible(true);
		connect(ui->transitionButton, &QPushButton::clicked, this,
			&OBSBasicProperties::previewTransitionClicked);
	} else {
		ui->transitionButton->setVisible(false);
	}

	view->show();
	installEventFilter(CreateShortcutFilter());

	const char *name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name)));

	obs_source_inc_showing(source);

	updatePropertiesSignal.Connect(obs_source_get_signal_handler(source), "update_properties",
				       OBSBasicProperties::UpdateProperties, this);

	uint32_t caps = obs_source_get_output_flags(source);
	bool isSourceType = type == OBS_SOURCE_TYPE_INPUT || type == OBS_SOURCE_TYPE_SCENE;
	bool hasVideo = (caps & OBS_SOURCE_VIDEO) != 0;

	auto *previewWidget = new OBSSourceWidget(this);
	ui->windowSplitter->insertWidget(0, previewWidget);

	if (hasVideo && isSourceType) {
		previewWidget->setSource(source_);
	} else if (type == OBS_SOURCE_TYPE_TRANSITION) {
		sourceA = obs_source_create_private("scene", "sourceA", nullptr);
		sourceB = obs_source_create_private("scene", "sourceB", nullptr);

		uint32_t colorA = 0xFFB26F52;
		uint32_t colorB = 0xFF6FB252;

		CreateTransitionScene(sourceA.Get(), "A", colorA);
		CreateTransitionScene(sourceB.Get(), "B", colorB);

		/**
		 * The cloned source is made from scratch, rather than using
		 * obs_source_duplicate, as the stinger transition would not
		 * play correctly otherwise.
		 */

		OBSDataAutoRelease settings = obs_source_get_settings(source);

		sourceClone = obs_source_create_private(obs_source_get_id(source), "clone", settings);

		obs_source_inc_active(sourceClone);
		obs_transition_set(sourceClone, sourceA);

		auto updateCallback = [this]() {
			OBSDataAutoRelease settings = obs_source_get_settings(source);
			obs_source_update(sourceClone, settings);

			obs_transition_clear(sourceClone);
			obs_transition_set(sourceClone, sourceA);
			obs_transition_force_stop(sourceClone);

			direction = true;
		};

		connect(view, &OBSPropertiesView::Changed, this, updateCallback);

		previewWidget->setSource(sourceClone);
		previewWidget->setForceLinearSRGB(true);
	} else {
		previewWidget->hide();
	}

	previewWidget->resize(width(), ui->windowSplitter->height() / 2);

	ui->windowSplitter->setStretchFactor(0, 2);
	ui->windowSplitter->setStretchFactor(1, 3);

	connect(ui->defaultsButton, &QPushButton::clicked, this, &OBSBasicProperties::restoreDefaultsClicked);
}

OBSBasicProperties::~OBSBasicProperties()
{
	if (sourceClone) {
		obs_source_dec_active(sourceClone);
	}
	obs_source_dec_showing(source);
	main->SaveProject();
	main->UpdateContextBarDeferred(true);
}

static obs_source_t *CreateLabel(const char *name, size_t h)
{
	OBSDataAutoRelease settings = obs_data_create();
	OBSDataAutoRelease font = obs_data_create();

	std::string text;
	text += " ";
	text += name;
	text += " ";

#if defined(_WIN32)
	obs_data_set_string(font, "face", "Arial");
#elif defined(__APPLE__)
	obs_data_set_string(font, "face", "Helvetica");
#else
	obs_data_set_string(font, "face", "Monospace");
#endif
	obs_data_set_int(font, "flags", 1); // Bold text
	obs_data_set_int(font, "size", min(int(h), 300));

	obs_data_set_obj(settings, "font", font);
	obs_data_set_string(settings, "text", text.c_str());
	obs_data_set_bool(settings, "outline", false);

#ifdef _WIN32
	const char *text_source_id = "text_gdiplus";
#else
	const char *text_source_id = "text_ft2_source";
#endif

	obs_source_t *txtSource = obs_source_create_private(text_source_id, name, settings);

	return txtSource;
}

static void CreateTransitionScene(OBSSource scene, const char *text, uint32_t color)
{
	OBSDataAutoRelease settings = obs_data_create();
	obs_data_set_int(settings, "width", obs_source_get_width(scene));
	obs_data_set_int(settings, "height", obs_source_get_height(scene));
	obs_data_set_int(settings, "color", color);

	OBSSourceAutoRelease colorBG = obs_source_create_private("color_source", "background", settings);

	obs_scene_add(obs_scene_from_source(scene), colorBG);

	OBSSourceAutoRelease label = CreateLabel(text, obs_source_get_height(scene));
	obs_sceneitem_t *item = obs_scene_add(obs_scene_from_source(scene), label);

	vec2 size;
	vec2_set(&size, obs_source_get_width(scene),
#ifdef _WIN32
		 obs_source_get_height(scene));
#else
		 obs_source_get_height(scene) * 0.8);
#endif

	obs_sceneitem_set_bounds(item, &size);
	obs_sceneitem_set_bounds_type(item, OBS_BOUNDS_SCALE_INNER);
}

static bool ConfirmReset(QWidget *parent)
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(parent, QTStr("ConfirmReset.Title"), QTStr("ConfirmReset.Text"),
					 QMessageBox::Yes | QMessageBox::No);

	return button == QMessageBox::Yes;
}

void OBSBasicProperties::restoreDefaultsClicked()
{
	if (!ConfirmReset(this)) {
		return;
	}

	OBSDataAutoRelease settings = obs_source_get_settings(source);
	obs_data_clear(settings);

	if (!view->DeferUpdate()) {
		obs_source_update(source, nullptr);
	}

	view->ReloadProperties();
}

void OBSBasicProperties::previewTransitionClicked()
{
	OBSSource start;
	OBSSource end;

	if (direction) {
		start = sourceA;
		end = sourceB;
	} else {
		start = sourceB;
		end = sourceA;
	}

	obs_transition_set(sourceClone, start);
	obs_transition_start(sourceClone, OBS_TRANSITION_MODE_AUTO, main->GetTransitionDuration(), end);
	direction = !direction;

	start = nullptr;
	end = nullptr;
}

void OBSBasicProperties::SourceRemoved(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicProperties *>(data), "close");
}

void OBSBasicProperties::SourceRenamed(void *data, calldata_t *params)
{
	const char *name = calldata_string(params, "new_name");
	QString title = QTStr("Basic.PropertiesWindow").arg(QT_UTF8(name));

	QMetaObject::invokeMethod(static_cast<OBSBasicProperties *>(data), "setWindowTitle", Q_ARG(QString, title));
}

void OBSBasicProperties::UpdateProperties(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicProperties *>(data)->view, "ReloadProperties");
}

void OBSBasicProperties::on_buttonBox_clicked(QAbstractButton *button)
{
	QDialogButtonBox::ButtonRole val = ui->buttonBox->buttonRole(button);

	if (val == QDialogButtonBox::AcceptRole) {

		std::string scene_uuid = obs_source_get_uuid(main->GetCurrentSceneSource());

		auto undo_redo = [scene_uuid](const std::string &data) {
			OBSDataAutoRelease settings = obs_data_create_from_json(data.c_str());
			OBSSourceAutoRelease source =
				obs_get_source_by_uuid(obs_data_get_string(settings, "undo_uuid"));
			obs_source_reset_settings(source, settings);

			obs_source_update_properties(source);

			OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());

			OBSBasic::Get()->SetCurrentScene(scene_source.Get(), true);
		};

		OBSDataAutoRelease new_settings = obs_data_create();
		OBSDataAutoRelease curr_settings = obs_source_get_settings(source);
		obs_data_apply(new_settings, curr_settings);
		obs_data_set_string(new_settings, "undo_uuid", obs_source_get_uuid(source));
		obs_data_set_string(oldSettings, "undo_uuid", obs_source_get_uuid(source));

		std::string undo_data(obs_data_get_json(oldSettings));
		std::string redo_data(obs_data_get_json(new_settings));

		if (undo_data.compare(redo_data) != 0)
			main->undo_s.add_action(QTStr("Undo.Properties").arg(obs_source_get_name(source)), undo_redo,
						undo_redo, undo_data, redo_data);

		acceptClicked = true;
		close();

		if (view->DeferUpdate())
			view->UpdateSettings();

	} else if (val == QDialogButtonBox::RejectRole) {
		OBSDataAutoRelease settings = obs_source_get_settings(source);
		obs_data_clear(settings);

		if (view->DeferUpdate())
			obs_data_apply(settings, oldSettings);
		else
			obs_source_update(source, oldSettings);

		close();
	}
}

void OBSBasicProperties::Cleanup()
{
	config_set_int(App()->GetAppConfig(), "PropertiesWindow", "cx", width());
	config_set_int(App()->GetAppConfig(), "PropertiesWindow", "cy", height());
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
	QDialog::closeEvent(event);
	if (event->isAccepted())
		Cleanup();
}

void OBSBasicProperties::Init()
{
	show();
}

int OBSBasicProperties::CheckSettings()
{
	OBSDataAutoRelease currentSettings = obs_source_get_settings(source);
	const char *oldSettingsJson = obs_data_get_json(oldSettings);
	const char *currentSettingsJson = obs_data_get_json(currentSettings);

	return strcmp(currentSettingsJson, oldSettingsJson);
}

bool OBSBasicProperties::ConfirmQuit()
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(this, QTStr("Basic.PropertiesWindow.ConfirmTitle"),
					 QTStr("Basic.PropertiesWindow.Confirm"),
					 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

	switch (button) {
	case QMessageBox::Save:
		acceptClicked = true;
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
