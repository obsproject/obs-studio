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

#include "OBSBasicSourceSelect.hpp"

#include <QMessageBox>
#include <QList>

#include "qt-wrappers.hpp"
#include "OBSApp.hpp"

#include <utility/ThumbnailManager.hpp>

#include "moc_OBSBasicSourceSelect.cpp"

struct AddSourceData {
	// Input data
	obs_source_t *source;
	bool visible;
	obs_transform_info *transform = nullptr;
	obs_sceneitem_crop *crop = nullptr;
	obs_blending_method *blend_method = nullptr;
	obs_blending_type *blend_mode = nullptr;

	// Return data
	obs_sceneitem_t *scene_item = nullptr;
};

namespace {
const char *getSourceDisplayName(QString type)
{
	std::string typeId = type.toStdString();
	const char *unversionedId = typeId.c_str();

	if (strcmp(unversionedId, "scene") == 0) {
		return Str("Basic.Scene");
	}
	const char *id = obs_get_latest_input_type_id(unversionedId);
	return obs_source_get_display_name(id);
}
} // namespace

char *getNewSourceName(const char *name, const char *format)
{
	struct dstr new_name = {0};
	int inc = 0;

	dstr_copy(&new_name, name);

	for (;;) {
		OBSSourceAutoRelease existing_source = obs_get_source_by_name(new_name.array);
		if (!existing_source) {
			break;
		}

		dstr_printf(&new_name, format, name, ++inc + 1);
	}

	return new_name.array;
}

static void AddSource(void *_data, obs_scene_t *scene)
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

	obs_sceneitem_set_visible(sceneitem, data->visible);

	data->scene_item = sceneitem;
}

static void AddExisting(OBSSource source, bool visible, bool duplicate, obs_transform_info *transform,
			obs_sceneitem_crop *crop, obs_blending_method *blend_method, obs_blending_type *blend_mode)
{
	OBSBasic *main = OBSBasic::Get();
	OBSScene scene = main->GetCurrentScene();
	if (!scene) {
		return;
	}

	if (duplicate) {
		OBSSource from = source;
		char *new_name = getNewSourceName(obs_source_get_name(source), "%s %d");
		source = obs_source_duplicate(from, new_name, false);
		obs_source_release(source);
		bfree(new_name);

		if (!source) {
			return;
		}
	}

	AddSourceData data;
	data.source = source;
	data.visible = visible;
	data.transform = transform;
	data.crop = crop;
	data.blend_method = blend_method;
	data.blend_mode = blend_mode;

	obs_enter_graphics();
	obs_scene_atomic_update(scene, AddSource, &data);
	obs_leave_graphics();
}

static void AddExisting(const char *name, bool visible, bool duplicate, obs_transform_info *transform,
			obs_sceneitem_crop *crop, obs_blending_method *blend_method, obs_blending_type *blend_mode)
{
	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source) {
		AddExisting(source.Get(), visible, duplicate, transform, crop, blend_method, blend_mode);
	}
}

bool AddNew(QWidget *parent, const char *id, const char *name, const bool visible, OBSSource &newSource,
	    OBSSceneItem &newSceneItem)
{
	OBSBasic *main = OBSBasic::Get();
	OBSScene scene = main->GetCurrentScene();
	bool success = false;
	if (!scene) {
		return false;
	}

	OBSSourceAutoRelease source = obs_get_source_by_name(name);
	if (source && parent) {
		OBSMessageBox::information(parent, QTStr("NameExists.Title"), QTStr("NameExists.Text"));

	} else {
		const char *v_id = obs_get_latest_input_type_id(id);
		source = obs_source_create(v_id, name, NULL, nullptr);

		if (source) {
			AddSourceData data;
			data.source = source;
			data.visible = visible;

			obs_enter_graphics();
			obs_scene_atomic_update(scene, AddSource, &data);
			obs_leave_graphics();

			newSource = source;
			newSceneItem = data.scene_item;

			// Set monitoring if source monitors by default
			uint32_t flags = obs_source_get_output_flags(source);
			if ((flags & OBS_SOURCE_MONITOR_BY_DEFAULT) != 0) {
				obs_source_set_monitoring_type(source, OBS_MONITORING_TYPE_MONITOR_ONLY);
			}

			success = true;
		}
	}

	return success;
}

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

	connect(resizeSignaler, &ResizeSignaler::resized, this, &OBSBasicSourceSelect::checkSourceVisibility);
	connect(ui->existingScrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this,
		&OBSBasicSourceSelect::checkSourceVisibility);
	connect(ui->existingScrollArea->horizontalScrollBar(), &QScrollBar::valueChanged, this,
		&OBSBasicSourceSelect::checkSourceVisibility);

	ui->createNewFrame->setVisible(false);

	getSourceTypes();
	getSources();

	updateExistingSources(16);

	connect(ui->sourceTypeList, &QListWidget::itemDoubleClicked, this, &OBSBasicSourceSelect::createNewSource);
	connect(ui->newSourceName, &QLineEdit::returnPressed, this, &OBSBasicSourceSelect::createNewSource);
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

void OBSBasicSourceSelect::checkSourceVisibility()
{
	QList<QAbstractButton *> buttons = sourceButtons->buttons();

	// Allow some room for previous/next rows to make scrolling a bit more seamless
	QRect scrollAreaRect(QPoint(0, 0), ui->existingScrollArea->size());
	scrollAreaRect.setTop(scrollAreaRect.top() - Thumbnail::cy);
	scrollAreaRect.setBottom(scrollAreaRect.bottom() + Thumbnail::cy);

	for (QAbstractButton *button : buttons) {
		SourceSelectButton *sourceButton = qobject_cast<SourceSelectButton *>(button->parent());
		if (sourceButton) {
			QRect buttonRect = button->rect();
			buttonRect.moveTo(button->mapTo(ui->existingScrollArea, buttonRect.topLeft()));

			if (scrollAreaRect.intersects(buttonRect)) {
				sourceButton->setPreload(true);
			} else {
				sourceButton->setPreload(false);
			}
		}
	}

	scrollAreaRect = QRect(QPoint(0, 0), ui->existingScrollArea->size());

	for (QAbstractButton *button : buttons) {
		SourceSelectButton *sourceButton = qobject_cast<SourceSelectButton *>(button->parent());
		if (sourceButton) {
			QRect buttonRect = button->rect();
			buttonRect.moveTo(button->mapTo(ui->existingScrollArea, buttonRect.topLeft()));

			if (scrollAreaRect.intersects(buttonRect)) {
				sourceButton->setRectVisible(true);
			} else {
				sourceButton->setRectVisible(false);
			}
		}
	}
}

void OBSBasicSourceSelect::getSources()
{
	sources.clear();

	obs_enum_sources(enumSourcesCallback, this);
	emit sourcesUpdated();
}

void OBSBasicSourceSelect::updateExistingSources(int limit)
{
	delete sourceButtons;
	sourceButtons = new QButtonGroup(this);
	sourceButtons->setExclusive(false);

	auto sourcesList = &sources;

	std::vector<obs_source_t *> reversedSources;
	bool isReverseListOrder = sourceTypeId.isNull();
	if (isReverseListOrder) {
		reversedSources = {sources.rbegin(), sources.rend()};
		sourcesList = &reversedSources;
	}

	QWidget *prevTabWidget = ui->sourceTypeList;
	int count = 0;
	for (obs_source_t *source : *sourcesList) {
		if (limit > 0 && count >= limit) {
			break;
		}

		const char *id = obs_source_get_unversioned_id(source);
		QString stringId = QString(id);

		if (stringId.compare("group") == 0) {
			continue;
		}

		if (sourceTypeId.compare(stringId) == 0 || sourceTypeId.isNull()) {
			SourceSelectButton *newButton = new SourceSelectButton(source, ui->existingListFrame);
			std::string name = obs_source_get_name(source);

			existingFlowLayout->addWidget(newButton);
			sourceButtons->addButton(newButton->getButton());

			if (!prevTabWidget) {
				setTabOrder(ui->existingListFrame, newButton->getButton());
			} else {
				setTabOrder(prevTabWidget, newButton->getButton());
			}

			prevTabWidget = newButton->getButton();

			count++;
		}
	}
	setTabOrder(prevTabWidget, ui->addExistingContainer);

	connect(sourceButtons, &QButtonGroup::buttonToggled, this, &OBSBasicSourceSelect::sourceButtonToggled);

	ui->existingListFrame->adjustSize();
	QTimer::singleShot(10, this, [this] { checkSourceVisibility(); });
}

bool OBSBasicSourceSelect::enumSourcesCallback(void *data, obs_source_t *source)
{
	if (obs_source_is_hidden(source)) {
		return true;
	}

	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect *>(data);

	window->sources.push_back(source);

	return true;
}

bool OBSBasicSourceSelect::enumGroupsCallback(void *data, obs_source_t *source)
{
	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect *>(data);
	const char *name = obs_source_get_name(source);
	const char *id = obs_source_get_unversioned_id(source);

	if (window->sourceTypeId.compare(QString(id)) == 0) {
		OBSBasic *main = OBSBasic::Get();
		OBSScene scene = main->GetCurrentScene();

		obs_sceneitem_t *existing = obs_scene_get_group(scene, name);
		if (!existing) {
			QPushButton *button = new QPushButton(name);
			connect(button, &QPushButton::clicked, window, &OBSBasicSourceSelect::addSelectedSources);
		}
	}

	return true;
}

void OBSBasicSourceSelect::OBSSourceAdded(void *data, calldata_t *calldata)
{
	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect *>(data);
	obs_source_t *source = (obs_source_t *)calldata_ptr(calldata, "source");

	QMetaObject::invokeMethod(window, "SourceAdded", Q_ARG(OBSSource, source));
}

void OBSBasicSourceSelect::getSourceTypes()
{
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

		QListWidgetItem *newItem = new QListWidgetItem();
		newItem->setData(Qt::DisplayRole, name);
		newItem->setData(UNVERSIONED_ID_ROLE, unversioned_type);

		QIcon icon;
		icon = main->GetSourceIcon(type);
		newItem->setIcon(icon);

		ui->sourceTypeList->addItem(newItem);
	}

	QListWidgetItem *newItem = new QListWidgetItem();
	newItem->setData(Qt::DisplayRole, Str("Basic.Scene"));
	newItem->setData(UNVERSIONED_ID_ROLE, "scene");

	QIcon icon;
	icon = main->GetSceneIcon();
	newItem->setIcon(icon);

	ui->sourceTypeList->addItem(newItem);

	ui->sourceTypeList->sortItems();

	QListWidgetItem *allSources = new QListWidgetItem();
	allSources->setData(Qt::DisplayRole, Str("Basic.SourceSelect.Recent"));
	allSources->setData(UNVERSIONED_ID_ROLE, QVariant());
	ui->sourceTypeList->insertItem(0, allSources);

	ui->sourceTypeList->setCurrentItem(allSources);
	ui->sourceTypeList->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);

	connect(ui->sourceTypeList, &QListWidget::currentItemChanged, this, &OBSBasicSourceSelect::sourceTypeSelected);
}

void OBSBasicSourceSelect::setSelectedSourceType(QListWidgetItem *item)
{
	setSelectedSource(nullptr);
	QLayout *layout = ui->existingListFrame->flowLayout();

	// Clear existing buttons when switching types
	QLayoutItem *child = nullptr;
	while ((child = layout->takeAt(0)) != nullptr) {
		if (child->widget()) {
			child->widget()->deleteLater();
		}
		delete child;
	}

	QVariant data = item->data(UNVERSIONED_ID_ROLE);

	if (data.isNull()) {
		setSelectedSource(nullptr);
		sourceTypeId.clear();
		ui->createNewFrame->setVisible(false);
		updateExistingSources(16);
		return;
	}

	QString type = data.toString();
	if (type.compare(sourceTypeId) == 0) {
		return;
	}

	ui->createNewFrame->setVisible(true);

	sourceTypeId = type;

	QString placeHolderText{QT_UTF8(getSourceDisplayName(sourceTypeId))};

	QString text{placeHolderText};
	int i = 2;
	OBSSourceAutoRelease source = nullptr;
	while ((source = obs_get_source_by_name(QT_TO_UTF8(text)))) {
		text = QString("%1 %2").arg(placeHolderText).arg(i++);
	}

	ui->newSourceName->setText(text);
	ui->newSourceName->selectAll();

	if (sourceTypeId.compare("scene") == 0) {
		OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
		OBSSource curSceneSource = main->GetCurrentSceneSource();

		delete sourceButtons;
		sourceButtons = new QButtonGroup(this);

		int count = main->ui->scenes->count();
		QWidget *prevTabItem = ui->sourceTypeList;
		for (int i = 0; i < count; i++) {
			QListWidgetItem *item = main->ui->scenes->item(i);
			OBSScene scene = GetOBSRef<OBSScene>(item);
			OBSSource sceneSource = obs_scene_get_source(scene);

			if (curSceneSource == sceneSource) {
				continue;
			}

			SourceSelectButton *newButton = new SourceSelectButton(sceneSource, ui->existingListFrame);
			existingFlowLayout->addWidget(newButton);
			sourceButtons->addButton(newButton->getButton());

			setTabOrder(prevTabItem, newButton->getButton());
			prevTabItem = newButton->getButton();
		}
		connect(sourceButtons, &QButtonGroup::buttonToggled, this, &OBSBasicSourceSelect::sourceButtonToggled);

		QTimer::singleShot(100, this, [this] { checkSourceVisibility(); });

		ui->createNewFrame->setVisible(false);

	} else if (sourceTypeId.compare("group") == 0) {
		obs_enum_sources(enumGroupsCallback, this);
	} else {
		updateExistingSources();
	}

	if (layout->count() == 0) {
		QLabel *noExisting = new QLabel();
		noExisting->setText(QTStr("Basic.SourceSelect.NoExisting").arg(getSourceDisplayName(sourceTypeId)));
		noExisting->setProperty("class", "text-muted");
		layout->addWidget(noExisting);
	}
}

void OBSBasicSourceSelect::OBSSourceRemoved(void *data, calldata_t *calldata)
{
	OBSBasicSourceSelect *window = static_cast<OBSBasicSourceSelect *>(data);
	obs_source_t *source = (obs_source_t *)calldata_ptr(calldata, "source");

	QMetaObject::invokeMethod(window, "SourceRemoved", Q_ARG(OBSSource, source));
}

void OBSBasicSourceSelect::sourceButtonToggled(QAbstractButton *button, bool checked)
{
	SourceSelectButton *buttonParent = dynamic_cast<SourceSelectButton *>(button->parentWidget());

	Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
	bool ctrlDown = (modifiers & Qt::ControlModifier);
	bool shiftDown = (modifiers & Qt::ShiftModifier);

	if (!buttonParent) {
		clearSelectedItems();
		return;
	}

	int selectedIndex = existingFlowLayout->indexOf(buttonParent);

	if (ctrlDown && !shiftDown) {
		if (checked) {
			addSelectedItem(buttonParent);
		} else {
			removeSelectedItem(buttonParent);
		}

		lastSelectedIndex = existingFlowLayout->indexOf(buttonParent);
		return;
	} else if (shiftDown) {
		if (!ctrlDown) {
			clearSelectedItems();
		}
		sourceButtons->blockSignals(true);
		int start = std::min(selectedIndex, lastSelectedIndex);
		int end = std::max(selectedIndex, lastSelectedIndex);
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
				entry->getButton()->setChecked(true);
				addSelectedItem(entry);
			}
		}
		sourceButtons->blockSignals(false);
	} else {
		lastSelectedIndex = existingFlowLayout->indexOf(buttonParent);

		bool reselectItem = selectedItems.size() > 1;
		clearSelectedItems();
		if (checked) {
			addSelectedItem(buttonParent);
		} else if (reselectItem) {
			button->setChecked(true);
			addSelectedItem(buttonParent);
		}
	}
}

void OBSBasicSourceSelect::sourceDropped(QString uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toStdString().c_str());
	if (source) {
		const char *name = obs_source_get_name(source);
		bool visible = ui->sourceVisible->isChecked();

		addExistingSource(name, visible);
	}
}

void OBSBasicSourceSelect::setSelectedSource(SourceSelectButton *button)
{
	clearSelectedItems();
	addSelectedItem(button);
}

void OBSBasicSourceSelect::addSelectedItem(SourceSelectButton *button)
{
	if (button == nullptr) {
		return;
	}

	auto it = std::find(selectedItems.begin(), selectedItems.end(), button);
	if (it == selectedItems.end()) {
		selectedItems.push_back(button);
		emit selectedItemsChanged();
	}
}

void OBSBasicSourceSelect::removeSelectedItem(SourceSelectButton *button)
{
	if (button == nullptr) {
		return;
	}

	auto it = std::find(selectedItems.begin(), selectedItems.end(), button);
	if (it != selectedItems.end()) {
		selectedItems.erase(it);
		emit selectedItemsChanged();
	}
}

void OBSBasicSourceSelect::clearSelectedItems()
{
	if (selectedItems.size() == 0) {
		return;
	}

	sourceButtons->blockSignals(true);
	for (auto &item : selectedItems) {
		item->getButton()->setChecked(false);
	}
	sourceButtons->blockSignals(false);
	selectedItems.clear();
	emit selectedItemsChanged();
}

void OBSBasicSourceSelect::createNewSource()
{
	bool visible = ui->sourceVisible->isChecked();

	if (ui->newSourceName->text().isEmpty()) {
		return;
	}

	if (sourceTypeId.isNull()) {
		return;
	}

	if (sourceTypeId.compare("scene") == 0) {
		return;
	}

	OBSSceneItem item;
	std::string sourceType = sourceTypeId.toStdString();
	const char *id = sourceType.c_str();
	if (!AddNew(this, id, QT_TO_UTF8(ui->newSourceName->text()), visible, newSource, item)) {
		return;
	}

	if (newSource && strcmp(obs_source_get_id(newSource.Get()), "group") != 0) {
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
			OBSSource source;
			OBSSceneItem item;
			AddNew(NULL, obs_data_get_string(dat, "id"), obs_data_get_string(dat, "name"),
			       obs_data_get_bool(dat, "visible"), source, item);
			obs_sceneitem_set_id(item, (int64_t)obs_data_get_int(dat, "item_id"));
		};
		undo_s.add_action(QTStr("Undo.Add").arg(ui->newSourceName->text()), undo, redo,
				  std::string(obs_source_get_name(newSource)), std::string(obs_data_get_json(wrapper)));

		main->CreatePropertiesWindow(newSource);
	}
	close();
}

void OBSBasicSourceSelect::addExistingSource(QString name, bool visible)
{
	OBSSourceAutoRelease source = obs_get_source_by_name(name.toStdString().c_str());
	if (source) {
		AddExisting(source.Get(), visible, false, nullptr, nullptr, nullptr, nullptr);

		OBSBasic *main = OBSBasic::Get();
		const char *scene_name = obs_source_get_name(main->GetCurrentSceneSource());

		auto undo = [scene_name, main](const std::string &) {
			obs_source_t *scene_source = obs_get_source_by_name(scene_name);
			main->SetCurrentScene(scene_source, true);
			obs_source_release(scene_source);

			obs_scene_t *scene = obs_get_scene_by_name(scene_name);
			OBSSceneItem item;
			auto cb = [](obs_scene_t *, obs_sceneitem_t *sceneitem, void *data) {
				OBSSceneItem &last = *static_cast<OBSSceneItem *>(data);
				last = sceneitem;
				return true;
			};
			obs_scene_enum_items(scene, cb, &item);

			obs_sceneitem_remove(item);
			obs_scene_release(scene);
		};

		auto redo = [scene_name, main, name, visible](const std::string &) {
			obs_source_t *scene_source = obs_get_source_by_name(scene_name);
			main->SetCurrentScene(scene_source, true);
			obs_source_release(scene_source);
			AddExisting(QT_TO_UTF8(name), visible, false, nullptr, nullptr, nullptr, nullptr);
		};

		undo_s.add_action(QTStr("Undo.Add").arg(name), undo, redo, "", "");
	}
}

void OBSBasicSourceSelect::on_createNewSource_clicked(bool)
{
	createNewSource();
}

void OBSBasicSourceSelect::addSelectedSources()
{
	if (selectedItems.size() == 0) {
		return;
	}

	bool visible = ui->sourceVisible->isChecked();

	for (auto &item : selectedItems) {
		QString sourceName = item->text();
		addExistingSource(sourceName, visible);
	}
	close();
}

void OBSBasicSourceSelect::sourceTypeSelected(QListWidgetItem *current, QListWidgetItem *)
{
	setSelectedSourceType(current);
}

void OBSBasicSourceSelect::SourcePaste(SourceCopyInfo &info, bool dup)
{
	OBSSource source = OBSGetStrongRef(info.weak_source);
	if (!source) {
		return;
	}

	AddExisting(source, info.visible, dup, &info.transform, &info.crop, &info.blend_method, &info.blend_mode);
}
