/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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

#include <components/ApplicationAudioCaptureToolbar.hpp>
#include <components/AudioCaptureToolbar.hpp>
#include <components/BrowserToolbar.hpp>
#include <components/ColorSourceToolbar.hpp>
#include <components/DeviceCaptureToolbar.hpp>
#include <components/DisplayCaptureToolbar.hpp>
#include <components/GameCaptureToolbar.hpp>
#include <components/ImageSourceToolbar.hpp>
#include <components/MediaControls.hpp>
#include <components/TextSourceToolbar.hpp>
#include <components/WindowCaptureToolbar.hpp>

#include <qt-wrappers.hpp>

void OBSBasic::copyActionsDynamicProperties()
{
	// Themes need the QAction dynamic properties
	for (QAction *x : ui->scenesToolbar->actions()) {
		QWidget *temp = ui->scenesToolbar->widgetForAction(x);

		if (!temp)
			continue;

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}

	for (QAction *x : ui->sourcesToolbar->actions()) {
		QWidget *temp = ui->sourcesToolbar->widgetForAction(x);

		if (!temp)
			continue;

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}

	for (QAction *x : ui->mixerToolbar->actions()) {
		QWidget *temp = ui->mixerToolbar->widgetForAction(x);

		if (!temp)
			continue;

		for (QByteArray &y : x->dynamicPropertyNames()) {
			temp->setProperty(y, x->property(y));
		}
	}
}

void OBSBasic::ClearContextBar()
{
	QLayoutItem *la = ui->contextControlsContainer->layout()->itemAt(0);
	if (la) {
		delete la->widget();
		ui->contextControlsContainer->layout()->removeItem(la);
	}
}

void OBSBasic::UpdateContextBarVisibility()
{
	int width = ui->centralwidget->size().width();

	ContextBarSize contextBarSizeNew;
	if (width >= 740) {
		contextBarSizeNew = ContextBarSize_Normal;
	} else if (width >= 600) {
		contextBarSizeNew = ContextBarSize_Reduced;
	} else {
		contextBarSizeNew = ContextBarSize_Minimized;
	}

	if (contextBarSize == contextBarSizeNew)
		return;

	contextBarSize = contextBarSizeNew;
	UpdateContextBarDeferred();
}

static bool is_network_media_source(obs_source_t *source, const char *id)
{
	if (strcmp(id, "ffmpeg_source") != 0)
		return false;

	OBSDataAutoRelease s = obs_source_get_settings(source);
	bool is_local_file = obs_data_get_bool(s, "is_local_file");

	return !is_local_file;
}

void OBSBasic::UpdateContextBarDeferred(bool force)
{
	QMetaObject::invokeMethod(this, "UpdateContextBar", Qt::QueuedConnection, Q_ARG(bool, force));
}

void OBSBasic::SourceToolBarActionsSetEnabled()
{
	bool enable = false;
	bool disableProps = false;

	OBSSceneItem item = GetCurrentSceneItem();

	if (item) {
		OBSSource source = obs_sceneitem_get_source(item);
		disableProps = !obs_source_configurable(source);

		enable = true;
	}

	if (disableProps)
		ui->actionSourceProperties->setEnabled(false);
	else
		ui->actionSourceProperties->setEnabled(enable);

	ui->actionRemoveSource->setEnabled(enable);
	ui->actionSourceUp->setEnabled(enable);
	ui->actionSourceDown->setEnabled(enable);

	RefreshToolBarStyling(ui->sourcesToolbar);
}

void OBSBasic::UpdateContextBar(bool force)
{
	SourceToolBarActionsSetEnabled();

	if (!ui->contextContainer->isVisible() && !force)
		return;

	OBSSceneItem item = GetCurrentSceneItem();

	if (item) {
		obs_source_t *source = obs_sceneitem_get_source(item);

		bool updateNeeded = true;
		QLayoutItem *la = ui->contextControlsContainer->layout()->itemAt(0);
		if (la) {
			if (SourceToolbar *toolbar = dynamic_cast<SourceToolbar *>(la->widget())) {
				if (toolbar->GetSource() == source)
					updateNeeded = false;
			} else if (MediaControls *toolbar = dynamic_cast<MediaControls *>(la->widget())) {
				if (toolbar->GetSource() == source)
					updateNeeded = false;
			}
		}

		const char *id = obs_source_get_unversioned_id(source);
		uint32_t flags = obs_source_get_output_flags(source);

		if (contextBarSize >= ContextBarSize_Reduced && (updateNeeded || force)) {
			ClearContextBar();
			if (flags & OBS_SOURCE_CONTROLLABLE_MEDIA) {
				if (!is_network_media_source(source, id)) {
					MediaControls *mediaControls = new MediaControls(ui->contextControlsContainer);
					mediaControls->SetSource(source);

					ui->contextControlsContainer->layout()->addWidget(mediaControls);
				}
			} else if (strcmp(id, "browser_source") == 0) {
				BrowserToolbar *c = new BrowserToolbar(ui->contextControlsContainer, source);
				ui->contextControlsContainer->layout()->addWidget(c);

				connect(c, &BrowserToolbar::interactClicked, this,
					&OBSBasic::on_actionInteract_triggered);

			} else if (strcmp(id, "wasapi_input_capture") == 0 ||
				   strcmp(id, "wasapi_output_capture") == 0 ||
				   strcmp(id, "coreaudio_input_capture") == 0 ||
				   strcmp(id, "coreaudio_output_capture") == 0 ||
				   strcmp(id, "pulse_input_capture") == 0 || strcmp(id, "pulse_output_capture") == 0 ||
				   strcmp(id, "alsa_input_capture") == 0) {
				AudioCaptureToolbar *c = new AudioCaptureToolbar(ui->contextControlsContainer, source);
				c->Init();
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "wasapi_process_output_capture") == 0) {
				ApplicationAudioCaptureToolbar *c =
					new ApplicationAudioCaptureToolbar(ui->contextControlsContainer, source);
				c->Init();
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "window_capture") == 0 || strcmp(id, "xcomposite_input") == 0) {
				WindowCaptureToolbar *c =
					new WindowCaptureToolbar(ui->contextControlsContainer, source);
				c->Init();
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "monitor_capture") == 0 || strcmp(id, "display_capture") == 0 ||
				   strcmp(id, "xshm_input") == 0) {
				DisplayCaptureToolbar *c =
					new DisplayCaptureToolbar(ui->contextControlsContainer, source);
				c->Init();
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "dshow_input") == 0) {
				DeviceCaptureToolbar *c =
					new DeviceCaptureToolbar(ui->contextControlsContainer, source);
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "game_capture") == 0) {
				GameCaptureToolbar *c = new GameCaptureToolbar(ui->contextControlsContainer, source);
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "image_source") == 0) {
				ImageSourceToolbar *c = new ImageSourceToolbar(ui->contextControlsContainer, source);
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "color_source") == 0) {
				ColorSourceToolbar *c = new ColorSourceToolbar(ui->contextControlsContainer, source);
				ui->contextControlsContainer->layout()->addWidget(c);

			} else if (strcmp(id, "text_ft2_source") == 0 || strcmp(id, "text_gdiplus") == 0) {
				TextSourceToolbar *c = new TextSourceToolbar(ui->contextControlsContainer, source);
				ui->contextControlsContainer->layout()->addWidget(c);
			}
		} else if (contextBarSize == ContextBarSize_Minimized) {
			ClearContextBar();
		}

		QIcon icon;

		if (strcmp(id, "scene") == 0)
			icon = GetSceneIcon();
		else if (strcmp(id, "group") == 0)
			icon = GetGroupIcon();
		else
			icon = GetSourceIcon(id);

		ui->contextContainer->setEnabled(true);
		toggleClass(ui->contextSubContainer, "context-frame", true);

		QPixmap pixmap = icon.pixmap(QSize(16, 16));
		ui->contextSourceIcon->setPixmap(pixmap);
		ui->contextSourceIcon->show();
		ui->contextSubContainer->hide();

		const char *name = obs_source_get_name(source);
		ui->contextSourceLabel->setText(name);
		ui->contextSourceLabel->setAlignment(Qt::AlignLeft);

		ui->contextControlsContainer->show();

		ui->sourceFiltersButton->setEnabled(true);
		ui->sourcePropertiesButton->setEnabled(obs_source_configurable(source));
		ui->contextButtonsContainer->show();
	} else {
		ClearContextBar();
		ui->contextContainer->setEnabled(false);
		toggleClass(ui->contextSubContainer, "context-frame", false);
		ui->contextSubContainer->show();
		ui->contextSourceIcon->hide();
		ui->contextSourceLabel->setAlignment(Qt::AlignHCenter);
		ui->contextSourceLabel->setText(QTStr("ContextBar.NoSelectedSource"));

		ui->contextControlsContainer->hide();

		ui->sourceFiltersButton->setEnabled(false);
		ui->sourcePropertiesButton->setEnabled(false);
		ui->contextButtonsContainer->hide();
	}

	if (contextBarSize == ContextBarSize_Normal) {
		ui->sourcePropertiesButton->setText(QTStr("Properties"));
		ui->sourceFiltersButton->setText(QTStr("Filters"));
	} else {
		ui->sourcePropertiesButton->setText("");
		ui->sourceFiltersButton->setText("");
	}
}

void OBSBasic::ShowContextBar()
{
	on_toggleContextBar_toggled(true);
	ui->toggleContextBar->setChecked(true);
}

void OBSBasic::HideContextBar()
{
	on_toggleContextBar_toggled(false);
	ui->toggleContextBar->setChecked(false);
}

void OBSBasic::on_toggleContextBar_toggled(bool visible)
{
	config_set_bool(App()->GetUserConfig(), "BasicWindow", "ShowContextToolbars", visible);
	this->ui->contextContainer->setVisible(visible);
	UpdateContextBar(true);
}
