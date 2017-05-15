/******************************************************************************
    Copyright (C) 2016 by Hugh Bailey <obs.jim@gmail.com>

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

#include <QSpinBox>
#include <QWidgetAction>
#include <QToolTip>
#include <QMessageBox>
#include <util/dstr.hpp>
#include "window-basic-main.hpp"
#include "display-helpers.hpp"
#include "window-namedialog.hpp"
#include "menu-button.hpp"
#include "qt-wrappers.hpp"

using namespace std;

Q_DECLARE_METATYPE(OBSScene);
Q_DECLARE_METATYPE(OBSSource);
Q_DECLARE_METATYPE(QuickTransition);

static inline QString MakeQuickTransitionText(QuickTransition *qt)
{
	QString name = QT_UTF8(obs_source_get_name(qt->source));
	if (!obs_transition_fixed(qt->source))
		name += QString(" (%1ms)").arg(QString::number(qt->duration));
	return name;
}

void OBSBasic::InitDefaultTransitions()
{
	std::vector<OBSSource> transitions;
	size_t idx = 0;
	const char *id;

	/* automatically add transitions that have no configuration (things
	 * such as cut/fade/etc) */
	while (obs_enum_transition_types(idx++, &id)) {
		if (!obs_is_source_configurable(id)) {
			const char *name = obs_source_get_display_name(id);

			obs_source_t *tr = obs_source_create_private(
					id, name, NULL);
			InitTransition(tr);
			transitions.emplace_back(tr);

			if (strcmp(id, "fade_transition") == 0)
				fadeTransition = tr;

			obs_source_release(tr);
		}
	}

	for (OBSSource &tr : transitions) {
		ui->transitions->addItem(QT_UTF8(obs_source_get_name(tr)),
				QVariant::fromValue(OBSSource(tr)));
	}
}

void OBSBasic::AddQuickTransitionHotkey(QuickTransition *qt)
{
	DStr hotkeyId;
	QString hotkeyName;

	dstr_printf(hotkeyId, "OBSBasic.QuickTransition.%d", qt->id);
	hotkeyName = QTStr("QuickTransitions.HotkeyName")
		.arg(MakeQuickTransitionText(qt));

	auto quickTransition = [] (void *data, obs_hotkey_id, obs_hotkey_t*,
			bool pressed)
	{
		int id = (int)(uintptr_t)data;
		OBSBasic *main =
			reinterpret_cast<OBSBasic*>(App()->GetMainWindow());

		if (pressed)
			QMetaObject::invokeMethod(main,
					"TriggerQuickTransition",
					Qt::QueuedConnection,
					Q_ARG(int, id));
	};

	qt->hotkey = obs_hotkey_register_frontend(hotkeyId->array,
			QT_TO_UTF8(hotkeyName), quickTransition,
			(void*)(uintptr_t)qt->id);
}

void OBSBasic::TriggerQuickTransition(int id)
{
	QuickTransition *qt = GetQuickTransition(id);

	if (qt && previewProgramMode) {
		OBSScene scene = GetCurrentScene();
		obs_source_t *source = obs_scene_get_source(scene);

		ui->transitionDuration->setValue(qt->duration);
		if (GetCurrentTransition() != qt->source)
			SetTransition(qt->source);

		TransitionToScene(source);
	}
}

void OBSBasic::RemoveQuickTransitionHotkey(QuickTransition *qt)
{
	obs_hotkey_unregister(qt->hotkey);
}

void OBSBasic::InitTransition(obs_source_t *transition)
{
	auto onTransitionStop = [] (void *data, calldata_t*) {
		OBSBasic *window = (OBSBasic*)data;
		QMetaObject::invokeMethod(window, "TransitionStopped",
				Qt::QueuedConnection);
	};

	signal_handler_t *handler = obs_source_get_signal_handler(transition);
	signal_handler_connect(handler, "transition_video_stop",
			onTransitionStop, this);
}

static inline OBSSource GetTransitionComboItem(QComboBox *combo, int idx)
{
	return combo->itemData(idx).value<OBSSource>();
}

void OBSBasic::CreateDefaultQuickTransitions()
{
	/* non-configurable transitions are always available, so add them
	 * to the "default quick transitions" list */
	quickTransitions.emplace_back(
			GetTransitionComboItem(ui->transitions, 0),
			300, quickTransitionIdCounter++);
	quickTransitions.emplace_back(
			GetTransitionComboItem(ui->transitions, 1),
			300, quickTransitionIdCounter++);
}

void OBSBasic::LoadQuickTransitions(obs_data_array_t *array)
{
	size_t count = obs_data_array_count(array);

	quickTransitionIdCounter = 1;

	for (size_t i = 0; i < count; i++) {
		obs_data_t *data = obs_data_array_item(array, i);
		obs_data_array_t *hotkeys = obs_data_get_array(data, "hotkeys");
		const char *name = obs_data_get_string(data, "name");
		int duration = obs_data_get_int(data, "duration");
		int id = obs_data_get_int(data, "id");

		if (id) {
			obs_source_t *source = FindTransition(name);
			if (source) {
				quickTransitions.emplace_back(source, duration,
						id);

				if (quickTransitionIdCounter <= id)
					quickTransitionIdCounter = id + 1;

				int idx = (int)quickTransitions.size() - 1;
				AddQuickTransitionHotkey(&quickTransitions[idx]);
				obs_hotkey_load(quickTransitions[idx].hotkey,
						hotkeys);
			}
		}

		obs_data_release(data);
		obs_data_array_release(hotkeys);
	}
}

obs_data_array_t *OBSBasic::SaveQuickTransitions()
{
	obs_data_array_t *array = obs_data_array_create();

	for (QuickTransition &qt : quickTransitions) {
		obs_data_t *data = obs_data_create();
		obs_data_array_t *hotkeys = obs_hotkey_save(qt.hotkey);

		obs_data_set_string(data, "name",
				obs_source_get_name(qt.source));
		obs_data_set_int(data, "duration", qt.duration);
		obs_data_set_array(data, "hotkeys", hotkeys);
		obs_data_set_int(data, "id", qt.id);

		obs_data_array_push_back(array, data);

		obs_data_release(data);
		obs_data_array_release(hotkeys);
	}

	return array;
}

obs_source_t *OBSBasic::FindTransition(const char *name)
{
	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource tr = ui->transitions->itemData(i)
			.value<OBSSource>();

		const char *trName = obs_source_get_name(tr);
		if (strcmp(trName, name) == 0)
			return tr;
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

	if (api) {
		api->on_event(OBS_FRONTEND_EVENT_TRANSITION_STOPPED);
		api->on_event(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	}

	swapScene = nullptr;
}

void OBSBasic::TransitionToScene(OBSSource source, bool force)
{
	obs_scene_t *scene = obs_scene_from_source(source);
	bool usingPreviewProgram = IsPreviewProgramMode();
	if (!scene)
		return;

	OBSWeakSource lastProgramScene;
	
	if (usingPreviewProgram) {
		lastProgramScene = programScene;
		programScene = OBSGetWeakRef(source);

		if (swapScenesMode && !force) {
			OBSSource newScene = OBSGetStrongRef(lastProgramScene);

			if (!sceneDuplicationMode && newScene == source)
				return;

			if (newScene && newScene != GetCurrentSceneSource())
				swapScene = lastProgramScene;
		}
	}

	if (usingPreviewProgram && sceneDuplicationMode) {
		scene = obs_scene_duplicate(scene, NULL,
				editPropertiesMode ?
				OBS_SCENE_DUP_PRIVATE_COPY :
				OBS_SCENE_DUP_PRIVATE_REFS);
		source = obs_scene_get_source(scene);
	}

	obs_source_t *transition = obs_get_output_source(0);

	if (force) {
		obs_transition_set(transition, source);
		if (api)
			api->on_event(OBS_FRONTEND_EVENT_SCENE_CHANGED);
	} else {
		obs_transition_start(transition, OBS_TRANSITION_MODE_AUTO,
				ui->transitionDuration->value(), source);
	}

	if (usingPreviewProgram && sceneDuplicationMode)
		obs_scene_release(scene);

	obs_source_release(transition);
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
	obs_source_t *oldTransition = obs_get_output_source(0);

	if (oldTransition && transition) {
		obs_transition_swap_begin(transition, oldTransition);
		if (transition != GetCurrentTransition())
			SetComboTransition(ui->transitions, transition);
		obs_set_output_source(0, transition);
		obs_transition_swap_end(transition, oldTransition);
	} else {
		obs_set_output_source(0, transition);
	}

	if (oldTransition)
		obs_source_release(oldTransition);

	bool fixed = transition ? obs_transition_fixed(transition) : false;
	ui->transitionDurationLabel->setVisible(!fixed);
	ui->transitionDuration->setVisible(!fixed);

	bool configurable = obs_source_configurable(transition);
	ui->transitionRemove->setEnabled(configurable);
	ui->transitionProps->setEnabled(configurable);

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_TRANSITION_CHANGED);
}

OBSSource OBSBasic::GetCurrentTransition()
{
	return ui->transitions->currentData().value<OBSSource>();
}

void OBSBasic::on_transitions_currentIndexChanged(int)
{
	OBSSource transition = GetCurrentTransition();
	SetTransition(transition);
}

void OBSBasic::AddTransition()
{
	QAction *action = reinterpret_cast<QAction*>(sender());
	QString idStr = action->property("id").toString();

	string name;
	QString placeHolderText = QT_UTF8(
			obs_source_get_display_name(QT_TO_UTF8(idStr)));
	QString format = placeHolderText + " (%1)";
	obs_source_t *source = nullptr;
	int i = 1;

	while ((source = FindTransition(QT_TO_UTF8(placeHolderText)))) {
		placeHolderText = format.arg(++i);
	}

	bool accepted = NameDialog::AskForName(this,
			QTStr("TransitionNameDlg.Title"),
			QTStr("TransitionNameDlg.Text"),
			name, placeHolderText);

	if (accepted) {
		if (name.empty()) {
			OBSMessageBox::information(this,
					QTStr("NoNameEntered.Title"),
					QTStr("NoNameEntered.Text"));
			AddTransition();
			return;
		}

		source = FindTransition(name.c_str());
		if (source) {
			OBSMessageBox::information(this,
					QTStr("NameExists.Title"),
					QTStr("NameExists.Text"));

			AddTransition();
			return;
		}

		source = obs_source_create_private(QT_TO_UTF8(idStr),
						name.c_str(), NULL);
		InitTransition(source);
		ui->transitions->addItem(QT_UTF8(name.c_str()),
				QVariant::fromValue(OBSSource(source)));
		ui->transitions->setCurrentIndex(ui->transitions->count() - 1);
		CreatePropertiesWindow(source);
		obs_source_release(source);

		if (api)
			api->on_event(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);

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
			action->setProperty("id", id);

			connect(action, SIGNAL(triggered()),
					this, SLOT(AddTransition()));

			menu.addAction(action);
			foundConfigurableTransitions = true;
		}
	}

	if (foundConfigurableTransitions)
		menu.exec(QCursor::pos());
}

void OBSBasic::on_transitionRemove_clicked()
{
	OBSSource tr = GetCurrentTransition();

	if (!tr || !obs_source_configurable(tr) || !QueryRemoveSource(tr))
		return;

	int idx = ui->transitions->findData(QVariant::fromValue<OBSSource>(tr));
	if (idx == -1)
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

	ui->transitions->removeItem(idx);

	if (api)
		api->on_event(OBS_FRONTEND_EVENT_TRANSITION_LIST_CHANGED);

	ClearQuickTransitionWidgets();
	RefreshQuickTransitions();
}

void OBSBasic::RenameTransition()
{
	QAction *action = reinterpret_cast<QAction*>(sender());
	QVariant variant = action->property("transition");
	obs_source_t *transition = variant.value<OBSSource>();

	string name;
	QString placeHolderText = QT_UTF8(obs_source_get_name(transition));
	obs_source_t *source = nullptr;

	bool accepted = NameDialog::AskForName(this,
			QTStr("TransitionNameDlg.Title"),
			QTStr("TransitionNameDlg.Text"),
			name, placeHolderText);

	if (accepted) {
		if (name.empty()) {
			OBSMessageBox::information(this,
					QTStr("NoNameEntered.Title"),
					QTStr("NoNameEntered.Text"));
			RenameTransition();
			return;
		}

		source = FindTransition(name.c_str());
		if (source) {
			OBSMessageBox::information(this,
					QTStr("NameExists.Title"),
					QTStr("NameExists.Text"));

			RenameTransition();
			return;
		}

		obs_source_set_name(transition, name.c_str());
		int idx = ui->transitions->findData(variant);
		if (idx != -1) {
			ui->transitions->setItemText(idx, QT_UTF8(name.c_str()));
			ClearQuickTransitionWidgets();
			RefreshQuickTransitions();
		}
	}
}

void OBSBasic::on_transitionProps_clicked()
{
	OBSSource source = GetCurrentTransition();

	if (!obs_source_configurable(source))
		return;

	auto properties = [&] () {
		CreatePropertiesWindow(source);
	};

	QMenu menu(this);

	QAction *action = new QAction(QTStr("Rename"), &menu);
	connect(action, SIGNAL(triggered()), this, SLOT(RenameTransition()));
	action->setProperty("transition", QVariant::fromValue(source));
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

template <typename T>
static T GetOBSRef(QListWidgetItem *item)
{
	return item->data(static_cast<int>(QtDataRole::OBSRef)).value<T>();
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
				ui->scenes->setCurrentItem(item);
				ui->scenes->blockSignals(false);
				break;
			}
		}
	}

	UpdateSceneSelection(scene);

	bool userSwitched = (!force && !disableSaving);
	blog(LOG_INFO, "%s to scene '%s'",
			userSwitched ? "User switched" : "Switched",
			obs_source_get_name(scene));
}

void OBSBasic::CreateProgramDisplay()
{
	program = new OBSQTDisplay();

	auto displayResize = [this]() {
		struct obs_video_info ovi;

		if (obs_get_video_info(&ovi))
			ResizeProgram(ovi.base_width, ovi.base_height);
	};

	connect(program.data(), &OBSQTDisplay::DisplayResized,
			displayResize);

	auto addDisplay = [this] (OBSQTDisplay *window)
	{
		obs_display_add_draw_callback(window->GetDisplay(),
				OBSBasic::RenderProgram, this);

		struct obs_video_info ovi;
		if (obs_get_video_info(&ovi))
			ResizeProgram(ovi.base_width, ovi.base_height);
	};

	connect(program.data(), &OBSQTDisplay::DisplayCreated, addDisplay);

	program->setSizePolicy(QSizePolicy::Expanding,
			QSizePolicy::Expanding);
}

void OBSBasic::TransitionClicked()
{
	if (previewProgramMode)
		TransitionToScene(GetCurrentScene());
}

void OBSBasic::CreateProgramOptions()
{
	programOptions = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	layout->setSpacing(4);

	QPushButton *configTransitions = new QPushButton();
	configTransitions->setMaximumSize(22, 22);
	configTransitions->setProperty("themeID", "configIconSmall");
	configTransitions->setFlat(true);

	QHBoxLayout *mainButtonLayout = new QHBoxLayout();
	mainButtonLayout->setSpacing(2);

	QPushButton *transitionButton = new QPushButton(QTStr("Transition"));
	QHBoxLayout *quickTransitions = new QHBoxLayout();
	quickTransitions->setSpacing(2);

	QPushButton *addQuickTransition = new QPushButton();
	addQuickTransition->setMaximumSize(22, 22);
	addQuickTransition->setProperty("themeID", "addIconSmall");
	addQuickTransition->setFlat(true);

	QLabel *quickTransitionsLabel = new QLabel(QTStr("QuickTransitions"));

	quickTransitions->addWidget(quickTransitionsLabel);
	quickTransitions->addWidget(addQuickTransition);

	mainButtonLayout->addWidget(transitionButton);
	mainButtonLayout->addWidget(configTransitions);

	layout->addStretch(0);
	layout->addLayout(mainButtonLayout);
	layout->addLayout(quickTransitions);
	layout->addStretch(0);

	programOptions->setLayout(layout);

	auto onAdd = [this] () {
		QPointer<QMenu> menu = CreateTransitionMenu(this, nullptr);
		menu->exec(QCursor::pos());
	};

	auto onConfig = [this] () {
		QMenu menu(this);
		QAction *action;

		auto toggleEditProperties = [this] () {
			editPropertiesMode = !editPropertiesMode;

			OBSSource actualScene = OBSGetStrongRef(programScene);
			if (actualScene)
				TransitionToScene(actualScene, true);
		};

		auto toggleSwapScenesMode = [this] () {
			swapScenesMode = !swapScenesMode;
		};

		auto toggleSceneDuplication = [this] () {
			sceneDuplicationMode = !sceneDuplicationMode;

			OBSSource actualScene = OBSGetStrongRef(programScene);
			if (actualScene)
				TransitionToScene(actualScene, true);
		};

		auto showToolTip = [&] () {
			QAction *act = menu.activeAction();
			QToolTip::showText(QCursor::pos(), act->toolTip(),
					&menu, menu.actionGeometry(act));
		};

		action = menu.addAction(QTStr("QuickTransitions.DuplicateScene"));
		action->setToolTip(QTStr("QuickTransitions.DuplicateSceneTT"));
		action->setCheckable(true);
		action->setChecked(sceneDuplicationMode);
		connect(action, &QAction::triggered, toggleSceneDuplication);
		connect(action, &QAction::hovered, showToolTip);

		action = menu.addAction(QTStr("QuickTransitions.EditProperties"));
		action->setToolTip(QTStr("QuickTransitions.EditPropertiesTT"));
		action->setCheckable(true);
		action->setChecked(editPropertiesMode);
		action->setEnabled(sceneDuplicationMode);
		connect(action, &QAction::triggered, toggleEditProperties);
		connect(action, &QAction::hovered, showToolTip);

		action = menu.addAction(QTStr("QuickTransitions.SwapScenes"));
		action->setToolTip(QTStr("QuickTransitions.SwapScenesTT"));
		action->setCheckable(true);
		action->setChecked(swapScenesMode);
		connect(action, &QAction::triggered, toggleSwapScenesMode);
		connect(action, &QAction::hovered, showToolTip);

		menu.exec(QCursor::pos());
	};

	connect(transitionButton, &QAbstractButton::clicked,
			this, &OBSBasic::TransitionClicked);
	connect(addQuickTransition, &QAbstractButton::clicked, onAdd);
	connect(configTransitions, &QAbstractButton::clicked, onConfig);
}

void OBSBasic::on_modeSwitch_clicked()
{
	SetPreviewProgramMode(!IsPreviewProgramMode());
}

static inline void ResetQuickTransitionText(QuickTransition *qt)
{
	qt->button->setText(MakeQuickTransitionText(qt));
}

QMenu *OBSBasic::CreateTransitionMenu(QWidget *parent, QuickTransition *qt)
{
	QMenu *menu = new QMenu(parent);
	QAction *action;

	if (qt) {
		action = menu->addAction(QTStr("Remove"));
		action->setProperty("id", qt->id);
		connect(action, &QAction::triggered,
				this, &OBSBasic::QuickTransitionRemoveClicked);

		menu->addSeparator();
	}

	QSpinBox *duration = new QSpinBox(menu);
	if (qt)
		duration->setProperty("id", qt->id);
	duration->setMinimum(50);
	duration->setSuffix("ms");
	duration->setMaximum(20000);
	duration->setSingleStep(50);
	duration->setValue(qt ? qt->duration : 300);

	if (qt) {
		connect(duration, (void (QSpinBox::*)(int))&QSpinBox::valueChanged,
				this, &OBSBasic::QuickTransitionChangeDuration);
	}

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource tr = GetTransitionComboItem(ui->transitions, i);

		action = menu->addAction(obs_source_get_name(tr));
		action->setProperty("transition_index", i);

		if (qt) {
			action->setProperty("id", qt->id);
			connect(action, &QAction::triggered, this,
					&OBSBasic::QuickTransitionChange);
		} else {
			action->setProperty("duration",
					QVariant::fromValue<QWidget*>(duration));
			connect(action, &QAction::triggered, this,
					&OBSBasic::AddQuickTransition);
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
	connect(button, &QAbstractButton::clicked,
			this, &OBSBasic::QuickTransitionClicked);

	QVBoxLayout *programLayout =
		reinterpret_cast<QVBoxLayout*>(programOptions->layout());

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
	int trIdx = sender()->property("transition_index").toInt();
	QSpinBox *duration = sender()->property("duration").value<QSpinBox*>();
	OBSSource transition = GetTransitionComboItem(ui->transitions, trIdx);
	int id = quickTransitionIdCounter++;

	quickTransitions.emplace_back(transition, duration->value(), id);
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

	QVBoxLayout *programLayout =
		reinterpret_cast<QVBoxLayout*>(programOptions->layout());

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
	int trIdx = sender()->property("transition_index").toInt();
	QuickTransition *qt = GetQuickTransition(id);

	if (qt) {
		qt->source = GetTransitionComboItem(ui->transitions, trIdx);
		ResetQuickTransitionText(qt);
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

	QVBoxLayout *programLayout =
		reinterpret_cast<QVBoxLayout*>(programOptions->layout());

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

void OBSBasic::SetPreviewProgramMode(bool enabled)
{
	if (IsPreviewProgramMode() == enabled)
		return;

	ui->modeSwitch->setChecked(enabled);
	os_atomic_set_bool(&previewProgramMode, enabled);

	if (IsPreviewProgramMode()) {
		if (!previewEnabled)
			EnablePreviewDisplay(true);

		CreateProgramDisplay();
		CreateProgramOptions();

		OBSScene curScene = GetCurrentScene();

		obs_scene_t *dup;
		if (sceneDuplicationMode) {
			dup = obs_scene_duplicate(curScene, nullptr,
					editPropertiesMode ?
					OBS_SCENE_DUP_PRIVATE_COPY :
					OBS_SCENE_DUP_PRIVATE_REFS);
		} else {
			dup = curScene;
			obs_scene_addref(dup);
		}

		obs_source_t *transition = obs_get_output_source(0);
		obs_source_t *dup_source = obs_scene_get_source(dup);
		obs_transition_set(transition, dup_source);
		obs_source_release(transition);
		obs_scene_release(dup);

		if (curScene) {
			obs_source_t *source = obs_scene_get_source(curScene);
			obs_source_inc_showing(source);
			lastScene = OBSGetWeakRef(source);
			programScene = OBSGetWeakRef(source);
		}

		RefreshQuickTransitions();

		ui->previewLayout->addWidget(programOptions);
		ui->previewLayout->addWidget(program);
		program->show();

		blog(LOG_INFO, "Switched to Preview/Program mode");
		blog(LOG_INFO, "-----------------------------"
				"-------------------");
	} else {
		OBSSource actualProgramScene = OBSGetStrongRef(programScene);
		if (!actualProgramScene)
			actualProgramScene = GetCurrentSceneSource();
		else
			SetCurrentScene(actualProgramScene);
		TransitionToScene(actualProgramScene, true);

		delete programOptions;
		delete program;

		if (lastScene) {
			OBSSource actualLastScene = OBSGetStrongRef(lastScene);
			if (actualLastScene)
				obs_source_dec_showing(actualLastScene);
			lastScene = nullptr;
		}

		programScene = nullptr;
		swapScene = nullptr;

		for (QuickTransition &qt : quickTransitions)
			qt.button = nullptr;

		if (!previewEnabled)
			EnablePreviewDisplay(false);

		blog(LOG_INFO, "Switched to regular Preview mode");
		blog(LOG_INFO, "-----------------------------"
				"-------------------");
	}

	UpdateTitleBar();
}

void OBSBasic::RenderProgram(void *data, uint32_t cx, uint32_t cy)
{
	OBSBasic *window = static_cast<OBSBasic*>(data);
	obs_video_info ovi;

	obs_get_video_info(&ovi);

	window->programCX = int(window->programScale * float(ovi.base_width));
	window->programCY = int(window->programScale * float(ovi.base_height));

	gs_viewport_push();
	gs_projection_push();

	/* --------------------------------------- */

	gs_ortho(0.0f, float(ovi.base_width), 0.0f, float(ovi.base_height),
			-100.0f, 100.0f);
	gs_set_viewport(window->programX, window->programY,
			window->programCX, window->programCY);

	window->DrawBackdrop(float(ovi.base_width), float(ovi.base_height));

	obs_render_main_view();
	gs_load_vertexbuffer(nullptr);

	/* --------------------------------------- */

	gs_projection_pop();
	gs_viewport_pop();

	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}

void OBSBasic::ResizeProgram(uint32_t cx, uint32_t cy)
{
	QSize targetSize;

	/* resize program panel to fix to the top section of the window */
	targetSize = GetPixelSize(program);
	GetScaleAndCenterPos(int(cx), int(cy),
			targetSize.width()  - PREVIEW_EDGE_SIZE * 2,
			targetSize.height() - PREVIEW_EDGE_SIZE * 2,
			programX, programY, programScale);

	programX += float(PREVIEW_EDGE_SIZE);
	programY += float(PREVIEW_EDGE_SIZE);
}

obs_data_array_t *OBSBasic::SaveTransitions()
{
	obs_data_array_t *transitions = obs_data_array_create();

	for (int i = 0; i < ui->transitions->count(); i++) {
		OBSSource tr = ui->transitions->itemData(i).value<OBSSource>();
		if (!obs_source_configurable(tr))
			continue;

		obs_data_t *sourceData = obs_data_create();
		obs_data_t *settings = obs_source_get_settings(tr);

		obs_data_set_string(sourceData, "name", obs_source_get_name(tr));
		obs_data_set_string(sourceData, "id", obs_obj_get_id(tr));
		obs_data_set_obj(sourceData, "settings", settings);

		obs_data_array_push_back(transitions, sourceData);

		obs_data_release(settings);
		obs_data_release(sourceData);
	}

	return transitions;
}

void OBSBasic::LoadTransitions(obs_data_array_t *transitions)
{
	size_t count = obs_data_array_count(transitions);

	for (size_t i = 0; i < count; i++) {
		obs_data_t *item = obs_data_array_item(transitions, i);
		const char *name = obs_data_get_string(item, "name");
		const char *id = obs_data_get_string(item, "id");
		obs_data_t *settings = obs_data_get_obj(item, "settings");

		obs_source_t *source = obs_source_create_private(id, name,
				settings);
		if (!obs_obj_invalid(source)) {
			InitTransition(source);
			ui->transitions->addItem(QT_UTF8(name),
					QVariant::fromValue(OBSSource(source)));
			ui->transitions->setCurrentIndex(
					ui->transitions->count() - 1);
		}

		obs_data_release(settings);
		obs_data_release(item);
		obs_source_release(source);
	}
}
