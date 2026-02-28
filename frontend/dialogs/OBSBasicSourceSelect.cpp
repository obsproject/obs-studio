/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include "OBSApp.hpp"
#include "OBSBasicSourceSelect.hpp"

#include <utility/ResizeSignaler.hpp>
#include <utility/ThumbnailManager.hpp>
#include <utility/ThumbnailView.hpp>

#include "qt-wrappers.hpp"

#include <QList>
#include <QMessageBox>

#include "moc_OBSBasicSourceSelect.cpp"

struct AddSourceData {
	// Input data
	obs_source_t *source;
	bool visible;
	obs_transform_info *transform = nullptr;
	obs_sceneitem_crop *crop = nullptr;
	obs_blending_method *blend_method = nullptr;
	obs_blending_type *blend_mode = nullptr;
	obs_scale_type *scale_type = nullptr;
	const char *show_transition_id = nullptr;
	const char *hide_transition_id = nullptr;
	OBSData show_transition_settings;
	OBSData hide_transition_settings;
	uint32_t show_transition_duration = 300;
	uint32_t hide_transition_duration = 300;
	OBSData private_settings;

	// Return data
	obs_sceneitem_t *scene_item = nullptr;
};

namespace {
QString getDisplayNameForSourceType(QString type)
{
	if (type == "scene") {
		return QTStr("Basic.Scene");
	}

	const char *inputChar = obs_get_latest_input_type_id(type.toUtf8().constData());
	const char *displayChar = obs_source_get_display_name(inputChar);
	std::string displayId = (displayChar) ? displayChar : "";

	if (!displayId.empty()) {
		return QString::fromStdString(displayId);
	} else {
		return QString();
	}
}

std::string getNewSourceName(std::string_view name)
{
	std::string newName{name};
	int suffix = 1;

	for (;;) {
		OBSSourceAutoRelease existing_source = obs_get_source_by_name(newName.c_str());
		if (!existing_source) {
			break;
		}

		char nextName[256];
		std::snprintf(nextName, sizeof(nextName), "%s %d", name.data(), ++suffix);
		newName = nextName;
	}

	return newName;
}

void setupSceneItem(void *_data, obs_scene_t *scene)
{
	AddSourceData *data = (AddSourceData *)_data;
	obs_sceneitem_t *sceneitem;

	sceneitem = obs_scene_add(scene, data->source);

	if (data->transform != nullptr) {
		obs_sceneitem_set_info2(sceneitem, data->transform);
	}
	if (data->crop != nullptr) {
		obs_sceneitem_set_crop(sceneitem, data->crop);
	}
	if (data->blend_method != nullptr) {
		obs_sceneitem_set_blending_method(sceneitem, *data->blend_method);
	}
	if (data->blend_mode != nullptr) {
		obs_sceneitem_set_blending_mode(sceneitem, *data->blend_mode);
	}
	if (data->scale_type != nullptr) {
		obs_sceneitem_set_scale_filter(sceneitem, *data->scale_type);
	}

	if (data->show_transition_id && *data->show_transition_id) {
		OBSSourceAutoRelease source = obs_source_create(data->show_transition_id, data->show_transition_id,
								data->show_transition_settings, nullptr);

		if (source) {
			obs_sceneitem_set_transition(sceneitem, true, source);
		}
	}

	if (data->hide_transition_id && *data->hide_transition_id) {
		OBSSourceAutoRelease source = obs_source_create(data->hide_transition_id, data->hide_transition_id,
								data->hide_transition_settings, nullptr);

		if (source) {
			obs_sceneitem_set_transition(sceneitem, false, source);
		}
	}

	obs_sceneitem_set_transition_duration(sceneitem, true, data->show_transition_duration);
	obs_sceneitem_set_transition_duration(sceneitem, false, data->hide_transition_duration);

	obs_sceneitem_set_visible(sceneitem, data->visible);

	if (data->private_settings) {
		OBSDataAutoRelease newPrivateSettings = obs_sceneitem_get_private_settings(sceneitem);
		obs_data_apply(newPrivateSettings, data->private_settings);
	}

	data->scene_item = sceneitem;
}

std::optional<OBSSceneItem> setupExistingSource(std::string_view uuid, bool visible, bool duplicate,
						SourceCopyInfo *info = nullptr)
{
	OBSSourceAutoRelease temp = obs_get_source_by_uuid(uuid.data());
	if (!temp) {
		return std::nullopt;
	}

	OBSBasic *main = OBSBasic::Get();
	OBSScene scene = main->GetCurrentScene();
	if (!scene) {
		return std::nullopt;
	}

	if (duplicate) {
		OBSSource source = temp.Get();
		std::string new_name = getNewSourceName(obs_source_get_name(source));
		temp = obs_source_duplicate(source, new_name.c_str(), false);

		if (!source) {
			return std::nullopt;
		}
	}

	AddSourceData data;
	data.source = temp;
	data.visible = visible;

	if (info) {
		data.transform = &info->transform;
		data.crop = &info->crop;
		data.blend_method = &info->blend_method;
		data.blend_mode = &info->blend_mode;
		data.scale_type = &info->scale_type;
		data.show_transition_id = info->show_transition_id;
		data.hide_transition_id = info->hide_transition_id;
		data.show_transition_settings = std::move(info->show_transition_settings);
		data.hide_transition_settings = std::move(info->hide_transition_settings);
		data.show_transition_duration = info->show_transition_duration;
		data.hide_transition_duration = info->hide_transition_duration;
		data.private_settings = std::move(info->private_settings);
	}

	obs_enter_graphics();
	obs_scene_atomic_update(scene, setupSceneItem, &data);
	obs_leave_graphics();

	return OBSSceneItem(data.scene_item);
}

std::optional<OBSSource> setupNewSource(QWidget *parent, const char *id, const char *name)
{
	OBSBasic *main = OBSBasic::Get();
	OBSScene scene = main->GetCurrentScene();

	if (!scene) {
		return std::nullopt;
	}

	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source && parent) {
		OBSMessageBox::information(parent, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
		return std::nullopt;
	}

	const char *v_id = obs_get_latest_input_type_id(id);

	source = obs_source_create(v_id, name, NULL, nullptr);
	if (!source) {
		return std::nullopt;
	}

	return OBSSource(source.Get());
}
} // namespace

OBSBasicSourceSelect::OBSBasicSourceSelect(OBSBasic *parent, undo_stack &undo_s)
	: QDialog(parent),
	  ui(new Ui::OBSBasicSourceSelect),
	  undo_s(undo_s)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	existingFlowLayout = ui->existingListFrame->flowLayout();
	existingFlowLayout->setContentsMargins(0, 0, 0, 0);
	existingFlowLayout->setSpacing(0);

	/* The scroll viewport is not accessible via Designer, so we have to disable autoFillBackground here.
	 *
	 * Additionally when Qt calls setWidget on a scrollArea to set the contents widget, it force sets
	 * autoFillBackground to true overriding whatever is set in Designer so we have to do that here too.
	 */
	ui->existingScrollArea->viewport()->setAutoFillBackground(false);
	ui->existingScrollContents->setAutoFillBackground(false);

	auto resizeSignaler = new ResizeSignaler(ui->existingScrollArea);
	ui->existingScrollArea->installEventFilter(resizeSignaler);

	connect(resizeSignaler, &ResizeSignaler::resized, this, &OBSBasicSourceSelect::updateButtonVisibility);
	connect(ui->existingScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this,
		&OBSBasicSourceSelect::updateButtonVisibility);
	connect(ui->existingScrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this,
		&OBSBasicSourceSelect::updateButtonVisibility);

	ui->createNewFrame->setVisible(false);
	ui->deprecatedCreateLabel->setVisible(false);
	ui->deprecatedCreateLabel->setProperty("class", "text-muted");

	rebuildSourceTypeList();
	refreshSources();

	updateExistingSources(RECENT_LIST_LIMIT);

	signalHandlers.reserve(2);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_create", &OBSBasicSourceSelect::obsSourceCreated,
				    this);
	signalHandlers.emplace_back(obs_get_signal_handler(), "source_destroy", &OBSBasicSourceSelect::obsSourceRemoved,
				    this);

	connect(ui->sourceTypeList, &QListWidget::itemDoubleClicked, this, &OBSBasicSourceSelect::createNew);
	connect(ui->sourceTypeList, &QListWidget::currentItemChanged, this, &OBSBasicSourceSelect::sourceTypeSelected);
	connect(ui->newSourceName, &QLineEdit::returnPressed, this, &OBSBasicSourceSelect::createNew);
	connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	connect(ui->addExistingButton, &QAbstractButton::clicked, this, &OBSBasicSourceSelect::addSelectedSources);
	connect(this, &OBSBasicSourceSelect::selectedItemsChanged, this, [=]() {
		ui->addExistingButton->setEnabled(selectedItems.size() > 0);
		if (selectedItems.size() > 0) {
			ui->addExistingButton->setText(QTStr("Add %1 Existing").arg(selectedItems.size()));
		} else {
			ui->addExistingButton->setText("Add Existing");
		}
	});

	App()->DisableHotkeys();
}

OBSBasicSourceSelect::~OBSBasicSourceSelect()
{
	App()->UpdateHotkeyFocusSetting();
}

void OBSBasicSourceSelect::obsSourceCreated(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicSourceSelect *>(data), "handleSourceCreated",
				  Qt::QueuedConnection);
}

void OBSBasicSourceSelect::obsSourceRemoved(void *data, calldata_t *params)
{
	obs_source_t *source = static_cast<obs_source_t *>(calldata_ptr(params, "source"));
	auto uuidPointer = obs_source_get_uuid(source);

	if (uuidPointer) {
		QMetaObject::invokeMethod(static_cast<OBSBasicSourceSelect *>(data), "handleSourceRemoved",
					  Qt::QueuedConnection, Q_ARG(QString, QString::fromUtf8(uuidPointer)));
	}
}

void OBSBasicSourceSelect::sourcePaste(SourceCopyInfo &info, bool duplicate)
{
	OBSSource source = OBSGetStrongRef(info.weak_source);
	if (!source) {
		return;
	}

	std::string uuid = obs_source_get_uuid(source);

	setupExistingSource(uuid, info.visible, duplicate, &info);
}

void OBSBasicSourceSelect::showEvent(QShowEvent *)
{
	QTimer::singleShot(0, this, [this] { updateButtonVisibility(); });
}

void OBSBasicSourceSelect::updateButtonVisibility()
{
	QList<QAbstractButton *> buttons = sourceButtons->buttons();

	if (buttons.size() <= 0) {
		return;
	}

	auto firstButton = buttons.first();

	// Allow some room for previous/next rows to make scrolling a bit more seamless
	QRect scrollAreaRect(QPoint(0, 0), ui->existingScrollArea->size());
	scrollAreaRect.setTop(scrollAreaRect.top() - firstButton->rect().height());
	scrollAreaRect.setBottom(scrollAreaRect.bottom() + firstButton->rect().height());

	for (QAbstractButton *button : buttons) {
		SourceSelectButton *sourceButton = qobject_cast<SourceSelectButton *>(button);
		if (sourceButton) {
			QRect buttonRect = button->rect();
			buttonRect.moveTo(button->mapTo(ui->existingScrollArea, buttonRect.topLeft()));

			if (scrollAreaRect.intersects(buttonRect)) {
				sourceButton->setThumbnailEnabled(true);
			} else {
				sourceButton->setThumbnailEnabled(false);
			}
		}
	}
}

void OBSBasicSourceSelect::refreshSources()
{
	weakSources.clear();

	obs_enum_sources(enumSourcesCallback, this);

	struct obs_frontend_source_list list = {};
	obs_frontend_get_scenes(&list);

	for (size_t i = 0; i < list.sources.num; i++) {
		obs_source_t *source = list.sources.array[i];

		weakSources.push_back(obs_source_get_weak_source(source));
	}
	obs_frontend_source_list_free(&list);

	emit sourcesUpdated();
}

void OBSBasicSourceSelect::updateExistingSources(int limit)
{
	QLayout *layout = ui->existingListFrame->flowLayout();

	// Clear existing buttons when switching types
	QLayoutItem *child = nullptr;
	while ((child = layout->takeAt(0)) != nullptr) {
		if (child->widget()) {
			child->widget()->deleteLater();
		}
		delete child;
	}

	if (sourceButtons) {
		sourceButtons->deleteLater();
	}

	sourceButtons = new QButtonGroup(this);
	sourceButtons->setExclusive(false);

	std::vector<obs_weak_source_t *> matchingSources{};
	std::copy_if(weakSources.begin(), weakSources.end(), std::back_inserter(matchingSources),
		     [this](obs_weak_source_t *weak) {
			     obs_source_t *source = OBSGetStrongRef(weak);

			     if (!source || obs_source_removed(source)) {
				     return false;
			     }

			     const char *id = obs_source_get_unversioned_id(source);
			     QString sourceTypeId = QString(id);

			     if (sourceTypeId.compare("group") == 0) {
				     return false;
			     }

			     if (selectedTypeId.compare(RECENT_TYPE_ID) == 0) {
				     // Skip listing scenes in recent sources list
				     if (sourceTypeId.compare("scene") == 0) {
					     return false;
				     }

				     return true;
			     }

			     if (selectedTypeId.compare(sourceTypeId) == 0) {
				     return true;
			     }

			     return false;
		     });

	QWidget *prevTabWidget = ui->sourceTypeList;

	auto createSourceButton = [this, &prevTabWidget](obs_weak_source_t *weak) {
		OBSSource source{OBSGetStrongRef(weak)};

		if (!source) {
			return;
		}

		SourceSelectButton *newButton = new SourceSelectButton(weak, ui->existingListFrame);
		std::string uuid = obs_source_get_uuid(source);

		existingFlowLayout->addWidget(newButton);
		sourceButtons->addButton(newButton);

		bool isSelected = false;
		if (selectedItems.size() > 0) {
			if (std::find(selectedItems.begin(), selectedItems.end(), uuid) != selectedItems.end()) {
				isSelected = true;
			}
		}

		newButton->setChecked(isSelected);

		if (!prevTabWidget) {
			setTabOrder(ui->existingListFrame, newButton);
		} else {
			setTabOrder(prevTabWidget, newButton);
		}

		prevTabWidget = newButton;
	};

	bool isReverseListOrder = selectedTypeId.compare(RECENT_TYPE_ID) == 0;
	size_t iterationLimit = limit > 0 ? std::min(static_cast<size_t>(limit), matchingSources.size())
					  : matchingSources.size();
	if (isReverseListOrder) {
		std::for_each(matchingSources.rbegin(), matchingSources.rbegin() + iterationLimit, createSourceButton);
	} else {
		std::for_each(matchingSources.begin(), matchingSources.begin() + iterationLimit, createSourceButton);
	}

	setTabOrder(prevTabWidget, ui->addExistingContainer);

	connect(sourceButtons, &QButtonGroup::buttonToggled, this, &OBSBasicSourceSelect::sourceButtonToggled);

	ui->existingListFrame->adjustSize();
}

bool OBSBasicSourceSelect::enumSourcesCallback(void *data, obs_source_t *source)
{
	if (obs_source_is_hidden(source)) {
		return true;
	}

	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect *>(data);

	window->weakSources.push_back(obs_source_get_weak_source(source));

	return true;
}

void OBSBasicSourceSelect::rebuildSourceTypeList()
{
	ui->sourceTypeList->clear();

	OBSBasic *main = qobject_cast<OBSBasic *>(App()->GetMainWindow());

	const char *unversioned_type;
	const char *type;

	size_t idx = 0;

	while (obs_enum_input_types2(idx++, &type, &unversioned_type)) {
		const char *name = obs_source_get_display_name(type);
		uint32_t caps = obs_get_source_output_flags(type);

		if ((caps & OBS_SOURCE_CAP_DISABLED) != 0) {
			continue;
		}

		QListWidgetItem *newItem = new QListWidgetItem(ui->sourceTypeList);
		newItem->setData(Qt::DisplayRole, name);
		newItem->setData(UNVERSIONED_ID_ROLE, unversioned_type);

		if ((caps & OBS_SOURCE_DEPRECATED) != 0) {
			newItem->setData(DEPRECATED_ROLE, true);
		} else {
			newItem->setData(DEPRECATED_ROLE, false);

			QIcon icon;
			icon = main->GetSourceIcon(type);
			newItem->setIcon(icon);
		}
	}

	QListWidgetItem *newItem = new QListWidgetItem(ui->sourceTypeList);
	newItem->setData(Qt::DisplayRole, Str("Basic.Scene"));
	newItem->setData(UNVERSIONED_ID_ROLE, "scene");

	QIcon icon;
	icon = main->GetSceneIcon();
	newItem->setIcon(icon);

	ui->sourceTypeList->sortItems();

	// Shift Deprecated sources to the bottom
	QList<QListWidgetItem *> deprecatedItems;
	for (int i = 0; i < ui->sourceTypeList->count(); i++) {
		QListWidgetItem *item = ui->sourceTypeList->item(i);
		if (!item) {
			break;
		}

		bool isDeprecated = item->data(DEPRECATED_ROLE).toBool();
		if (isDeprecated) {
			ui->sourceTypeList->takeItem(i);

			QVariant unversionedIdData = item->data(UNVERSIONED_ID_ROLE);

			deprecatedItems.append(item);
		}
	}

	for (auto &item : deprecatedItems) {
		ui->sourceTypeList->addItem(item);
	}

	QListWidgetItem *allSources = new QListWidgetItem();
	allSources->setData(Qt::DisplayRole, Str("Basic.SourceSelect.Recent"));
	allSources->setData(UNVERSIONED_ID_ROLE, QVariant(RECENT_TYPE_ID));
	ui->sourceTypeList->insertItem(0, allSources);

	ui->sourceTypeList->setCurrentItem(allSources);
	ui->sourceTypeList->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
}

void OBSBasicSourceSelect::sourceButtonToggled(QAbstractButton *button, bool checked)
{
	SourceSelectButton *sourceButton = dynamic_cast<SourceSelectButton *>(button);

	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	bool ctrlDown = (modifiers & Qt::ControlModifier);
	bool shiftDown = (modifiers & Qt::ShiftModifier);

	if (!sourceButton) {
		clearSelectedItems();
		return;
	}

	int toggledIndex = existingFlowLayout->indexOf(sourceButton);

	std::string toggledUuid = sourceButton->uuid();

	if (shiftDown) {
		if (!ctrlDown) {
			clearSelectedItems();
		}

		QSignalBlocker block(sourceButtons);

		int originalSelectedIndex = lastSelectedIndex;

		int start = std::min(toggledIndex, lastSelectedIndex);
		int end = std::max(toggledIndex, lastSelectedIndex);

		for (int i = start; i <= end; i++) {
			auto item = existingFlowLayout->itemAt(i);
			if (!item) {
				continue;
			}

			auto widget = item->widget();
			if (!widget) {
				continue;
			}

			auto entry = dynamic_cast<SourceSelectButton *>(widget);
			if (entry) {
				QSignalBlocker blocker(entry);
				entry->setChecked(true);

				std::string uuid = entry->uuid();
				addSelectedItem(uuid);
			}
		}

		lastSelectedIndex = originalSelectedIndex;

	} else if (ctrlDown) {
		lastSelectedIndex = toggledIndex;

		if (checked) {
			addSelectedItem(toggledUuid);
		} else {
			removeSelectedItem(toggledUuid);
		}
	} else {
		lastSelectedIndex = toggledIndex;

		bool reselectItem = selectedItems.size() > 1;
		clearSelectedItems();
		if (checked) {
			addSelectedItem(toggledUuid);
		} else if (reselectItem) {
			QSignalBlocker blocker(button);
			button->setChecked(true);
			addSelectedItem(toggledUuid);
		}
	}
}

void OBSBasicSourceSelect::sourceDropped(QString uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toStdString().c_str());
	if (source) {
		bool visible = ui->sourceVisible->isChecked();

		addExisting(uuid.toStdString(), visible);
	}
}

void OBSBasicSourceSelect::addSelectedItem(std::string uuid)
{
	auto it = std::find(selectedItems.begin(), selectedItems.end(), uuid);

	if (it == selectedItems.end()) {
		selectedItems.push_back(uuid);
		emit selectedItemsChanged();
	}

	auto button = findButtonForUuid(uuid);
	if (!button) {
		return;
	}

	lastSelectedIndex = existingFlowLayout->indexOf(button);
}

void OBSBasicSourceSelect::removeSelectedItem(std::string uuid)
{
	auto it = std::find(selectedItems.begin(), selectedItems.end(), uuid);
	if (it != selectedItems.end()) {
		selectedItems.erase(it);
		emit selectedItemsChanged();
	}

	auto button = findButtonForUuid(uuid);
	if (!button) {
		return;
	}

	lastSelectedIndex = existingFlowLayout->indexOf(button);
}

void OBSBasicSourceSelect::clearSelectedItems()
{
	if (selectedItems.size() == 0) {
		return;
	}

	sourceButtons->blockSignals(true);
	for (auto &uuid : selectedItems) {
		auto sourceButton = findButtonForUuid(uuid);
		if (sourceButton) {
			QSignalBlocker block(sourceButton);
			sourceButton->setChecked(false);
		}
	}
	sourceButtons->blockSignals(false);

	selectedItems.clear();
	emit selectedItemsChanged();
}

SourceSelectButton *OBSBasicSourceSelect::findButtonForUuid(std::string uuid)
{
	for (int i = 0; i <= existingFlowLayout->count(); i++) {
		auto layoutItem = existingFlowLayout->itemAt(i);
		if (!layoutItem || !layoutItem->widget()) {
			continue;
		}

		auto widget = layoutItem->widget();
		if (!widget) {
			continue;
		}

		auto entry = dynamic_cast<SourceSelectButton *>(widget);
		if (entry && entry->uuid() == uuid) {
			return entry;
		}
	}

	return nullptr;
}

void OBSBasicSourceSelect::createNew()
{
	bool visible = ui->sourceVisible->isChecked();

	if (ui->newSourceName->text().isEmpty()) {
		return;
	}

	if (selectedTypeId.compare(RECENT_TYPE_ID) == 0) {
		return;
	}

	if (selectedTypeId.compare("scene") == 0) {
		return;
	}

	std::string sourceType = selectedTypeId.toStdString();
	const char *id = sourceType.c_str();
	std::string newName = ui->newSourceName->text().toStdString();

	auto createResult = setupNewSource(this, id, newName.c_str());
	if (!createResult.has_value()) {
		return;
	}

	OBSSource newSource = createResult.value();
	if (strcmp(obs_source_get_id(newSource), "group") == 0) {
		return;
	}

	auto addResult = setupExistingSource(obs_source_get_uuid(newSource), visible, false);
	if (!addResult.has_value()) {
		return;
	}

	OBSSceneItem item = addResult.value();

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	std::string scene_name = obs_source_get_name(main->GetCurrentSceneSource());
	auto undo = [scene_name, main](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_name(data.c_str());
		obs_source_remove(source);

		OBSSourceAutoRelease scene_source = obs_get_source_by_name(scene_name.c_str());
		main->SetCurrentScene(scene_source.Get(), true);
	};
	OBSDataAutoRelease wrapper = obs_data_create();
	obs_data_set_string(wrapper, "id", id);
	obs_data_set_int(wrapper, "item_id", obs_sceneitem_get_id(item));
	obs_data_set_string(wrapper, "name", ui->newSourceName->text().toUtf8().constData());
	obs_data_set_bool(wrapper, "visible", visible);

	auto redo = [scene_name, main](const std::string &data) {
		OBSSourceAutoRelease scene_source = obs_get_source_by_name(scene_name.c_str());
		main->SetCurrentScene(scene_source.Get(), true);

		OBSDataAutoRelease dat = obs_data_create_from_json(data.c_str());

		auto createResult =
			setupNewSource(NULL, obs_data_get_string(dat, "id"), obs_data_get_string(dat, "name"));
		if (!createResult.has_value()) {
			return;
		}

		OBSSource source = createResult.value();

		auto addResult =
			setupExistingSource(obs_source_get_uuid(source), obs_data_get_bool(dat, "visible"), false);
		if (!addResult.has_value()) {
			return;
		}

		OBSSceneItem item = addResult.value();

		obs_sceneitem_set_id(item, (int64_t)obs_data_get_int(dat, "item_id"));
	};
	undo_s.add_action(QTStr("Undo.Add").arg(ui->newSourceName->text()), undo, redo,
			  std::string(obs_source_get_name(newSource)), std::string(obs_data_get_json(wrapper)));

	main->CreatePropertiesWindow(newSource);

	close();
}

void OBSBasicSourceSelect::addExisting(std::string uuid, bool visible)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
	if (!source) {
		return;
	}

	QString name = obs_source_get_name(source);
	setupExistingSource(uuid, visible, false);

	OBSBasic *main = OBSBasic::Get();
	const char *scene_name = obs_source_get_name(main->GetCurrentSceneSource());

	auto undo = [scene_name, main](const std::string &) {
		OBSSourceAutoRelease scene_source = obs_get_source_by_name(scene_name);
		main->SetCurrentScene(scene_source.Get(), true);

		OBSSceneAutoRelease scene = obs_get_scene_by_name(scene_name);
		OBSSceneItem item;
		auto cb = [](obs_scene_t *, obs_sceneitem_t *sceneitem, void *data) {
			OBSSceneItem &last = *static_cast<OBSSceneItem *>(data);
			last = sceneitem;
			return true;
		};
		obs_scene_enum_items(scene, cb, &item);

		obs_sceneitem_remove(item);
	};

	auto redo = [scene_name, main, uuid, visible](const std::string &) {
		OBSSourceAutoRelease scene_source = obs_get_source_by_name(scene_name);
		main->SetCurrentScene(scene_source.Get(), true);

		setupExistingSource(uuid, visible, false);
	};

	undo_s.add_action(QTStr("Undo.Add").arg(name), undo, redo, "", "");
}

void OBSBasicSourceSelect::on_createNewSource_clicked(bool)
{
	createNew();
}

void OBSBasicSourceSelect::addSelectedSources()
{
	if (selectedItems.size() == 0) {
		return;
	}

	bool visible = ui->sourceVisible->isChecked();

	for (auto &uuid : selectedItems) {
		addExisting(uuid, visible);
	}
	close();
}

void OBSBasicSourceSelect::handleSourceCreated()
{
	refreshSources();

	if (selectedTypeId.compare(RECENT_TYPE_ID) == 0) {
		updateExistingSources(RECENT_LIST_LIMIT);
	} else {
		updateExistingSources();
	}
}

void OBSBasicSourceSelect::handleSourceRemoved(QString uuid)
{
	refreshSources();

	removeSelectedItem(uuid.toStdString());

	if (selectedTypeId.compare(RECENT_TYPE_ID) == 0) {
		updateExistingSources(RECENT_LIST_LIMIT);
	} else {
		updateExistingSources();
	}
}

void OBSBasicSourceSelect::sourceTypeSelected(QListWidgetItem *current, QListWidgetItem *)
{
	clearSelectedItems();

	QVariant unversionedIdData = current->data(UNVERSIONED_ID_ROLE);
	QVariant deprecatedData = current->data(DEPRECATED_ROLE);

	if (unversionedIdData.toString().compare(RECENT_TYPE_ID) == 0) {
		selectedTypeId = RECENT_TYPE_ID;
		ui->createNewFrame->setVisible(false);
		updateExistingSources(RECENT_LIST_LIMIT);
		return;
	}

	QString type = unversionedIdData.toString();
	if (type.compare(selectedTypeId) == 0) {
		return;
	}

	ui->createNewFrame->setVisible(true);

	bool isDeprecatedType = deprecatedData.toBool();
	ui->deprecatedCreateLabel->setVisible(isDeprecatedType);

	selectedTypeId = type;

	QString placeHolderText{getDisplayNameForSourceType(selectedTypeId)};

	QString text{placeHolderText};
	int i = 2;
	OBSSourceAutoRelease source = nullptr;
	while ((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
		text = QString("%1 %2").arg(placeHolderText).arg(i++);
	}

	ui->newSourceName->setText(text);

	updateExistingSources();

	if (existingFlowLayout->count() == 0) {
		QLabel *noExisting = new QLabel();
		noExisting->setText(
			QTStr("Basic.SourceSelect.NoExisting").arg(getDisplayNameForSourceType(selectedTypeId)));
		noExisting->setProperty("class", "text-muted");
		existingFlowLayout->addWidget(noExisting);
	}
}
