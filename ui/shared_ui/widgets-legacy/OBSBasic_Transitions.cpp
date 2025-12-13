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

#include "OBSBasic.hpp"

#include <algorithm>

#include <components/MenuButton.hpp>
#include <dialogs/NameDialog.hpp>
#include <utility/display-helpers.hpp>
#include <utility/QuickTransition.hpp>

#include <qt-wrappers.hpp>
#include <slider-ignorewheel.hpp>

#include <QToolTip>
#include <QWidgetAction>

extern bool disable_3p_plugins;
extern bool safe_mode;

static inline QString MakeQuickTransitionText(QuickTransition *qt)
{
	QString name;

	if (!qt->fadeToBlack)
		name = QT_UTF8(obs_source_get_name(qt->source));
	else
		name = QTStr("FadeToBlack");

	if (!obs_transition_fixed(qt->source))
		name += QString(" (%1ms)").arg(QString::number(qt->duration));
	return name;
}

void OBSBasic::InitDefaultTransitions()
{
	std::vector<OBSSource> defaultTransitions;
	size_t idx = 0;
	const char *id;

	/* automatically add transitions that have no configuration (things
	 * such as cut/fade/etc) */
	while (obs_enum_transition_types(idx++, &id)) {
		if (!obs_is_source_configurable(id)) {
			const char *name = obs_source_get_display_name(id);

			OBSSourceAutoRelease tr = obs_source_create_private(id, name, NULL);
			InitTransition(tr);
			defaultTransitions.emplace_back(tr);

			if (strcmp(id, "fade_transition") == 0)
				fadeTransition = tr;
			else if (strcmp(id, "cut_transition") == 0)
				cutTransition = tr;
		}
	}

	for (OBSSource &tr : defaultTransitions) {
		std::string uuid = obs_source_get_uuid(tr);

		transitions.insert({uuid, OBSSource(tr)});
		transitionNameToUuids.insert({obs_source_get_name(tr), uuid});
		transitionUuids.push_back(uuid);

		emit TransitionAdded(QT_UTF8(obs_source_get_name(tr)), QString::fromStdString(uuid));
	}

	UpdateCurrentTransition(transitionUuids.back(), true);
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	DStr hotkeyId;
	QString hotkeyName;

	dstr_printf(hotkeyId, "OBSBasic.QuickTransition.%d", qt->id);
	hotkeyName = QTStr("QuickTransitions.HotkeyName").arg(MakeQuickTransitionText(qt));

	auto quickTransition = [](void *data, obs_hotkey_id, obs_hotkey_t *, bool pressed) {
		int id = (int)(uintptr_t)data;
		OBSBasic *main = OBSBasic::Get();

		if (pressed)
			QMetaObject::invokeMethod(main, "TriggerQuickTransition", Qt::QueuedConnection, Q_ARG(int, id));
	};

	qt->hotkey = obs_hotkey_register_frontend(hotkeyId->array, QT_TO_UTF8(hotkeyName), quickTransition,
						  (void *)(uintptr_t)qt->id);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);

	if (qt && previewProgramMode) {
		OBSScene scene = GetCurrentScene();
		obs_source_t *source = obs_scene_get_source(scene);

		if (GetCurrentTransition() != qt->source) {
			OverrideTransition(qt->source);
			overridingTransition = true;
		}

		TransitionToScene(source, false, true, qt->duration, qt->fadeToBlack);
	}
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	obs_hotkey_unregister(qt->hotkey);
}

void OBSBasic::InitTransition(obs_source_t *transition)
{
	auto onTransitionStop = [](void *data, calldata_t *) {
		OBSBasic *window = (OBSBasic *)data;
		QMetaObject::invokeMethod(window, "TransitionStopped", Qt::QueuedConnection);
	};

	auto onTransitionFullStop = [](void *data, calldata_t *) {
		OBSBasic *window = (OBSBasic *)data;
		QMetaObject::invokeMethod(window, "TransitionFullyStopped", Qt::QueuedConnection);
	};

	signal_handler_t *handler = obs_source_get_signal_handler(transition);
	signal_handler_connect(handler, "transition_video_stop", onTransitionStop, this);
	signal_handler_connect(handler, "transition_stop", onTransitionFullStop, this);
}

static inline OBSSource GetTransitionComboItem(QComboBox *combo, int idx)
{
	return combo->itemData(idx).value<OBSSource>();
}

void OBSBasic::CreateDefaultQuickTransitions()
{
	/* non-configurable transitions are always available, so add them
	 * to the "default quick transitions" list */
	quickTransitions.emplace_back(cutTransition, 300, quickTransitionIdCounter++);
	quickTransitions.emplace_back(fadeTransition, 300, quickTransitionIdCounter++);
	quickTransitions.emplace_back(fadeTransition, 300, quickTransitionIdCounter++, true);
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *array)
{
	size_t count = obs_data_array_count(array);

	quickTransitionIdCounter = 1;

	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease data = obs_data_array_item(array, i);
		OBSDataArrayAutoRelease hotkeys = obs_data_get_array(data, "hotkeys");
		const char *name = obs_data_get_string(data, "name");
		int duration = obs_data_get_int(data, "duration");
		int id = obs_data_get_int(data, "id");
		bool toBlack = obs_data_get_bool(data, "fade_to_black");

		if (id) {
			obs_source_t *source = FindTransition(name);
			if (source) {
				quickTransitions.emplace_back(source, duration, id, toBlack);

				if (quickTransitionIdCounter <= id)
					quickTransitionIdCounter = id + 1;

				int idx = (int)quickTransitions.size() - 1;
				AddQuickTransitionHotkey(&quickTransitions[idx]);
				obs_hotkey_load(quickTransitions[idx].hotkey, hotkeys);
			}
		}
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *array = obs_data_array_create();

	for (QuickTransition &qt : quickTransitions) {
		OBSDataAutoRelease data = obs_data_create();
		OBSDataArrayAutoRelease hotkeys = obs_hotkey_save(qt.hotkey);

		obs_data_set_string(data, "name", obs_source_get_name(qt.source));
		obs_data_set_int(data, "duration", qt.duration);
		obs_data_set_array(data, "hotkeys", hotkeys);
		obs_data_set_int(data, "id", qt.id);
		obs_data_set_bool(data, "fade_to_black", qt.fadeToBlack);

		obs_data_array_push_back(array, data);
	}

	return array;
}

obs_source_t *OBSBasic::FindTransition(const char *name)
{
	auto nameToUuid = transitionNameToUuids.find(name);

	if (nameToUuid != transitionNameToUuids.end()) {
		auto transition = transitions.find(nameToUuid->second);

		if (transition == transitions.end())
			return nullptr;

		return transition->second;
	}

	return nullptr;
}

void OBSBasic::TransitionToScene(OBSScene scene, bool force)
{
	obs_source_t *source = obs_scene_get_source(scene);
	TransitionToScene(source, force);
}

void OBSBasic::TransitionStopped()
{
	if (swapScenesMode) {
		OBSSource scene = OBSGetStrongRef(swapScene);
		if (scene)
			SetCurrentScene(scene);
	}

	EnableTransitionWidgets(true);
	UpdatePreviewProgramIndicators();

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
	OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);

	swapScene = nullptr;
}

void OBSBasic::OverrideTransition(OBSSource transition)
{
	OBSSourceAutoRelease oldTransition = obs_get_output_source(0);

	if (transition != oldTransition) {
		obs_transition_swap_begin(transition, oldTransition);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	}
}

void OBSBasic::TransitionFullyStopped()
{
	if (overridingTransition) {
		OverrideTransition(GetCurrentTransition());
		overridingTransition = false;
	}
}

void OBSBasic::TransitionToScene(OBSSource source, bool force, bool quickTransition, int quickDuration, bool black,
				 bool manual)
{
	obs_scene_t *scene = obs_scene_from_source(source);
	bool usingPreviewProgram = IsPreviewProgramMode();
	if (!scene)
		return;

	if (usingPreviewProgram) {
		if (!tBarActive)
			lastProgramScene = programScene;
		programScene = OBSGetWeakRef(source);

		if (!force && !black) {
			OBSSource lastScene = OBSGetStrongRef(lastProgramScene);

			if (!sceneDuplicationMode && lastScene == source)
				return;

			if (swapScenesMode && lastScene && lastScene != GetCurrentSceneSource())
				swapScene = lastProgramScene;
		}
	}

	if (usingPreviewProgram && sceneDuplicationMode) {
		scene = obs_scene_duplicate(scene, obs_source_get_name(obs_scene_get_source(scene)),
					    editPropertiesMode ? OBS_SCENE_DUP_PRIVATE_COPY
							       : OBS_SCENE_DUP_PRIVATE_REFS);
		source = obs_scene_get_source(scene);
	}

	OBSSourceAutoRelease transition = obs_get_output_source(0);
	if (!transition) {
		if (usingPreviewProgram && sceneDuplicationMode)
			obs_scene_release(scene);
		return;
	}

	float t = obs_transition_get_time(transition);
	bool stillTransitioning = t < 1.0f && t > 0.0f;

	// If actively transitioning, block new transitions from starting
	if (usingPreviewProgram && stillTransitioning)
		goto cleanup;

	if (usingPreviewProgram) {
		if (!black && !manual) {
			const char *sceneName = obs_source_get_name(source);
			blog(LOG_INFO, "User switched Program to scene '%s'", sceneName);

		} else if (black && !prevFTBSource) {
			OBSSourceAutoRelease target = obs_transition_get_active_source(transition);
			const char *sceneName = obs_source_get_name(target);
			blog(LOG_INFO, "User faded from scene '%s' to black", sceneName);

		} else if (black && prevFTBSource) {
			const char *sceneName = obs_source_get_name(prevFTBSource);
			blog(LOG_INFO, "User faded from black to scene '%s'", sceneName);

		} else if (manual) {
			const char *sceneName = obs_source_get_name(source);
			blog(LOG_INFO, "User started manual transition to scene '%s'", sceneName);
		}
	}

	if (force) {
		obs_transition_set(transition, source);
		OnEvent(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	} else {
		int duration = GetTransitionDuration();

		/* check for scene override */
		OBSSource trOverride = GetOverrideTransition(source);

		if (trOverride && !overridingTransition && !quickTransition) {
			transition = std::move(trOverride);
			duration = GetOverrideTransitionDuration(source);
			OverrideTransition(transition.Get());
			overridingTransition = true;
		}

		if (black && !prevFTBSource) {
			prevFTBSource = source;
			source = nullptr;
		} else if (black && prevFTBSource) {
			source = prevFTBSource;
			prevFTBSource = nullptr;
		} else if (!black) {
			prevFTBSource = nullptr;
		}

		if (quickTransition)
			duration = quickDuration;

		enum obs_transition_mode mode = manual ? OBS_TRANSITION_MODE_MANUAL : OBS_TRANSITION_MODE_AUTO;

		EnableTransitionWidgets(false);

		bool success = obs_transition_start(transition, mode, duration, source);

		if (!success)
			TransitionFullyStopped();
	}

cleanup:
	if (usingPreviewProgram && sceneDuplicationMode)
		obs_scene_release(scene);
}

static inline void SetComboTransition(QComboBox *combo, obs_source_t *tr)
{
	int idx = combo->findData(QVariant::fromValue<OBSSource>(tr));
	if (idx != -1) {
		combo->blockSignals(true);
		combo->setCurrentIndex(idx);
		combo->blockSignals(false);
	}
}

void OBSBasic::SetTransition(OBSSource transition)
{
	OBSSourceAutoRelease oldTransition = obs_get_output_source(0);

	if (oldTransition && transition) {
		std::string uuid = obs_source_get_uuid(transition);
		obs_transition_swap_begin(transition, oldTransition);
		if (currentTransitionUuid != uuid)
			UpdateCurrentTransition(uuid, false);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	} else {
		obs_set_output_source(0, transition);
	}

	bool fixed = transition ? obs_transition_fixed(transition) : false;
	ui->transitionDurationLabel->setVisible(!fixed);
	ui->transitionDuration->setVisible(!fixed);

	bool configurable = transition ? obs_source_configurable(transition) : false;
	ui->transitionRemove->setEnabled(configurable);
	ui->transitionProps->setEnabled(configurable);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

OBSSource OBSBasic::GetCurrentTransition()
{
	auto transition = transitions.find(currentTransitionUuid);

	if (transition == transitions.end())
		return nullptr;

	return transition->second;
}

void OBSBasic::AddTransition(const char *id)
{
	string name;
	QString placeHolderText = QT_UTF8(obs_source_get_display_name(id));
	QString format = placeHolderText + " (%1)";
	obs_source_t *source = nullptr;
	int i = 1;

	while ((FindTransition(QT_TO_UTF8(placeHolderText)))) {
		placeHolderText = format.arg(++i);
	}

	bool accepted = NameDialog::AskForName(this, QTStr("TransitionNameDlg.Title"), QTStr("TransitionNameDlg.Text"),
					       name, placeHolderText);

	if (accepted) {
		std::string uuid;

		if (name.empty()) {
			OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
			AddTransition(id);
			return;
		}

		source = FindTransition(name.c_str());
		if (source) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));

			AddTransition(id);
			return;
		}

		source = obs_source_create_private(id, name.c_str(), NULL);
		InitTransition(source);

		uuid = obs_source_get_uuid(source);
		transitions.insert({uuid, source});
		transitionNameToUuids.insert({name, uuid});
		transitionUuids.push_back(uuid);

		emit TransitionAdded(QString::fromStdString(name), QString::fromStdString(uuid));

		UpdateCurrentTransition(uuid, true);

		CreatePropertiesWindow(source);
		obs_source_release(source);

		OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);

		ClearQuickTransitionWidgets();
		RefreshQuickTransitions();
	}
}

void OBSBasic::on_transitionAdd_clicked()
{
	bool foundConfigurableTransitions = false;
	QMenu menu(this);
	size_t idx = 0;
	const char *id;

	while (obs_enum_transition_types(idx++, &id)) {
		if (obs_is_source_configurable(id)) {
			const char *name = obs_source_get_display_name(id);
			QAction *action = new QAction(name, this);

			connect(action, &QAction::triggered, [this, id]() { AddTransition(id); });

			menu.addAction(action);
			foundConfigurableTransitions = true;
		}
	}

	if (foundConfigurableTransitions)
		menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	auto transitionIterator = transitions.find(currentTransitionUuid);
	OBSSource tr;
	const char *name;

	if (transitionIterator == transitions.end())
		return;

	tr = transitionIterator->second;

	if (!tr || !obs_source_configurable(tr) || !QueryRemoveSource(tr))
		return;

	for (size_t i = quickTransitions.size(); i > 0; i--) {
		QuickTransition &qt = quickTransitions[i - 1];
		if (qt.source == tr) {
			if (qt.button)
				qt.button->deleteLater();
			RemoveQuickTransitionHotkey(&qt);
			quickTransitions.erase(quickTransitions.begin() + i - 1);
		}
	}

	name = obs_source_get_name(tr);
	if (name)
		transitionNameToUuids.erase(std::string(name));

	transitionUuids.erase(std::find(transitionUuids.begin(), transitionUuids.end(), currentTransitionUuid));
	transitions.erase(currentTransitionUuid);
	emit TransitionRemoved(QString::fromStdString(currentTransitionUuid));

	UpdateCurrentTransition(transitionUuids.back(), true);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);

	ClearQuickTransitionWidgets();
	RefreshQuickTransitions();
}

void OBSBasic::RenameTransition(OBSSource transition)
{
	std::string name;
	std::string oldName = obs_source_get_name(transition);
	std::string uuid = obs_source_get_uuid(transition);
	QString placeHolderText = QString::fromStdString(oldName);
	obs_source_t *source = nullptr;

	bool accepted = NameDialog::AskForName(this, QTStr("TransitionNameDlg.Title"), QTStr("TransitionNameDlg.Text"),
					       name, placeHolderText);

	if (!accepted)
		return;
	if (name.empty()) {
		OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
		RenameTransition(transition);
		return;
	}

	source = FindTransition(name.c_str());
	if (source) {
		OBSMessageBox::warning(this, QTStr("NameExists.Title"), QTStr("NameExists.Text"));

		RenameTransition(transition);
		return;
	}

	obs_source_set_name(transition, name.c_str());

	if (transitionNameToUuids.find(oldName) == transitionNameToUuids.end())
		return;

	transitionNameToUuids.erase(oldName);
	transitionNameToUuids.insert({name, uuid});

	emit TransitionRenamed(QString::fromStdString(uuid), QString::fromStdString(name));

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);

	ClearQuickTransitionWidgets();
	RefreshQuickTransitions();
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource source = GetCurrentTransition();

	if (!obs_source_configurable(source))
		return;

	auto properties = [&]() {
		CreatePropertiesWindow(source);
	};

	QMenu menu(this);

	QAction *action = new QAction(QTStr("Rename"), &menu);
	connect(action, &QAction::triggered, [this, source]() { RenameTransition(source); });
	menu.addAction(action);

	action = new QAction(QTStr("Properties"), &menu);
	connect(action, &QAction::triggered, properties);
	menu.addAction(action);

	menu.exec(QCursor::pos());
}

QuickTransition *OBSBasic::GetQuickTransition(int id)
{
	for (QuickTransition &qt : quickTransitions) {
		if (qt.id == id)
			return &qt;
	}

	return nullptr;
}

int OBSBasic::GetQuickTransitionIdx(int id)
{
	for (int idx = 0; idx < (int)quickTransitions.size(); idx++) {
		QuickTransition &qt = quickTransitions[idx];

		if (qt.id == id)
			return idx;
	}

	return -1;
}

void OBSBasic::SetCurrentScene(obs_scene_t *scene, bool force)
{
	obs_source_t *source = obs_scene_get_source(scene);
	SetCurrentScene(source, force);
}

void OBSBasic::SetCurrentScene(OBSSource scene, bool force)
{
	if (!IsPreviewProgramMode()) {
		TransitionToScene(scene, force);
	} else {
		OBSSource actualLastScene = OBSGetStrongRef(lastScene);
		if (actualLastScene != scene) {
			if (scene)
				obs_source_inc_showing(scene);
			if (actualLastScene)
				obs_source_dec_showing(actualLastScene);
			lastScene = OBSGetWeakRef(scene);
		}
	}

	if (obs_scene_get_source(GetCurrentScene()) != scene) {
		for (int i = 0; i < ui->scenes->count(); i++) {
			QListWidgetItem *item = ui->scenes->item(i);
			OBSScene itemScene = GetOBSRef<OBSScene>(item);
			obs_source_t *source = obs_scene_get_source(itemScene);

			if (source == scene) {
				ui->scenes->blockSignals(true);
				currentScene = itemScene.Get();
				ui->scenes->setCurrentItem(item);
				ui->scenes->blockSignals(false);

				if (vcamEnabled && vcamConfig.type == VCamOutputType::PreviewOutput)
					outputHandler->UpdateVirtualCamOutputSource();

				OnEvent(OBS_FRONTEND_EVENT_PREVIEW_SCENE_CHANGED);
				break;
			}
		}
	}

	UpdateContextBar(true);
	UpdatePreviewProgramIndicators();

	if (scene) {
		bool userSwitched = (!force && !disableSaving);
		blog(LOG_INFO, "%s to scene '%s'", userSwitched ? "User switched" : "Switched",
		     obs_source_get_name(scene));
	}
}

void OBSBasic::TransitionClicked()
{
	if (previewProgramMode)
		TransitionToScene(GetCurrentScene());
}

#define T_BAR_PRECISION 1024
#define T_BAR_PRECISION_F ((float)T_BAR_PRECISION)
#define T_BAR_CLAMP (T_BAR_PRECISION / 10)

void OBSBasic::TBarReleased()
{
	int val = tBar->value();

	OBSSourceAutoRelease transition = obs_get_output_source(0);

	if ((tBar->maximum() - val) <= T_BAR_CLAMP) {
		obs_transition_set_manual_time(transition, 1.0f);
		tBar->blockSignals(true);
		tBar->setValue(0);
		tBar->blockSignals(false);
		tBarActive = false;
		EnableTransitionWidgets(true);

		OBSSourceAutoRelease target = obs_transition_get_active_source(transition);
		const char *sceneName = obs_source_get_name(target);
		blog(LOG_INFO, "Manual transition to scene '%s' finished", sceneName);
	} else if (val <= T_BAR_CLAMP) {
		obs_transition_set_manual_time(transition, 0.0f);
		TransitionFullyStopped();
		tBar->blockSignals(true);
		tBar->setValue(0);
		tBar->blockSignals(false);
		tBarActive = false;
		EnableTransitionWidgets(true);
		programScene = lastProgramScene;
		blog(LOG_INFO, "Manual transition cancelled");
	}

	tBar->clearFocus();
}

static bool ValidTBarTransition(OBSSource transition)
{
	if (!transition)
		return false;

	QString id = QT_UTF8(obs_source_get_id(transition));

	if (id == "cut_transition" || id == "obs_stinger_transition")
		return false;

	return true;
}

void OBSBasic::TBarChanged(int value)
{
	OBSSourceAutoRelease transition = obs_get_output_source(0);

	if (!tBarActive) {
		OBSSource sceneSource = GetCurrentSceneSource();
		OBSSource tBarTr = GetOverrideTransition(sceneSource);

		if (!ValidTBarTransition(tBarTr)) {
			tBarTr = GetCurrentTransition();

			if (!ValidTBarTransition(tBarTr))
				tBarTr = FindTransition(obs_source_get_display_name("fade_transition"));

			OverrideTransition(tBarTr);
			overridingTransition = true;

			transition = std::move(tBarTr);
		}

		obs_transition_set_manual_torque(transition, 8.0f, 0.05f);
		TransitionToScene(sceneSource, false, false, false, 0, true);
		tBarActive = true;
	}

	obs_transition_set_manual_time(transition, (float)value / T_BAR_PRECISION_F);

	OnEvent(OBS_FRONTEND_EVENT_TBAR_VALUE_CHANGED);
}

int OBSBasic::GetTbarPosition()
{
	return tBar->value();
}

static inline void ResetQuickTransitionText(QuickTransition *qt)
{
	qt->button->setText(MakeQuickTransitionText(qt));
}

QMenu *OBSBasic::CreatePerSceneTransitionMenu()
{
	OBSSource scene = GetCurrentSceneSource();
	QMenu *menu = new QMenu(QTStr("TransitionOverride"));

	OBSDataAutoRelease data = obs_source_get_private_settings(scene);

	obs_data_set_default_int(data, "transition_duration", 300);

	const char *curTransition = obs_data_get_string(data, "transition");
	int curDuration = (int)obs_data_get_int(data, "transition_duration");

	QSpinBox *duration = new QSpinBox(menu);
	duration->setMinimum(50);
	duration->setSuffix(" ms");
	duration->setMaximum(20000);
	duration->setSingleStep(50);
	duration->setValue(curDuration);

	auto setTransition = [this](QAction *action) {
		std::string uuid = action->property("transition_uuid").toString().toStdString();
		OBSSource scene = GetCurrentSceneSource();
		OBSDataAutoRelease data = obs_source_get_private_settings(scene);
		auto transitionIter = transitions.find(uuid);
		OBSSource transition;

		if (uuid.empty()) {
			obs_data_set_string(data, "transition", "");
			return;
		}

		if (transitionIter == transitions.end())
			return;

		transition = transitionIter->second;

		if (transition) {
			const char *name = obs_source_get_name(transition);
			obs_data_set_string(data, "transition", name);
		}
	};

	auto setDuration = [this](int duration) {
		OBSSource scene = GetCurrentSceneSource();
		OBSDataAutoRelease data = obs_source_get_private_settings(scene);

		obs_data_set_int(data, "transition_duration", duration);
	};

	connect(duration, (void(QSpinBox::*)(int)) & QSpinBox::valueChanged, setDuration);

	auto addAction = [&](const std::string &uuid = "") {
		const char *name = "";
		QAction *action;

		if (!uuid.empty()) {
			auto transition = transitions.find(uuid);
			if (transition == transitions.end())
				return;

			name = obs_source_get_name(transition->second);
		}

		bool match = (name && strcmp(name, curTransition) == 0);

		if (!name || !*name)
			name = Str("None");

		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_uuid", QString::fromStdString(uuid));
		action->setCheckable(true);
		action->setChecked(match);

		connect(action, &QAction::triggered, std::bind(setTransition, action));
	};

	addAction();
	for (const auto &[uuid, transition] : transitions)
		addAction(uuid);

	QWidgetAction *durationAction = new QWidgetAction(menu);
	durationAction->setDefaultWidget(duration);

	menu->addSeparator();
	menu->addAction(durationAction);
	return menu;
}

void OBSBasic::ShowTransitionProperties()
{
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_get_transition(item, true);

	if (source)
		CreatePropertiesWindow(source);
}

void OBSBasic::HideTransitionProperties()
{
	OBSSceneItem item = GetCurrentSceneItem();
	OBSSource source = obs_sceneitem_get_transition(item, false);

	if (source)
		CreatePropertiesWindow(source);
}

void OBSBasic::PasteShowHideTransition(obs_sceneitem_t *item, bool show, obs_source_t *tr, int duration)
{
	int64_t sceneItemId = obs_sceneitem_get_id(item);
	std::string sceneUUID = obs_source_get_uuid(obs_scene_get_source(obs_sceneitem_get_scene(item)));

	auto undo_redo = [sceneUUID, sceneItemId, show](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(sceneUUID.c_str());
		obs_scene_t *scene = obs_scene_from_source(source);
		obs_sceneitem_t *i = obs_scene_find_sceneitem_by_id(scene, sceneItemId);
		if (i) {
			OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
			obs_sceneitem_transition_load(i, dat, show);
		}
	};

	OBSDataAutoRelease oldTransitionData = obs_sceneitem_transition_save(item, show);

	OBSSourceAutoRelease dup = obs_source_duplicate(tr, obs_source_get_name(tr), true);
	obs_sceneitem_set_transition(item, show, dup);
	obs_sceneitem_set_transition_duration(item, show, duration);

	OBSDataAutoRelease transitionData = obs_sceneitem_transition_save(item, show);

	std::string undo_data(obs_data_get_json(oldTransitionData));
	std::string redo_data(obs_data_get_json(transitionData));
	if (undo_data.compare(redo_data) == 0)
		return;

	QString text = show ? QTStr("Undo.ShowTransition") : QTStr("Undo.HideTransition");
	const char *name = obs_source_get_name(obs_sceneitem_get_source(item));
	undo_s.add_action(text.arg(name), undo_redo, undo_redo, undo_data, redo_data);
}

QMenu *OBSBasic::CreateVisibilityTransitionMenu(bool visible)
{
	OBSSceneItem si = GetCurrentSceneItem();

	QMenu *menu = new QMenu(QTStr(visible ? "ShowTransition" : "HideTransition"));
	QAction *action;

	OBSSource curTransition = obs_sceneitem_get_transition(si, visible);
	const char *curId = curTransition ? obs_source_get_id(curTransition) : nullptr;
	int curDuration = (int)obs_sceneitem_get_transition_duration(si, visible);

	if (curDuration <= 0)
		curDuration = obs_frontend_get_transition_duration();

	QSpinBox *duration = new QSpinBox(menu);
	duration->setMinimum(50);
	duration->setSuffix(" ms");
	duration->setMaximum(20000);
	duration->setSingleStep(50);
	duration->setValue(curDuration);

	auto setTransition = [this](QAction *action, bool visible) {
		OBSBasic *main = OBSBasic::Get();

		QString id = action->property("transition_id").toString();
		OBSSceneItem sceneItem = main->GetCurrentSceneItem();
		int64_t sceneItemId = obs_sceneitem_get_id(sceneItem);
		std::string sceneUUID = obs_source_get_uuid(obs_scene_get_source(obs_sceneitem_get_scene(sceneItem)));

		auto undo_redo = [sceneUUID, sceneItemId, visible](const std::string &data) {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(sceneUUID.c_str());
			obs_scene_t *scene = obs_scene_from_source(source);
			obs_sceneitem_t *i = obs_scene_find_sceneitem_by_id(scene, sceneItemId);
			if (i) {
				OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());
				obs_sceneitem_transition_load(i, dat, visible);
			}
		};
		OBSDataAutoRelease oldTransitionData = obs_sceneitem_transition_save(sceneItem, visible);
		if (id.isNull() || id.isEmpty()) {
			obs_sceneitem_set_transition(sceneItem, visible, nullptr);
			obs_sceneitem_set_transition_duration(sceneItem, visible, 0);
		} else {
			OBSSource tr = obs_sceneitem_get_transition(sceneItem, visible);

			if (!tr || strcmp(QT_TO_UTF8(id), obs_source_get_id(tr)) != 0) {
				QString name = QT_UTF8(obs_source_get_name(obs_sceneitem_get_source(sceneItem)));
				name += " ";
				name += QTStr(visible ? "ShowTransition" : "HideTransition");
				tr = obs_source_create_private(QT_TO_UTF8(id), QT_TO_UTF8(name), nullptr);
				obs_sceneitem_set_transition(sceneItem, visible, tr);
				obs_source_release(tr);

				int duration = (int)obs_sceneitem_get_transition_duration(sceneItem, visible);
				if (duration <= 0) {
					duration = obs_frontend_get_transition_duration();
					obs_sceneitem_set_transition_duration(sceneItem, visible, duration);
				}
			}
			if (obs_source_configurable(tr))
				CreatePropertiesWindow(tr);
		}
		OBSDataAutoRelease newTransitionData = obs_sceneitem_transition_save(sceneItem, visible);
		std::string undo_data(obs_data_get_json(oldTransitionData));
		std::string redo_data(obs_data_get_json(newTransitionData));
		if (undo_data.compare(redo_data) != 0)
			main->undo_s.add_action(QTStr(visible ? "Undo.ShowTransition" : "Undo.HideTransition")
							.arg(obs_source_get_name(obs_sceneitem_get_source(sceneItem))),
						undo_redo, undo_redo, undo_data, redo_data);
	};
	auto setDuration = [visible](int duration) {
		OBSBasic *main = OBSBasic::Get();

		OBSSceneItem item = main->GetCurrentSceneItem();
		obs_sceneitem_set_transition_duration(item, visible, duration);
	};
	connect(duration, (void(QSpinBox::*)(int)) & QSpinBox::valueChanged, setDuration);

	action = menu->addAction(QT_UTF8(Str("None")));
	action->setProperty("transition_id", QT_UTF8(""));
	action->setCheckable(true);
	action->setChecked(!curId);
	connect(action, &QAction::triggered, std::bind(setTransition, action, visible));
	size_t idx = 0;
	const char *id;
	while (obs_enum_transition_types(idx++, &id)) {
		const char *name = obs_source_get_display_name(id);
		const bool match = id && curId && strcmp(id, curId) == 0;
		action = menu->addAction(QT_UTF8(name));
		action->setProperty("transition_id", QT_UTF8(id));
		action->setCheckable(true);
		action->setChecked(match);
		connect(action, &QAction::triggered, std::bind(setTransition, action, visible));
	}

	QWidgetAction *durationAction = new QWidgetAction(menu);
	durationAction->setDefaultWidget(duration);

	menu->addSeparator();
	menu->addAction(durationAction);
	if (curId && obs_is_source_configurable(curId)) {
		menu->addSeparator();
		menu->addAction(QTStr("Properties"), this,
				visible ? &OBSBasic::ShowTransitionProperties : &OBSBasic::HideTransitionProperties);
	}

	auto copyTransition = [this](QAction *, bool visible) {
		OBSBasic *main = OBSBasic::Get();
		OBSSceneItem item = main->GetCurrentSceneItem();
		obs_source_t *tr = obs_sceneitem_get_transition(item, visible);
		int trDur = obs_sceneitem_get_transition_duration(item, visible);
		main->copySourceTransition = obs_source_get_weak_source(tr);
		main->copySourceTransitionDuration = trDur;
	};
	menu->addSeparator();
	action = menu->addAction(QT_UTF8(Str("Copy")));
	action->setEnabled(curId != nullptr);
	connect(action, &QAction::triggered, std::bind(copyTransition, action, visible));

	auto pasteTransition = [this](QAction *, bool show) {
		OBSBasic *main = OBSBasic::Get();
		OBSSource tr = OBSGetStrongRef(main->copySourceTransition);
		int trDuration = main->copySourceTransitionDuration;
		if (!tr)
			return;

		for (auto &selectedSource : GetAllSelectedSourceItems()) {
			OBSSceneItem item = main->ui->sources->Get(selectedSource.row());
			if (!item)
				continue;

			PasteShowHideTransition(item, show, tr, trDuration);
		}
	};

	action = menu->addAction(QT_UTF8(Str("Paste")));
	action->setEnabled(!!OBSGetStrongRef(copySourceTransition));
	connect(action, &QAction::triggered, std::bind(pasteTransition, action, visible));
	return menu;
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	QAction *action;

	if (qt) {
		action = menu->addAction(QTStr("Remove"));
		action->setProperty("id", qt->id);
		connect(action, &QAction::triggered, this, &OBSBasic::QuickTransitionRemoveClicked);

		menu->addSeparator();
	}

	QSpinBox *duration = new QSpinBox(menu);
	if (qt)
		duration->setProperty("id", qt->id);
	duration->setMinimum(50);
	duration->setSuffix(" ms");
	duration->setMaximum(20000);
	duration->setSingleStep(50);
	duration->setValue(qt ? qt->duration : 300);

	if (qt) {
		connect(duration, (void(QSpinBox::*)(int)) & QSpinBox::valueChanged, this,
			&OBSBasic::QuickTransitionChangeDuration);
	}

	action = menu->addAction(QTStr("FadeToBlack"));
	action->setProperty("fadeToBlack", true);

	if (qt) {
		action->setProperty("id", qt->id);
		connect(action, &QAction::triggered, this, &OBSBasic::QuickTransitionChange);
	} else {
		action->setProperty("duration", QVariant::fromValue<QWidget *>(duration));
		connect(action, &QAction::triggered, this, &OBSBasic::AddQuickTransition);
	}

	for (const auto &[uuid, transition] : transitions) {
		if (!transition)
			continue;

		action = menu->addAction(obs_source_get_name(transition));
		action->setProperty("transition_uuid", QString::fromStdString(uuid));

		if (qt) {
			action->setProperty("id", qt->id);
			connect(action, &QAction::triggered, this, &OBSBasic::QuickTransitionChange);
		} else {
			action->setProperty("duration", QVariant::fromValue<QWidget *>(duration));
			connect(action, &QAction::triggered, this, &OBSBasic::AddQuickTransition);
		}
	}

	QWidgetAction *durationAction = new QWidgetAction(menu);
	durationAction->setDefaultWidget(duration);

	menu->addSeparator();
	menu->addAction(durationAction);
	return menu;
}

void OBSBasic::AddQuickTransitionId(int id)
{
	QuickTransition *qt = GetQuickTransition(id);
	if (!qt)
		return;

	/* --------------------------------- */

	QPushButton *button = new MenuButton();
	button->setProperty("id", id);

	qt->button = button;
	ResetQuickTransitionText(qt);

	/* --------------------------------- */

	QMenu *buttonMenu = CreateTransitionMenu(button, qt);

	/* --------------------------------- */

	button->setMenu(buttonMenu);
	connect(button, &QAbstractButton::clicked, this, &OBSBasic::QuickTransitionClicked);

	QVBoxLayout *programLayout = reinterpret_cast<QVBoxLayout *>(programOptions->layout());

	int idx = 3;
	for (;; idx++) {
		QLayoutItem *item = programLayout->itemAt(idx);
		if (!item)
			break;

		QWidget *widget = item->widget();
		if (!widget || !widget->property("id").isValid())
			break;
	}

	programLayout->insertWidget(idx, button);
}

void OBSBasic::AddQuickTransition()
{
	std::string transitionUuid = sender()->property("transition_uuid").toString().toStdString();
	QSpinBox *duration = sender()->property("duration").value<QSpinBox *>();
	bool fadeToBlack = sender()->property("fadeToBlack").value<bool>();
	auto transitionIter = transitions.find(transitionUuid);
	OBSSource transition;

	if (!fadeToBlack && (transitionIter == transitions.end()))
		return;

	transition = fadeToBlack ? OBSSource(fadeTransition) : transitionIter->second;

	if (!transition)
		return;

	int id = quickTransitionIdCounter++;

	quickTransitions.emplace_back(transition, duration->value(), id, fadeToBlack);
	AddQuickTransitionId(id);

	int idx = (int)quickTransitions.size() - 1;
	AddQuickTransitionHotkey(&quickTransitions[idx]);
}

void OBSBasic::ClearQuickTransitions()
{
	for (QuickTransition &qt : quickTransitions)
		RemoveQuickTransitionHotkey(&qt);
	quickTransitions.clear();

	if (!programOptions)
		return;

	QVBoxLayout *programLayout = reinterpret_cast<QVBoxLayout *>(programOptions->layout());

	for (int idx = 0;; idx++) {
		QLayoutItem *item = programLayout->itemAt(idx);
		if (!item)
			break;

		QWidget *widget = item->widget();
		if (!widget)
			continue;

		int id = widget->property("id").toInt();
		if (id != 0) {
			delete widget;
			idx--;
		}
	}
}

void OBSBasic::QuickTransitionClicked()
{
	int id = sender()->property("id").toInt();
	TriggerQuickTransition(id);
}

void OBSBasic::QuickTransitionChange()
{
	int id = sender()->property("id").toInt();
	std::string transitionUuid = sender()->property("transition_uuid").toString().toStdString();
	bool fadeToBlack = sender()->property("fadeToBlack").value<bool>();
	QuickTransition *qt = GetQuickTransition(id);

	if (qt) {
		auto transitionIter = transitions.find(transitionUuid);
		OBSSource tr;

		if (!fadeToBlack && (transitionIter == transitions.end()))
			return;

		tr = fadeToBlack ? OBSSource(fadeTransition) : transitionIter->second;

		if (tr) {
			qt->source = tr;
			qt->fadeToBlack = fadeToBlack;
			ResetQuickTransitionText(qt);
		}
	}
}

void OBSBasic::QuickTransitionChangeDuration(int value)
{
	int id = sender()->property("id").toInt();
	QuickTransition *qt = GetQuickTransition(id);

	if (qt) {
		qt->duration = value;
		ResetQuickTransitionText(qt);
	}
}

void OBSBasic::QuickTransitionRemoveClicked()
{
	int id = sender()->property("id").toInt();
	int idx = GetQuickTransitionIdx(id);
	if (idx == -1)
		return;

	QuickTransition &qt = quickTransitions[idx];

	if (qt.button)
		qt.button->deleteLater();

	RemoveQuickTransitionHotkey(&qt);
	quickTransitions.erase(quickTransitions.begin() + idx);
}

void OBSBasic::ClearQuickTransitionWidgets()
{
	if (!IsPreviewProgramMode())
		return;

	QVBoxLayout *programLayout = reinterpret_cast<QVBoxLayout *>(programOptions->layout());

	for (int idx = 0;; idx++) {
		QLayoutItem *item = programLayout->itemAt(idx);
		if (!item)
			break;

		QWidget *widget = item->widget();
		if (!widget)
			continue;

		int id = widget->property("id").toInt();
		if (id != 0) {
			delete widget;
			idx--;
		}
	}
}

void OBSBasic::RefreshQuickTransitions()
{
	if (!IsPreviewProgramMode())
		return;

	for (QuickTransition &qt : quickTransitions)
		AddQuickTransitionId(qt.id);
}

void OBSBasic::EnableTransitionWidgets(bool enable)
{
	ui->transitions->setEnabled(enable);

	if (!enable) {
		ui->transitionProps->setEnabled(false);
	} else {
		bool configurable = obs_source_configurable(GetCurrentTransition());
		ui->transitionProps->setEnabled(configurable);
	}

	if (!IsPreviewProgramMode())
		return;

	QVBoxLayout *programLayout = reinterpret_cast<QVBoxLayout *>(programOptions->layout());

	for (int idx = 0;; idx++) {
		QLayoutItem *item = programLayout->itemAt(idx);
		if (!item)
			break;

		QPushButton *button = qobject_cast<QPushButton *>(item->widget());
		if (!button)
			continue;

		button->setEnabled(enable);
	}

	if (transitionButton)
		transitionButton->setEnabled(enable);
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *transitionsData = obs_data_array_create();

	for (const auto &[uuid, transition] : transitions) {
		if (!transition || !obs_source_configurable(transition.Get()))
			continue;

		OBSDataAutoRelease sourceData = obs_data_create();
		OBSDataAutoRelease settings = obs_source_get_settings(transition.Get());

		obs_data_set_string(sourceData, "name", obs_source_get_name(transition.Get()));
		obs_data_set_string(sourceData, "id", obs_obj_get_id(transition.Get()));
		obs_data_set_obj(sourceData, "settings", settings);

		obs_data_array_push_back(transitionsData, sourceData);
	}

	for (const OBSDataAutoRelease &transition : safeModeTransitions) {
		obs_data_array_push_back(transitionsData, transition);
	}

	return transitionsData;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitionsData, obs_load_source_cb cb, void *private_data)
{
	size_t count = obs_data_array_count(transitionsData);

	safeModeTransitions.clear();
	for (size_t i = 0; i < count; i++) {
		OBSDataAutoRelease item = obs_data_array_item(transitionsData, i);
		const char *name = obs_data_get_string(item, "name");
		const char *id = obs_data_get_string(item, "id");
		OBSDataAutoRelease settings = obs_data_get_obj(item, "settings");

		OBSSourceAutoRelease source = obs_source_create_private(id, name, settings);
		if (!obs_obj_invalid(source)) {
			std::string uuid = obs_source_get_uuid(source);
			InitTransition(source);

			transitions.insert({uuid, OBSSource(source)});
			transitionNameToUuids.insert({name, uuid});
			transitionUuids.push_back(uuid);

			emit TransitionAdded(QT_UTF8(name), QString::fromStdString(uuid));

			if (cb)
				cb(private_data, source);
		} else if (safe_mode || disable_3p_plugins) {
			safeModeTransitions.push_back(std::move(item));
		}
	}

	UpdateCurrentTransition(transitionUuids.back(), true);
}

OBSSource OBSBasic::GetOverrideTransition(OBSSource source)
{
	if (!source)
		return nullptr;

	OBSDataAutoRelease data = obs_source_get_private_settings(source);

	const char *trOverrideName = obs_data_get_string(data, "transition");

	OBSSource trOverride = nullptr;

	if (trOverrideName && *trOverrideName)
		trOverride = FindTransition(trOverrideName);

	return trOverride;
}

int OBSBasic::GetOverrideTransitionDuration(OBSSource source)
{
	if (!source)
		return 300;

	OBSDataAutoRelease data = obs_source_get_private_settings(source);
	obs_data_set_default_int(data, "transition_duration", 300);

	return (int)obs_data_get_int(data, "transition_duration");
}

int OBSBasic::GetTransitionDuration()
{
	return transitionDuration;
}

void OBSBasic::UpdateCurrentTransition(const std::string &uuid, bool setTransition)
{
	auto transitionIter = transitions.find(uuid);

	if (currentTransitionUuid == uuid || transitionIter == transitions.end())
		return;

	currentTransitionUuid = uuid;

	if (setTransition)
		SetTransition(transitionIter->second);

	emit CurrentTransitionChanged(QString::fromStdString(uuid));
}

void OBSBasic::SetCurrentTransition(const QString &uuid)
{
	auto transitionIter = transitions.find(uuid.toStdString());

	if (currentTransitionUuid == uuid.toStdString() || transitionIter == transitions.end())
		return;

	currentTransitionUuid = uuid.toStdString();
	SetTransition(transitionIter->second);

	emit CurrentTransitionChanged(uuid);
}

void OBSBasic::SetTransitionDuration(int duration)
{
	duration = std::max(duration, 50);
	duration = std::min(duration, 20000);

	if (duration == transitionDuration)
		return;

	transitionDuration = duration;

	emit TransitionDurationChanged(transitionDuration);

	OnEvent(OBS_FRONTEND_EVENT_TRANSITION_DURATION_CHANGED);
}
