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
#include "ColorSelect.hpp"
#include "OBSProjector.hpp"
#include "VolControl.hpp"

#include <dialogs/NameDialog.hpp>
#include <dialogs/OBSBasicAdvAudio.hpp>
#include <dialogs/OBSBasicSourceSelect.hpp>
#include <utility/item-widget-helpers.hpp>

#include <qt-wrappers.hpp>

#include <QWidgetAction>

#include <sstream>

using namespace std;

static inline bool HasAudioDevices(const char *source_id)
{
	const char *output_id = source_id;
	obs_properties_t *props = obs_get_source_properties(output_id);
	size_t count = 0;

	if (!props)
		return false;

	obs_property_t *devices = obs_properties_get(props, "device_id");
	if (devices)
		count = obs_property_list_item_count(devices);

	obs_properties_destroy(props);

	return count != 0;
}

void OBSBasic::CreateFirstRunSources()
{
	bool hasDesktopAudio = HasAudioDevices(App()->OutputAudioSource());
	bool hasInputAudio = HasAudioDevices(App()->InputAudioSource());

#ifdef __APPLE__
	/* On macOS 13 and above, the SCK based audio capture provides a
	 * better alternative to the device-based audio capture. */
	if (__builtin_available(macOS 13.0, *)) {
		hasDesktopAudio = false;
	}
#endif

	if (hasDesktopAudio)
		ResetAudioDevice(App()->OutputAudioSource(), "default", Str("Basic.DesktopDevice1"), 1);
	if (hasInputAudio)
		ResetAudioDevice(App()->InputAudioSource(), "default", Str("Basic.AuxDevice1"), 3);
}

OBSSceneItem OBSBasic::GetSceneItem(QListWidgetItem *item)
{
	return item ? GetOBSRef<OBSSceneItem>(item) : nullptr;
}

OBSSceneItem OBSBasic::GetCurrentSceneItem()
{
	return ui->sources->Get(GetTopSelectedSourceItem());
}

static void RenameListValues(QListWidget *listWidget, const QString &newName, const QString &prevName)
{
	QList<QListWidgetItem *> items = listWidget->findItems(prevName, Qt::MatchExactly);

	for (int i = 0; i < items.count(); i++)
		items[i]->setText(newName);
}

void OBSBasic::RenameSources(OBSSource source, QString newName, QString prevName)
{
	RenameListValues(ui->scenes, newName, prevName);

	if (vcamConfig.type == VCamOutputType::SourceOutput && prevName == QString::fromStdString(vcamConfig.source))
		vcamConfig.source = newName.toStdString();
	if (vcamConfig.type == VCamOutputType::SceneOutput && prevName == QString::fromStdString(vcamConfig.scene))
		vcamConfig.scene = newName.toStdString();

	SaveProject();

	obs_scene_t *scene = obs_scene_from_source(source);
	if (scene)
		OBSProjector::UpdateMultiviewProjectors();

	UpdateContextBar();
	UpdatePreviewProgramIndicators();
}

void OBSBasic::GetAudioSourceFilters()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	CreateFiltersWindow(source);
}

void OBSBasic::GetAudioSourceProperties()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	obs_source_t *source = vol->GetSource();

	CreatePropertiesWindow(source);
}

void OBSBasic::MixerRenameSource()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	VolControl *vol = action->property("volControl").value<VolControl *>();
	OBSSource source = vol->GetSource();

	const char *prevName = obs_source_get_name(source);

	for (;;) {
		string name;
		bool accepted = NameDialog::AskForName(this, QTStr("Basic.Main.MixerRename.Title"),
						       QTStr("Basic.Main.MixerRename.Text"), name, QT_UTF8(prevName));
		if (!accepted)
			return;

		if (name.empty()) {
			OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			continue;
		}

		OBSSourceAutoRelease sourceTest = obs_get_source_by_name(name.c_str());

		if (sourceTest) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
			continue;
		}

		obs_source_set_name(source, name.c_str());
		break;
	}
}

void OBSBasic::ActivateAudioSource(OBSSource source)
{
	if (SourceMixerHidden(source))
		return;
	if (!obs_source_active(source))
		return;
	if (!obs_source_audio_active(source))
		return;

	bool vertical = config_get_bool(App()->GetUserConfig(), "BasicWindow", "VerticalVolControl");
	VolControl *vol = new VolControl(source, true, vertical);

	vol->EnableSlider(!SourceVolumeLocked(source));

	double meterDecayRate = config_get_double(activeConfiguration, "Audio", "MeterDecayRate");
	vol->SetMeterDecayRate(meterDecayRate);

	uint32_t peakMeterTypeIdx = config_get_uint(activeConfiguration, "Audio", "PeakMeterType");

	enum obs_peak_meter_type peakMeterType;
	switch (peakMeterTypeIdx) {
	case 0:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	case 1:
		peakMeterType = TRUE_PEAK_METER;
		break;
	default:
		peakMeterType = SAMPLE_PEAK_METER;
		break;
	}

	vol->setPeakMeterType(peakMeterType);

	vol->setContextMenuPolicy(Qt::CustomContextMenu);

	connect(vol, &QWidget::customContextMenuRequested, this, &OBSBasic::VolControlContextMenu);
	connect(vol, &VolControl::ConfigClicked, this, &OBSBasic::VolControlContextMenu);

	InsertQObjectByName(volumes, vol);

	for (auto volume : volumes) {
		if (vertical)
			ui->vVolControlLayout->addWidget(volume);
		else
			ui->hVolControlLayout->addWidget(volume);
	}
}

void OBSBasic::DeactivateAudioSource(OBSSource source)
{
	for (size_t i = 0; i < volumes.size(); i++) {
		if (volumes[i]->GetSource() == source) {
			delete volumes[i];
			volumes.erase(volumes.begin() + i);
			break;
		}
	}
}

bool OBSBasic::QueryRemoveSource(obs_source_t *source)
{
	if (obs_source_get_type(source) == OBS_SOURCE_TYPE_SCENE && !obs_source_is_group(source)) {
		int count = ui->scenes->count();

		if (count == 1) {
			OBSMessageBox::information(this, QTStr("FinalScene.Title"), QTStr("FinalScene.Text"));
			return false;
		}
	}

	const char *name = obs_source_get_name(source);

	QString text = QTStr("ConfirmRemove.Text").arg(QT_UTF8(name));

	QMessageBox remove_source(this);
	remove_source.setText(text);
	QPushButton *Yes = remove_source.addButton(QTStr("Yes"), QMessageBox::YesRole);
	remove_source.setDefaultButton(Yes);
	remove_source.addButton(QTStr("No"), QMessageBox::NoRole);
	remove_source.setIcon(QMessageBox::Question);
	remove_source.setWindowTitle(QTStr("ConfirmRemove.Title"));
	remove_source.exec();

	return Yes == remove_source.clickedButton();
}

void OBSBasic::ReorderSources(OBSScene scene)
{
	if (scene != GetCurrentScene() || ui->sources->IgnoreReorder())
		return;

	ui->sources->ReorderItems();
	SaveProject();
}

void OBSBasic::RefreshSources(OBSScene scene)
{
	if (scene != GetCurrentScene() || ui->sources->IgnoreReorder())
		return;

	ui->sources->RefreshItems();
	SaveProject();
}

void OBSBasic::SourceCreated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_scene_from_source(source) != NULL)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "AddScene", WaitConnection(),
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceRemoved(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_scene_from_source(source) != NULL)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "RemoveScene",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ActivateAudioSource",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_AUDIO)
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "DeactivateAudioSource",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceAudioActivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");

	if (obs_source_active(source))
		QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "ActivateAudioSource",
					  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceAudioDeactivated(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "DeactivateAudioSource",
				  Q_ARG(OBSSource, OBSSource(source)));
}

void OBSBasic::SourceRenamed(void *data, calldata_t *params)
{
	obs_source_t *source = (obs_source_t *)calldata_ptr(params, "source");
	const char *newName = calldata_string(params, "new_name");
	const char *prevName = calldata_string(params, "prev_name");

	QMetaObject::invokeMethod(static_cast<OBSBasic *>(data), "RenameSources", Q_ARG(OBSSource, source),
				  Q_ARG(QString, QT_UTF8(newName)), Q_ARG(QString, QT_UTF8(prevName)));

	blog(LOG_INFO, "Source '%s' renamed to '%s'", prevName, newName);
}

extern char *get_new_source_name(const char *name, const char *format);

void OBSBasic::ResetAudioDevice(const char *sourceId, const char *deviceId, const char *deviceDesc, int channel)
{
	bool disable = deviceId && strcmp(deviceId, "disabled") == 0;
	OBSSourceAutoRelease source;
	OBSDataAutoRelease settings;

	source = obs_get_output_source(channel);
	if (source) {
		if (disable) {
			obs_set_output_source(channel, nullptr);
		} else {
			settings = obs_source_get_settings(source);
			const char *oldId = obs_data_get_string(settings, "device_id");
			if (strcmp(oldId, deviceId) != 0) {
				obs_data_set_string(settings, "device_id", deviceId);
				obs_source_update(source, settings);
			}
		}

	} else if (!disable) {
		BPtr<char> name = get_new_source_name(deviceDesc, "%s (%d)");

		settings = obs_data_create();
		obs_data_set_string(settings, "device_id", deviceId);
		source = obs_source_create(sourceId, name, settings, nullptr);

		obs_set_output_source(channel, source);
	}
}

void OBSBasic::SetDeinterlacingMode()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_deinterlace_mode mode = (obs_deinterlace_mode)action->property("mode").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();
	obs_source_t *source = obs_sceneitem_get_source(sceneItem);

	obs_source_set_deinterlace_mode(source, mode);
}

void OBSBasic::SetDeinterlacingOrder()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_deinterlace_field_order order = (obs_deinterlace_field_order)action->property("order").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();
	obs_source_t *source = obs_sceneitem_get_source(sceneItem);

	obs_source_set_deinterlace_field_order(source, order);
}

QMenu *OBSBasic::AddDeinterlacingMenu(QMenu *menu, obs_source_t *source)
{
	obs_deinterlace_mode deinterlaceMode = obs_source_get_deinterlace_mode(source);
	obs_deinterlace_field_order deinterlaceOrder = obs_source_get_deinterlace_field_order(source);
	QAction *action;

#define ADD_MODE(name, mode)                                                             \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetDeinterlacingMode); \
	action->setProperty("mode", (int)mode);                                          \
	action->setCheckable(true);                                                      \
	action->setChecked(deinterlaceMode == mode);

	ADD_MODE("Disable", OBS_DEINTERLACE_MODE_DISABLE);
	ADD_MODE("Deinterlacing.Discard", OBS_DEINTERLACE_MODE_DISCARD);
	ADD_MODE("Deinterlacing.Retro", OBS_DEINTERLACE_MODE_RETRO);
	ADD_MODE("Deinterlacing.Blend", OBS_DEINTERLACE_MODE_BLEND);
	ADD_MODE("Deinterlacing.Blend2x", OBS_DEINTERLACE_MODE_BLEND_2X);
	ADD_MODE("Deinterlacing.Linear", OBS_DEINTERLACE_MODE_LINEAR);
	ADD_MODE("Deinterlacing.Linear2x", OBS_DEINTERLACE_MODE_LINEAR_2X);
	ADD_MODE("Deinterlacing.Yadif", OBS_DEINTERLACE_MODE_YADIF);
	ADD_MODE("Deinterlacing.Yadif2x", OBS_DEINTERLACE_MODE_YADIF_2X);
#undef ADD_MODE

	menu->addSeparator();

#define ADD_ORDER(name, order)                                                                          \
	action = menu->addAction(QTStr("Deinterlacing." name), this, &OBSBasic::SetDeinterlacingOrder); \
	action->setProperty("order", (int)order);                                                       \
	action->setCheckable(true);                                                                     \
	action->setChecked(deinterlaceOrder == order);

	ADD_ORDER("TopFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_TOP);
	ADD_ORDER("BottomFieldFirst", OBS_DEINTERLACE_FIELD_ORDER_BOTTOM);
#undef ADD_ORDER

	return menu;
}

void OBSBasic::SetScaleFilter()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_scale_type mode = (obs_scale_type)action->property("mode").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();

	obs_sceneitem_set_scale_filter(sceneItem, mode);
}

QMenu *OBSBasic::AddScaleFilteringMenu(QMenu *menu, obs_sceneitem_t *item)
{
	obs_scale_type scaleFilter = obs_sceneitem_get_scale_filter(item);
	QAction *action;

#define ADD_MODE(name, mode)                                                       \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetScaleFilter); \
	action->setProperty("mode", (int)mode);                                    \
	action->setCheckable(true);                                                \
	action->setChecked(scaleFilter == mode);

	ADD_MODE("Disable", OBS_SCALE_DISABLE);
	ADD_MODE("ScaleFiltering.Point", OBS_SCALE_POINT);
	ADD_MODE("ScaleFiltering.Bilinear", OBS_SCALE_BILINEAR);
	ADD_MODE("ScaleFiltering.Bicubic", OBS_SCALE_BICUBIC);
	ADD_MODE("ScaleFiltering.Lanczos", OBS_SCALE_LANCZOS);
	ADD_MODE("ScaleFiltering.Area", OBS_SCALE_AREA);
#undef ADD_MODE

	return menu;
}

void OBSBasic::SetBlendingMethod()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_blending_method method = (obs_blending_method)action->property("method").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();

	obs_sceneitem_set_blending_method(sceneItem, method);
}

QMenu *OBSBasic::AddBlendingMethodMenu(QMenu *menu, obs_sceneitem_t *item)
{
	obs_blending_method blendingMethod = obs_sceneitem_get_blending_method(item);
	QAction *action;

#define ADD_MODE(name, method)                                                        \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetBlendingMethod); \
	action->setProperty("method", (int)method);                                   \
	action->setCheckable(true);                                                   \
	action->setChecked(blendingMethod == method);

	ADD_MODE("BlendingMethod.Default", OBS_BLEND_METHOD_DEFAULT);
	ADD_MODE("BlendingMethod.SrgbOff", OBS_BLEND_METHOD_SRGB_OFF);
#undef ADD_MODE

	return menu;
}

void OBSBasic::SetBlendingMode()
{
	QAction *action = reinterpret_cast<QAction *>(sender());
	obs_blending_type mode = (obs_blending_type)action->property("mode").toInt();
	OBSSceneItem sceneItem = GetCurrentSceneItem();

	obs_sceneitem_set_blending_mode(sceneItem, mode);
}

QMenu *OBSBasic::AddBlendingModeMenu(QMenu *menu, obs_sceneitem_t *item)
{
	obs_blending_type blendingMode = obs_sceneitem_get_blending_mode(item);
	QAction *action;

#define ADD_MODE(name, mode)                                                        \
	action = menu->addAction(QTStr("" name), this, &OBSBasic::SetBlendingMode); \
	action->setProperty("mode", (int)mode);                                     \
	action->setCheckable(true);                                                 \
	action->setChecked(blendingMode == mode);

	ADD_MODE("BlendingMode.Normal", OBS_BLEND_NORMAL);
	ADD_MODE("BlendingMode.Additive", OBS_BLEND_ADDITIVE);
	ADD_MODE("BlendingMode.Subtract", OBS_BLEND_SUBTRACT);
	ADD_MODE("BlendingMode.Screen", OBS_BLEND_SCREEN);
	ADD_MODE("BlendingMode.Multiply", OBS_BLEND_MULTIPLY);
	ADD_MODE("BlendingMode.Lighten", OBS_BLEND_LIGHTEN);
	ADD_MODE("BlendingMode.Darken", OBS_BLEND_DARKEN);
#undef ADD_MODE

	return menu;
}

QMenu *OBSBasic::AddBackgroundColorMenu(QMenu *menu, QWidgetAction *widgetAction, ColorSelect *select,
					obs_sceneitem_t *item)
{
	QAction *action;

	menu->setStyleSheet(QString("*[bgColor=\"1\"]{background-color:rgba(255,68,68,33%);}"
				    "*[bgColor=\"2\"]{background-color:rgba(255,255,68,33%);}"
				    "*[bgColor=\"3\"]{background-color:rgba(68,255,68,33%);}"
				    "*[bgColor=\"4\"]{background-color:rgba(68,255,255,33%);}"
				    "*[bgColor=\"5\"]{background-color:rgba(68,68,255,33%);}"
				    "*[bgColor=\"6\"]{background-color:rgba(255,68,255,33%);}"
				    "*[bgColor=\"7\"]{background-color:rgba(68,68,68,33%);}"
				    "*[bgColor=\"8\"]{background-color:rgba(255,255,255,33%);}"));

	obs_data_t *privData = obs_sceneitem_get_private_settings(item);
	obs_data_release(privData);

	obs_data_set_default_int(privData, "color-preset", 0);
	int preset = obs_data_get_int(privData, "color-preset");

	action = menu->addAction(QTStr("Clear"), this, &OBSBasic::ColorChange);
	action->setCheckable(true);
	action->setProperty("bgColor", 0);
	action->setChecked(preset == 0);

	action = menu->addAction(QTStr("CustomColor"), this, &OBSBasic::ColorChange);
	action->setCheckable(true);
	action->setProperty("bgColor", 1);
	action->setChecked(preset == 1);

	menu->addSeparator();

	widgetAction->setDefaultWidget(select);

	for (int i = 1; i < 9; i++) {
		stringstream button;
		button << "preset" << i;
		QPushButton *colorButton = select->findChild<QPushButton *>(button.str().c_str());
		if (preset == i + 1)
			colorButton->setStyleSheet("border: 2px solid black");

		colorButton->setProperty("bgColor", i);
		select->connect(colorButton, &QPushButton::released, this, &OBSBasic::ColorChange);
	}

	menu->addAction(widgetAction);

	return menu;
}

void OBSBasic::CreateSourcePopupMenu(int idx, bool preview)
{
	QMenu popup(this);
	delete previewProjectorSource;
	delete sourceProjector;
	delete scaleFilteringMenu;
	delete blendingMethodMenu;
	delete blendingModeMenu;
	delete colorMenu;
	delete colorWidgetAction;
	delete colorSelect;
	delete deinterlaceMenu;

	if (preview) {
		QAction *action =
			popup.addAction(QTStr("Basic.Main.PreviewConextMenu.Enable"), this, &OBSBasic::TogglePreview);
		action->setCheckable(true);
		action->setChecked(obs_display_enabled(ui->preview->GetDisplay()));
		if (IsPreviewProgramMode())
			action->setEnabled(false);

		popup.addAction(ui->actionLockPreview);
		popup.addMenu(ui->scalingMenu);

		previewProjectorSource = new QMenu(QTStr("PreviewProjector"));
		AddProjectorMenuMonitors(previewProjectorSource, this, &OBSBasic::OpenPreviewProjector);

		popup.addMenu(previewProjectorSource);

		QAction *previewWindow = popup.addAction(QTStr("PreviewWindow"), this, &OBSBasic::OpenPreviewWindow);

		popup.addAction(previewWindow);

		popup.addAction(QTStr("Screenshot.Preview"), this, &OBSBasic::ScreenshotScene);

		popup.addSeparator();
	}

	QPointer<QMenu> addSourceMenu = CreateAddSourcePopupMenu();
	if (addSourceMenu)
		popup.addMenu(addSourceMenu);

	if (ui->sources->MultipleBaseSelected()) {
		popup.addSeparator();
		popup.addAction(QTStr("Basic.Main.GroupItems"), ui->sources, &SourceTree::GroupSelectedItems);

	} else if (ui->sources->GroupsSelected()) {
		popup.addSeparator();
		popup.addAction(QTStr("Basic.Main.Ungroup"), ui->sources, &SourceTree::UngroupSelectedGroups);
	}

	popup.addSeparator();
	popup.addAction(ui->actionCopySource);
	popup.addAction(ui->actionPasteRef);
	popup.addAction(ui->actionPasteDup);
	popup.addSeparator();

	popup.addSeparator();
	popup.addAction(ui->actionCopyFilters);
	popup.addAction(ui->actionPasteFilters);
	popup.addSeparator();

	if (idx != -1) {
		if (addSourceMenu)
			popup.addSeparator();

		OBSSceneItem sceneItem = ui->sources->Get(idx);
		obs_source_t *source = obs_sceneitem_get_source(sceneItem);
		uint32_t flags = obs_source_get_output_flags(source);
		bool isAsyncVideo = (flags & OBS_SOURCE_ASYNC_VIDEO) == OBS_SOURCE_ASYNC_VIDEO;
		bool hasAudio = (flags & OBS_SOURCE_AUDIO) == OBS_SOURCE_AUDIO;
		bool hasVideo = (flags & OBS_SOURCE_VIDEO) == OBS_SOURCE_VIDEO;

		colorMenu = new QMenu(QTStr("ChangeBG"));
		colorWidgetAction = new QWidgetAction(colorMenu);
		colorSelect = new ColorSelect(colorMenu);
		popup.addMenu(AddBackgroundColorMenu(colorMenu, colorWidgetAction, colorSelect, sceneItem));
		popup.addAction(renameSource);
		popup.addAction(ui->actionRemoveSource);
		popup.addSeparator();

		popup.addMenu(ui->orderMenu);

		if (hasVideo)
			popup.addMenu(ui->transformMenu);

		popup.addSeparator();

		if (hasAudio) {
			QAction *actionHideMixer =
				popup.addAction(QTStr("HideMixer"), this, &OBSBasic::ToggleHideMixer);
			actionHideMixer->setCheckable(true);
			actionHideMixer->setChecked(SourceMixerHidden(source));
			popup.addSeparator();
		}

		if (hasVideo) {
			QAction *resizeOutput = popup.addAction(QTStr("ResizeOutputSizeOfSource"), this,
								&OBSBasic::ResizeOutputSizeOfSource);

			int width = obs_source_get_width(source);
			int height = obs_source_get_height(source);

			resizeOutput->setEnabled(!obs_video_active());

			if (width < 32 || height < 32)
				resizeOutput->setEnabled(false);

			scaleFilteringMenu = new QMenu(QTStr("ScaleFiltering"));
			popup.addMenu(AddScaleFilteringMenu(scaleFilteringMenu, sceneItem));
			blendingModeMenu = new QMenu(QTStr("BlendingMode"));
			popup.addMenu(AddBlendingModeMenu(blendingModeMenu, sceneItem));
			blendingMethodMenu = new QMenu(QTStr("BlendingMethod"));
			popup.addMenu(AddBlendingMethodMenu(blendingMethodMenu, sceneItem));
			if (isAsyncVideo) {
				deinterlaceMenu = new QMenu(QTStr("Deinterlacing"));
				popup.addMenu(AddDeinterlacingMenu(deinterlaceMenu, source));
			}

			popup.addSeparator();

			popup.addMenu(CreateVisibilityTransitionMenu(true));
			popup.addMenu(CreateVisibilityTransitionMenu(false));
			popup.addSeparator();

			sourceProjector = new QMenu(QTStr("SourceProjector"));
			AddProjectorMenuMonitors(sourceProjector, this, &OBSBasic::OpenSourceProjector);
			popup.addMenu(sourceProjector);
			popup.addAction(QTStr("SourceWindow"), this, &OBSBasic::OpenSourceWindow);

			popup.addAction(QTStr("Screenshot.Source"), this, &OBSBasic::ScreenshotSelectedSource);
		}

		popup.addSeparator();

		if (flags & OBS_SOURCE_INTERACTION)
			popup.addAction(QTStr("Interact"), this, &OBSBasic::on_actionInteract_triggered);

		popup.addAction(QTStr("Filters"), this, [&]() { OpenFilters(); });
		QAction *action =
			popup.addAction(QTStr("Properties"), this, &OBSBasic::on_actionSourceProperties_triggered);
		action->setEnabled(obs_source_configurable(source));
	}

	popup.exec(QCursor::pos());
}

void OBSBasic::on_sources_customContextMenuRequested(const QPoint &pos)
{
	if (ui->scenes->count()) {
		QModelIndex idx = ui->sources->indexAt(pos);
		CreateSourcePopupMenu(idx.row(), false);
	}
}

static inline bool should_show_properties(obs_source_t *source, const char *id)
{
	if (!source)
		return false;
	if (strcmp(id, "group") == 0)
		return false;
	if (!obs_source_configurable(source))
		return false;

	uint32_t caps = obs_source_get_output_flags(source);
	if ((caps & OBS_SOURCE_CAP_DONT_SHOW_PROPERTIES) != 0)
		return false;

	return true;
}

void OBSBasic::AddSource(const char *id)
{
	if (id && *id) {
		OBSBasicSourceSelect sourceSelect(this, id, undo_s);
		sourceSelect.exec();
		if (should_show_properties(sourceSelect.newSource, id)) {
			CreatePropertiesWindow(sourceSelect.newSource);
		}
	}
}

QMenu *OBSBasic::CreateAddSourcePopupMenu()
{
	const char *unversioned_type;
	const char *type;
	bool foundValues = false;
	bool foundDeprecated = false;
	size_t idx = 0;

	QMenu *popup = new QMenu(QTStr("Add"), this);
	QMenu *deprecated = new QMenu(QTStr("Deprecated"), popup);

	auto getActionAfter = [](QMenu *menu, const QString &name) {
		QList<QAction *> actions = menu->actions();

		for (QAction *menuAction : actions) {
			if (menuAction->text().compare(name, Qt::CaseInsensitive) >= 0)
				return menuAction;
		}

		return (QAction *)nullptr;
	};

	auto addSource = [this, getActionAfter](QMenu *popup, const char *type, const char *name) {
		QString qname = QT_UTF8(name);
		QAction *popupItem = new QAction(qname, this);
		connect(popupItem, &QAction::triggered, [this, type]() { AddSource(type); });

		QIcon icon;

		if (strcmp(type, "scene") == 0)
			icon = GetSceneIcon();
		else
			icon = GetSourceIcon(type);

		popupItem->setIcon(icon);

		QAction *after = getActionAfter(popup, qname);
		popup->insertAction(after, popupItem);
	};

	while (obs_enum_input_types2(idx++, &type, &unversioned_type)) {
		const char *name = obs_source_get_display_name(type);
		uint32_t caps = obs_get_source_output_flags(type);

		if ((caps & OBS_SOURCE_CAP_DISABLED) != 0)
			continue;

		if ((caps & OBS_SOURCE_DEPRECATED) == 0) {
			addSource(popup, unversioned_type, name);
		} else {
			addSource(deprecated, unversioned_type, name);
			foundDeprecated = true;
		}
		foundValues = true;
	}

	addSource(popup, "scene", Str("Basic.Scene"));

	popup->addSeparator();
	QAction *addGroup = new QAction(QTStr("Group"), this);
	addGroup->setIcon(GetGroupIcon());
	connect(addGroup, &QAction::triggered, [this]() { AddSource("group"); });
	popup->addAction(addGroup);

	if (!foundDeprecated) {
		delete deprecated;
		deprecated = nullptr;
	}

	if (!foundValues) {
		delete popup;
		popup = nullptr;

	} else if (foundDeprecated) {
		popup->addSeparator();
		popup->addMenu(deprecated);
	}

	return popup;
}

void OBSBasic::AddSourcePopupMenu(const QPoint &pos)
{
	if (!GetCurrentScene()) {
		// Tell the user he needs a scene first (help beginners).
		OBSMessageBox::information(this, QTStr("Basic.Main.AddSourceHelp.Title"),
					   QTStr("Basic.Main.AddSourceHelp.Text"));
		return;
	}

	QScopedPointer<QMenu> popup(CreateAddSourcePopupMenu());
	if (popup)
		popup->exec(pos);
}

void OBSBasic::on_actionAddSource_triggered()
{
	AddSourcePopupMenu(QCursor::pos());
}

static bool remove_items(obs_scene_t *, obs_sceneitem_t *item, void *param)
{
	vector<OBSSceneItem> &items = *reinterpret_cast<vector<OBSSceneItem> *>(param);

	if (obs_sceneitem_selected(item)) {
		items.emplace_back(item);
	} else if (obs_sceneitem_is_group(item)) {
		obs_sceneitem_group_enum_items(item, remove_items, &items);
	}
	return true;
};

void OBSBasic::on_actionRemoveSource_triggered()
{
	vector<OBSSceneItem> items;
	OBSScene scene = GetCurrentScene();
	obs_source_t *scene_source = obs_scene_get_source(scene);

	obs_scene_enum_items(scene, remove_items, &items);

	if (!items.size())
		return;

	/* ------------------------------------- */
	/* confirm action with user              */

	bool confirmed = false;

	if (items.size() > 1) {
		QString text = QTStr("ConfirmRemove.TextMultiple").arg(QString::number(items.size()));

		QMessageBox remove_items(this);
		remove_items.setText(text);
		QPushButton *Yes = remove_items.addButton(QTStr("Yes"), QMessageBox::YesRole);
		remove_items.setDefaultButton(Yes);
		remove_items.addButton(QTStr("No"), QMessageBox::NoRole);
		remove_items.setIcon(QMessageBox::Question);
		remove_items.setWindowTitle(QTStr("ConfirmRemove.Title"));
		remove_items.exec();

		confirmed = Yes == remove_items.clickedButton();
	} else {
		OBSSceneItem &item = items[0];
		obs_source_t *source = obs_sceneitem_get_source(item);
		if (source && QueryRemoveSource(source))
			confirmed = true;
	}
	if (!confirmed)
		return;

	/* ----------------------------------------------- */
	/* save undo data                                  */

	OBSData undo_data = BackupScene(scene_source);

	/* ----------------------------------------------- */
	/* remove items                                    */

	for (auto &item : items)
		obs_sceneitem_remove(item);

	/* ----------------------------------------------- */
	/* save redo data                                  */

	OBSData redo_data = BackupScene(scene_source);

	/* ----------------------------------------------- */
	/* add undo/redo action                            */

	QString action_name;
	if (items.size() > 1) {
		action_name = QTStr("Undo.Sources.Multi").arg(QString::number(items.size()));
	} else {
		QString str = QTStr("Undo.Delete");
		action_name = str.arg(obs_source_get_name(obs_sceneitem_get_source(items[0])));
	}

	CreateSceneUndoRedoAction(action_name, undo_data, redo_data);
}

void OBSBasic::on_actionInteract_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_get_source(item);

	if (source)
		CreateInteractionWindow(source);
}

void OBSBasic::on_actionSourceProperties_triggered()
{
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_get_source(item);

	if (source)
		CreatePropertiesWindow(source);
}

void OBSBasic::on_actionSourceUp_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_UP, QTStr("Undo.MoveUp"));
}

void OBSBasic::on_actionSourceDown_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_DOWN, QTStr("Undo.MoveDown"));
}

void OBSBasic::on_actionMoveUp_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_UP, QTStr("Undo.MoveUp"));
}

void OBSBasic::on_actionMoveDown_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_DOWN, QTStr("Undo.MoveDown"));
}

void OBSBasic::on_actionMoveToTop_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_TOP, QTStr("Undo.MoveToTop"));
}

void OBSBasic::on_actionMoveToBottom_triggered()
{
	MoveSceneItem(OBS_ORDER_MOVE_BOTTOM, QTStr("Undo.MoveToBottom"));
}

void OBSBasic::OpenFilters(OBSSource source)
{
	if (source == nullptr) {
		OBSSceneItem item = GetCurrentSceneItem();
		source = obs_sceneitem_get_source(item);
	}
	CreateFiltersWindow(source);
}

void OBSBasic::OpenProperties(OBSSource source)
{
	if (source == nullptr) {
		OBSSceneItem item = GetCurrentSceneItem();
		source = obs_sceneitem_get_source(item);
	}
	CreatePropertiesWindow(source);
}

void OBSBasic::OpenInteraction(OBSSource source)
{
	if (source == nullptr) {
		OBSSceneItem item = GetCurrentSceneItem();
		source = obs_sceneitem_get_source(item);
	}
	CreateInteractionWindow(source);
}

void OBSBasic::OpenEditTransform(OBSSceneItem item)
{
	if (!item)
		item = GetCurrentSceneItem();
	if (!item)
		return;
	CreateEditTransformWindow(item);
}

int OBSBasic::GetTopSelectedSourceItem()
{
	QModelIndexList selectedItems = ui->sources->selectionModel()->selectedIndexes();
	return selectedItems.count() ? selectedItems[0].row() : -1;
}

QModelIndexList OBSBasic::GetAllSelectedSourceItems()
{
	return ui->sources->selectionModel()->selectedIndexes();
}

void OBSBasic::on_actionEditTransform_triggered()
{
	const auto item = GetCurrentSceneItem();
	if (!item)
		return;
	CreateEditTransformWindow(item);
}

void undo_redo(const std::string &data)
{
	OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
	OBSSourceAutoRelease source = obs_get_source_by_uuid(obs_data_get_string(dat, "scene_uuid"));
	reinterpret_cast<OBSBasic *>(App()->GetMainWindow())->SetCurrentScene(source.Get(), true);

	obs_scene_load_transform_states(data.c_str());
}

static void GetItemBox(obs_sceneitem_t *item, vec3 &tl, vec3 &br)
{
	matrix4 boxTransform;
	obs_sceneitem_get_box_transform(item, &boxTransform);

	vec3_set(&tl, M_INFINITE, M_INFINITE, 0.0f);
	vec3_set(&br, -M_INFINITE, -M_INFINITE, 0.0f);

	auto GetMinPos = [&](float x, float y) {
		vec3 pos;
		vec3_set(&pos, x, y, 0.0f);
		vec3_transform(&pos, &pos, &boxTransform);
		vec3_min(&tl, &tl, &pos);
		vec3_max(&br, &br, &pos);
	};

	GetMinPos(0.0f, 0.0f);
	GetMinPos(1.0f, 0.0f);
	GetMinPos(0.0f, 1.0f);
	GetMinPos(1.0f, 1.0f);
}

static vec3 GetItemTL(obs_sceneitem_t *item)
{
	vec3 tl, br;
	GetItemBox(item, tl, br);
	return tl;
}

static void SetItemTL(obs_sceneitem_t *item, const vec3 &tl)
{
	vec3 newTL;
	vec2 pos;

	obs_sceneitem_get_pos(item, &pos);
	newTL = GetItemTL(item);
	pos.x += tl.x - newTL.x;
	pos.y += tl.y - newTL.y;
	obs_sceneitem_set_pos(item, &pos);
}

static bool RotateSelectedSources(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, RotateSelectedSources, param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	float rot = *reinterpret_cast<float *>(param);

	vec3 tl = GetItemTL(item);

	rot += obs_sceneitem_get_rot(item);
	if (rot >= 360.0f)
		rot -= 360.0f;
	else if (rot <= -360.0f)
		rot += 360.0f;
	obs_sceneitem_set_rot(item, rot);

	obs_sceneitem_force_update_transform(item);

	SetItemTL(item, tl);

	return true;
};

void OBSBasic::on_actionRotate90CW_triggered()
{
	float f90CW = 90.0f;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), RotateSelectedSources, &f90CW);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionRotate90CCW_triggered()
{
	float f90CCW = -90.0f;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), RotateSelectedSources, &f90CCW);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionRotate180_triggered()
{
	float f180 = 180.0f;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), RotateSelectedSources, &f180);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Rotate").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

static bool MultiplySelectedItemScale(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	vec2 &mul = *reinterpret_cast<vec2 *>(param);

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, MultiplySelectedItemScale, param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	vec3 tl = GetItemTL(item);

	vec2 scale;
	obs_sceneitem_get_scale(item, &scale);
	vec2_mul(&scale, &scale, &mul);
	obs_sceneitem_set_scale(item, &scale);

	obs_sceneitem_force_update_transform(item);

	SetItemTL(item, tl);

	return true;
}

void OBSBasic::on_actionFlipHorizontal_triggered()
{
	vec2 scale;
	vec2_set(&scale, -1.0f, 1.0f);
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), MultiplySelectedItemScale, &scale);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.HFlip").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionFlipVertical_triggered()
{
	vec2 scale;
	vec2_set(&scale, 1.0f, -1.0f);
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), MultiplySelectedItemScale, &scale);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.VFlip").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

static bool CenterAlignSelectedItems(obs_scene_t * /* scene */, obs_sceneitem_t *item, void *param)
{
	obs_bounds_type boundsType = *reinterpret_cast<obs_bounds_type *>(param);

	if (obs_sceneitem_is_group(item))
		obs_sceneitem_group_enum_items(item, CenterAlignSelectedItems, param);
	if (!obs_sceneitem_selected(item))
		return true;
	if (obs_sceneitem_locked(item))
		return true;

	obs_video_info ovi;
	obs_get_video_info(&ovi);

	obs_transform_info itemInfo;
	vec2_set(&itemInfo.pos, 0.0f, 0.0f);
	vec2_set(&itemInfo.scale, 1.0f, 1.0f);
	itemInfo.alignment = OBS_ALIGN_LEFT | OBS_ALIGN_TOP;
	itemInfo.rot = 0.0f;

	vec2_set(&itemInfo.bounds, float(ovi.base_width), float(ovi.base_height));
	itemInfo.bounds_type = boundsType;
	itemInfo.bounds_alignment = OBS_ALIGN_CENTER;
	itemInfo.crop_to_bounds = obs_sceneitem_get_bounds_crop(item);

	obs_sceneitem_set_info2(item, &itemInfo);

	return true;
}

void OBSBasic::on_actionFitToScreen_triggered()
{
	obs_bounds_type boundsType = OBS_BOUNDS_SCALE_INNER;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), CenterAlignSelectedItems, &boundsType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.FitToScreen").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionStretchToScreen_triggered()
{
	obs_bounds_type boundsType = OBS_BOUNDS_STRETCH;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	obs_scene_enum_items(GetCurrentScene(), CenterAlignSelectedItems, &boundsType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(QTStr("Undo.Transform.StretchToScreen")
				  .arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
			  undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::CenterSelectedSceneItems(const CenterType &centerType)
{
	QModelIndexList selectedItems = GetAllSelectedSourceItems();

	if (!selectedItems.count())
		return;

	vector<OBSSceneItem> items;

	// Filter out items that have no size
	for (int x = 0; x < selectedItems.count(); x++) {
		OBSSceneItem item = ui->sources->Get(selectedItems[x].row());
		obs_transform_info oti;
		obs_sceneitem_get_info2(item, &oti);

		obs_source_t *source = obs_sceneitem_get_source(item);
		float width = float(obs_source_get_width(source)) * oti.scale.x;
		float height = float(obs_source_get_height(source)) * oti.scale.y;

		if (width == 0.0f || height == 0.0f)
			continue;

		items.emplace_back(item);
	}

	if (!items.size())
		return;

	// Get center x, y coordinates of items
	vec3 center;

	float top = M_INFINITE;
	float left = M_INFINITE;
	float right = 0.0f;
	float bottom = 0.0f;

	for (auto &item : items) {
		vec3 tl, br;

		GetItemBox(item, tl, br);

		left = std::min(tl.x, left);
		top = std::min(tl.y, top);
		right = std::max(br.x, right);
		bottom = std::max(br.y, bottom);
	}

	center.x = (right + left) / 2.0f;
	center.y = (top + bottom) / 2.0f;
	center.z = 0.0f;

	// Get coordinates of screen center
	obs_video_info ovi;
	obs_get_video_info(&ovi);

	vec3 screenCenter;
	vec3_set(&screenCenter, float(ovi.base_width), float(ovi.base_height), 0.0f);

	vec3_mulf(&screenCenter, &screenCenter, 0.5f);

	// Calculate difference between screen center and item center
	vec3 offset;
	vec3_sub(&offset, &screenCenter, &center);

	// Shift items by offset
	for (auto &item : items) {
		vec3 tl, br;

		GetItemBox(item, tl, br);

		vec3_add(&tl, &tl, &offset);

		vec3 itemTL = GetItemTL(item);

		if (centerType == CenterType::Vertical)
			tl.x = itemTL.x;
		else if (centerType == CenterType::Horizontal)
			tl.y = itemTL.y;

		SetItemTL(item, tl);
	}
}

void OBSBasic::on_actionCenterToScreen_triggered()
{
	CenterType centerType = CenterType::Scene;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	CenterSelectedSceneItems(centerType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.Center").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionVerticalCenter_triggered()
{
	CenterType centerType = CenterType::Vertical;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	CenterSelectedSceneItems(centerType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.VCenter").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_actionHorizontalCenter_triggered()
{
	CenterType centerType = CenterType::Horizontal;
	OBSDataAutoRelease wrapper = obs_scene_save_transform_states(GetCurrentScene(), false);
	CenterSelectedSceneItems(centerType);
	OBSDataAutoRelease rwrapper = obs_scene_save_transform_states(GetCurrentScene(), false);

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	undo_s.add_action(
		QTStr("Undo.Transform.HCenter").arg(obs_source_get_name(obs_scene_get_source(GetCurrentScene()))),
		undo_redo, undo_redo, undo_data, redo_data);
}

void OBSBasic::on_toggleSourceIcons_toggled(bool visible)
{
	ui->sources->SetIconsVisible(visible);
	if (advAudioWindow != nullptr)
		advAudioWindow->SetIconsVisible(visible);

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "ShowSourceIcons", visible);
}

void OBSBasic::on_sourcePropertiesButton_clicked()
{
	on_actionSourceProperties_triggered();
}

void OBSBasic::on_sourceFiltersButton_clicked()
{
	OpenFilters();
}

void OBSBasic::on_sourceInteractButton_clicked()
{
	on_actionInteract_triggered();
}
