/******************************************************************************
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

#include "SceneList.hpp"

#include "OBSBasic.hpp"
#include <components/SceneTree.hpp>
#include <widgets/OBSProjector.hpp>

#include <Idian/Utils.hpp>

#include <QVBoxLayout>

static constexpr int kIconSize = 16;

namespace OBS {
enum class SceneDataRole {
	uuid = Qt::UserRole,
};
}

SceneList::SceneList(QWidget *parent) : QFrame(parent)
{
	setWindowTitle("Scene List (New)");
	setLayout(new QVBoxLayout(this));

	layout()->setContentsMargins(0, 0, 0, 0);

	sceneTree = new SceneTree(this);

	connect(sceneTree, &QListWidget::itemSelectionChanged, this, &SceneList::onSceneSelectionChanged);
	connect(sceneTree, &SceneTree::scenesReordered, this, &SceneList::onSceneSelectionChanged);
	connect(sceneTree->itemDelegate(), &QAbstractItemDelegate::closeEditor, this, &SceneList::onItemEditorClosed);
	connect(sceneTree, &QListWidget ::itemDoubleClicked, this, &SceneList::onItemDoubleClicked);

	scenesToolbar = new QToolBar(this);
	scenesToolbar->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	scenesToolbar->setIconSize(QSize(kIconSize, kIconSize));
	scenesToolbar->setFloatable(false);
	scenesToolbar->setMovable(false);
	scenesToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

	actionAddScene = new QAction(this);
	actionAddScene->setObjectName("actionAddScene");
	actionAddScene->setText(QTStr("Add"));
	actionAddScene->setToolTip(QTStr("AddScene"));
	QIcon addIcon;
	addIcon.addFile(QString::fromUtf8(":/res/images/plus.svg"), QSize(kIconSize, kIconSize), QIcon::Mode::Normal,
			QIcon::State::Off);
	actionAddScene->setIcon(addIcon);

	actionSceneFilters = new QAction(this);
	actionSceneFilters->setObjectName("actionSceneFilters");
	actionSceneFilters->setText(QTStr("SceneFilters"));
	actionSceneFilters->setToolTip(QTStr("SceneFilters"));
	QIcon filtersIcon;
	filtersIcon.addFile(QString::fromUtf8(":/res/images/filter.svg"), QSize(kIconSize, kIconSize),
			    QIcon::Mode::Normal, QIcon::State::Off);
	actionSceneFilters->setIcon(filtersIcon);

	actionMoveUp = new QAction(this);
	actionMoveUp->setObjectName("actionMoveSceneUp");
	actionMoveUp->setText(QTStr("MoveUp"));
	actionMoveUp->setToolTip(QTStr("MoveSceneUp"));
	QIcon moveUpIcon;
	moveUpIcon.addFile(QString::fromUtf8(":/res/images/up.svg"), QSize(kIconSize, kIconSize), QIcon::Mode::Normal,
			   QIcon::State::Off);
	actionMoveUp->setIcon(moveUpIcon);

	actionMoveDown = new QAction(this);
	actionMoveDown->setObjectName("actionMoveSceneDown");
	actionMoveDown->setText(QTStr("MoveDown"));
	actionMoveDown->setToolTip(QTStr("MoveSceneDown"));
	QIcon moveDownIcon;
	moveDownIcon.addFile(QString::fromUtf8(":/res/images/down.svg"), QSize(kIconSize, kIconSize),
			     QIcon::Mode::Normal, QIcon::State::Off);
	actionMoveDown->setIcon(moveDownIcon);

	auto *spacer = new QWidget(scenesToolbar);
	spacer->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

	actionRenameScene = new QAction(QTStr("Rename"), sceneTree);
	connect(actionRenameScene, &QAction::triggered, this, &SceneList::beginInlineRename);

	actionRemoveScene = new QAction(QTStr("Remove"), sceneTree);
	actionRemoveScene->setObjectName("actionRemoveScene");
	actionRemoveScene->setToolTip(QTStr("RemoveScene"));
	QIcon removeIcon;
	removeIcon.addFile(QString::fromUtf8(":/res/images/trash.svg"), QSize(kIconSize, kIconSize),
			   QIcon::Mode::Normal, QIcon::State::Off);
	actionRemoveScene->setIcon(removeIcon);

	actionRenameScene->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	actionRemoveScene->setShortcutContext(Qt::WidgetWithChildrenShortcut);

#ifdef __APPLE__
	actionRenameScene->setShortcut({Qt::Key_Return});
	actionRemoveScene->setShortcut({Qt::Key_Backspace});
#else
	actionRenameScene->setShortcut({Qt::Key_F2});
	actionRemoveScene->setShortcut({Qt::Key_Delete});
#endif
	addAction(actionRenameScene);
	addAction(actionRemoveScene);

	optionsButton = new QPushButton(scenesToolbar);
	optionsButton->setText(QTStr("Basic.AudioMixer.Options"));
	idian::Utils::addClass(optionsButton, "toolbar-button");
	idian::Utils::addClass(optionsButton, "text-bold");

	recreateOptionsMenu();

	scenesToolbar->addAction(actionAddScene);
	scenesToolbar->addAction(actionRemoveScene);
	scenesToolbar->addSeparator();
	scenesToolbar->addAction(actionSceneFilters);
	scenesToolbar->addSeparator();
	scenesToolbar->addAction(actionMoveUp);
	scenesToolbar->addAction(actionMoveDown);
	scenesToolbar->addSeparator();
	scenesToolbar->addWidget(spacer);
	scenesToolbar->addSeparator();

	scenesToolbar->addWidget(optionsButton);

	QWidget *addButtonWidget = scenesToolbar->widgetForAction(actionAddScene);
	idian::Utils::addClass(addButtonWidget, "icon-plus");
	QWidget *removeButtonWidget = scenesToolbar->widgetForAction(actionRemoveScene);
	idian::Utils::addClass(removeButtonWidget, "icon-trash");
	QWidget *filterButtonWidget = scenesToolbar->widgetForAction(actionSceneFilters);
	idian::Utils::addClass(filterButtonWidget, "icon-filter");
	QWidget *moveUpButtonWidget = scenesToolbar->widgetForAction(actionMoveUp);
	idian::Utils::addClass(moveUpButtonWidget, "icon-up");
	QWidget *moveDownButtonWidget = scenesToolbar->widgetForAction(actionMoveDown);
	idian::Utils::addClass(moveDownButtonWidget, "icon-down");

	layout()->addWidget(sceneTree);
	layout()->addWidget(scenesToolbar);

	updateLayoutFromSettings();

	signalHandlers.reserve(6);

	connect(actionAddScene, &QAction::triggered, this, &SceneList::triggerCreateScene);
	connect(actionRemoveScene, &QAction::triggered, this, &SceneList::triggerDeleteForSelected);
	connect(actionSceneFilters, &QAction::triggered, this, &SceneList::openFiltersForSelectedScene);
	connect(actionMoveUp, &QAction::triggered, this, &SceneList::triggerMoveSelectedUp);
	connect(actionMoveDown, &QAction::triggered, this, &SceneList::triggerMoveSelectedDown);

	setContextMenuPolicy(Qt::CustomContextMenu);
	connect(this, &QWidget::customContextMenuRequested, this, &SceneList::onContextMenuRequested);

	OBSBasic *main = OBSBasic::Get();
	connect(main, &OBSBasic::userSettingChanged, this,
		[this](const std::string &category, const std::string &name) {
			if (category == "BasicWindow" && name == "gridMode") {
				updateLayoutFromSettings();
			}
		});
}

SceneList::~SceneList()
{
	attachMediator(nullptr);
	signalHandlers.clear();
}

void SceneList::createContextMenu()
{
	if (contextMenu) {
		contextMenu->deleteLater();
	}

	OBSBasic *main = OBSBasic::Get();

	contextMenu = new QMenu(this);
	contextMenu->setAttribute(Qt::WA_DeleteOnClose);

	contextMenu->addAction(QTStr("AddScene") + "...", this, &SceneList::triggerCreateScene);
	contextMenu->addSeparator();

	bool isGridMode = config_get_bool(App()->GetUserConfig(), "BasicWindow", "gridMode");
	QString layoutModeTitle = isGridMode ? QTStr("Basic.Main.ListMode") : QTStr("Basic.Main.GridMode");

	contextMenu->addAction(layoutModeTitle, main, [main]() { main->toggleSceneListLayout(); });
}

void SceneList::createContextMenuForItem(QListWidgetItem *item)
{
	auto dataUuid = item->data(static_cast<int>(OBS::SceneDataRole::uuid));
	QString uuid = dataUuid.toString();

	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	OBSScene scene = obs_scene_from_source(source);

	if (!scene) {
		return;
	}

	if (contextMenu) {
		contextMenu->deleteLater();
	}

	focusContextIndex = sceneTree->indexFromItem(item);

	contextMenu = new QMenu(this);
	contextMenu->setAttribute(Qt::WA_DeleteOnClose);

	contextMenu->addAction(QTStr("AddScene") + "...", this, &SceneList::triggerCreateScene);
	contextMenu->addSeparator();

	auto *main = OBSBasic::Get();

	auto *copyFilters = new QAction(QTStr("Copy.Filters"), this);
	copyFilters->setEnabled(obs_source_filter_count(source) > 0);
	connect(copyFilters, &QAction::triggered, main, [main, uuid]() { main->copyFiltersFromSource(uuid); });
	auto *pasteFilters = new QAction(QTStr("Paste.Filters"), this);
	pasteFilters->setEnabled(main->canPasteFilters());
	connect(pasteFilters, &QAction::triggered, main, [main, uuid]() { main->pasteFiltersToSource(uuid); });

	contextMenu->addAction(QTStr("Duplicate"), this, [this, scene]() { mediator->requestDuplicateScene(scene); });
	contextMenu->addAction(copyFilters);
	contextMenu->addAction(pasteFilters);
	contextMenu->addSeparator();
	contextMenu->addAction(actionRenameScene);
	contextMenu->addAction(actionRemoveScene);
	contextMenu->addSeparator();

	auto *order = new QMenu(QTStr("Basic.MainMenu.Edit.Order"), this);
	order->addAction(QTStr("Basic.MainMenu.Edit.Order.MoveUp"), this,
			 [this, scene]() { mediator->reorderSceneUp(scene); });
	order->addAction(QTStr("Basic.MainMenu.Edit.Order.MoveDown"), this,
			 [this, scene]() { mediator->reorderSceneDown(scene); });
	order->addSeparator();
	order->addAction(QTStr("Basic.MainMenu.Edit.Order.MoveToTop"), this,
			 [this, scene]() { mediator->reorderSceneToTop(scene); });
	order->addAction(QTStr("Basic.MainMenu.Edit.Order.MoveToBottom"), this,
			 [this, scene]() { mediator->reorderSceneToBottom(scene); });
	contextMenu->addMenu(order);

	contextMenu->addSeparator();

	auto *sceneProjectorMenu = new QMenu(QTStr("Projector.Open.Scene"));
	QList<QString> projectors = OBSBasic::GetProjectorMenuMonitorsFormatted();
	for (int i = 0; i < projectors.size(); i++) {
		const QString &actionName = projectors[i];
		QAction *action = sceneProjectorMenu->addAction(actionName, main,
								[main, uuid]() { main->openProjectorForUuid(uuid); });
		action->setProperty("monitor", i);
	}
	sceneProjectorMenu->addSeparator();
	sceneProjectorMenu->addAction(QTStr("Projector.Window"), main,
				      [main, uuid]() { main->openWindowedProjectorForUuid(uuid); });

	contextMenu->addMenu(sceneProjectorMenu);
	contextMenu->addSeparator();

	contextMenu->addAction(QTStr("Screenshot.Scene"), main, [main, uuid]() { main->screenshotScene(uuid); });
	contextMenu->addSeparator();
	contextMenu->addAction(QTStr("Filters"), this, [scene]() {
		OBSSource source = obs_scene_get_source(scene);
		if (!source) {
			return;
		}

		auto *main = OBSBasic::Get();
		main->CreateFiltersWindow(source);
	});

	contextMenu->addSeparator();

	auto *perSceneTransitionMenu = main->CreatePerSceneTransitionMenu();
	contextMenu->addMenu(perSceneTransitionMenu);

	/* ---------------------- */

	QAction *multiviewAction = contextMenu->addAction(QTStr("ShowInMultiview"));

	OBSScene previewScene = mediator->getPreviewScene();
	OBSSource previewSource = obs_scene_get_source(previewScene);
	OBSDataAutoRelease previewData = obs_source_get_private_settings(previewSource);

	obs_data_set_default_bool(previewData, "show_in_multiview", true);
	bool show = obs_data_get_bool(previewData, "show_in_multiview");

	multiviewAction->setCheckable(true);
	multiviewAction->setChecked(show);

	OBSData safePreviewData = previewData.Get();
	connect(multiviewAction, &QAction::triggered, multiviewAction, [safePreviewData]() {
		bool show = obs_data_get_bool(safePreviewData, "show_in_multiview");
		obs_data_set_bool(safePreviewData, "show_in_multiview", !show);
		OBSProjector::UpdateMultiviewProjectors();
	});
}

void SceneList::recreateOptionsMenu()
{
	if (optionsMenu) {
		optionsMenu->deleteLater();
	}

	optionsMenu = new QMenu(this);

	OBSBasic *main = OBSBasic::Get();

	bool isGridMode = config_get_bool(App()->GetUserConfig(), "BasicWindow", "gridMode");
	QString layoutModeTitle = isGridMode ? QTStr("Basic.Main.ListMode") : QTStr("Basic.Main.GridMode");

	optionsMenu->addAction(layoutModeTitle, main, [main]() { main->toggleSceneListLayout(); });

	optionsButton->setMenu(optionsMenu);
}

void SceneList::applyGridMode(bool enable)
{
	sceneTree->SetGridMode(enable);

	recreateOptionsMenu();
}

void SceneList::onContextMenuRequested(const QPoint &pos)
{
	QListWidgetItem *item = sceneTree->itemAt(pos);
	if (item) {
		createContextMenuForItem(item);
	} else {
		createContextMenu();
	}

	contextMenu->popup(mapToGlobal(pos));
}

void SceneList::triggerCreateScene()
{
	mediator->requestCreateScene();
}

void SceneList::openFiltersForSelectedScene()
{
	OBSScene scene = mediator->getPreviewScene();
	if (!scene) {
		return;
	}

	OBSSource source = obs_scene_get_source(scene);

	auto *main = OBSBasic::Get();
	main->CreateFiltersWindow(source);
}

void SceneList::triggerDeleteForSelected()
{
	OBSScene scene = mediator->getPreviewScene();
	if (!scene) {
		return;
	}

	triggerDeleteScene(scene);
}

void SceneList::triggerDuplicateForSelected()
{
	OBSScene scene = mediator->getPreviewScene();
	if (!scene) {
		return;
	}

	triggerDuplicateScene(scene);
}

void SceneList::triggerMoveSelectedToTop()
{
	OBSScene scene = mediator->getPreviewScene();
	if (!scene) {
		return;
	}

	mediator->reorderSceneToTop(scene);
}

void SceneList::triggerMoveSelectedToBottom()
{
	OBSScene scene = mediator->getPreviewScene();
	if (!scene) {
		return;
	}

	mediator->reorderSceneToBottom(scene);
}

void SceneList::triggerMoveSelectedUp()
{
	OBSScene scene = mediator->getPreviewScene();
	if (!scene) {
		return;
	}

	mediator->reorderSceneUp(scene);
}

void SceneList::triggerMoveSelectedDown()
{
	OBSScene scene = mediator->getPreviewScene();
	if (!scene) {
		return;
	}

	mediator->reorderSceneDown(scene);
}

void SceneList::triggerDeleteScene(const OBSScene &scene)
{
	mediator->requestDeleteScene(scene);
}

void SceneList::applySelectionFromMediator(const QString &uuid)
{
	QSignalBlocker block(sceneTree);

	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (!source) {
		return;
	}

	int scrollPosition = sceneTree->verticalScrollBar()->value();

	for (int i = 0; i < sceneTree->count(); ++i) {
		QListWidgetItem *item = sceneTree->item(i);

		QVariant itemUuidData = item->data(static_cast<int>(OBS::SceneDataRole::uuid));
		QString itemUuid = itemUuidData.toString();
		if (itemUuid == uuid) {
			if (item->isSelected()) {
				break;
			}

			sceneTree->setCurrentItem(item);
			break;
		}
	}

	sceneTree->verticalScrollBar()->setValue(scrollPosition);
}

void SceneList::rebuildFromMediator()
{
	auto scenes = mediator->getScenes();

	int scrollPosition = sceneTree->verticalScrollBar()->value();

	QSignalBlocker block(sceneTree);
	sceneTree->clear();
	focusContextIndex = QPersistentModelIndex();

	for (std::string &uuid : scenes) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.c_str());
		OBSScene scene = obs_scene_from_source(source);

		if (!scene) {
			continue;
		}

		const char *name = obs_source_get_name(source);

		auto *item = new QListWidgetItem(QString::fromUtf8(name));
		item->setData(static_cast<int>(OBS::SceneDataRole::uuid), QString::fromUtf8(uuid));

		sceneTree->addItem(item);

		if (scene == mediator->getPreviewScene()) {
			sceneTree->setCurrentItem(item);
			focusContextIndex = sceneTree->indexFromItem(item);
		}
	}

	sceneTree->verticalScrollBar()->setValue(scrollPosition);
}

void SceneList::triggerDuplicateScene(const OBSScene &scene)
{
	mediator->requestDuplicateScene(scene);
}

void SceneList::beginInlineRename()
{
	if (!focusContextIndex.isValid()) {
		return;
	}

	if (sceneTree->isPersistentEditorOpen(focusContextIndex)) {
		return;
	}

	Qt::ItemFlags flags = focusContextIndex.flags();

	QListWidgetItem *item = sceneTree->itemFromIndex(focusContextIndex);
	item->setFlags(flags | Qt::ItemIsEditable);
	sceneTree->editItem(item);
	item->setFlags(flags);
}

void SceneList::onItemDoubleClicked(QListWidgetItem *item)
{
	if (!item) {
		return;
	}

	auto *main = OBSBasic::Get();
	if (main->IsPreviewProgramMode()) {
		auto uuidData = item->data(static_cast<int>(OBS::SceneDataRole::uuid));
		QString uuid = uuidData.toString();

		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
		OBSScene scene = obs_scene_from_source(source);

		if (!scene) {
			return;
		}

		bool doubleClickSwitch =
			config_get_bool(App()->GetUserConfig(), "BasicWindow", "TransitionOnDoubleClick");

		if (doubleClickSwitch) {
			main->TransitionToScene(scene);
		}
	}
}

void SceneList::onSceneSelectionChanged()
{
	focusContextIndex = QPersistentModelIndex();
	auto selectedItems = sceneTree->selectedItems();
	if (selectedItems.count() <= 0) {
		return;
	}

	if (selectedItems.count() > 1) {
		// How did multiple items get selected?
		return;
	}

	QListWidgetItem *item = selectedItems.constFirst();
	focusContextIndex = sceneTree->indexFromItem(item);

	QVariant uuidData = item->data(static_cast<int>(OBS::SceneDataRole::uuid));
	QString uuid = uuidData.toString();

	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	OBSScene scene = obs_scene_from_source(source);
	if (!scene) {
		return;
	}

	emit sceneSelected(uuid);
}

void SceneList::onSceneDropped()
{
	int count = sceneTree->count();

	for (int i = 0; i < count; ++i) {
		QListWidgetItem *item = sceneTree->item(i);

		auto uuidData = item->data(static_cast<int>(OBS::SceneDataRole::uuid));
		QString uuid = uuidData.toString();

		OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
		OBSScene scene = obs_scene_from_source(source);
		if (!scene) {
			continue;
		}

		mediator->reorderSceneToIndex(scene, i);
	}

	emit sceneReorderFinished();
}

void SceneList::onItemEditorClosed(QWidget *editor)
{
	if (!focusContextIndex.isValid()) {
		return;
	}

	auto *lineEdit = qobject_cast<QLineEdit *>(editor);
	if (!lineEdit) {
		return;
	}

	QString newName = lineEdit->text();

	auto uuidData = focusContextIndex.data(static_cast<int>(OBS::SceneDataRole::uuid));

	QString uuid = uuidData.toString();
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	OBSScene scene = obs_scene_from_source(source);
	if (!scene) {
		return;
	}

	const char *oldName = obs_source_get_name(source);
	if (oldName != newName) {
		mediator->requestRenameScene(scene, newName);
	}
}

void SceneList::onSceneRenamed(const QString &uuid)
{
	OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
	if (!source) {
		return;
	}

	for (int i = 0; i < sceneTree->count(); ++i) {
		QListWidgetItem *item = sceneTree->item(i);

		QVariant itemUuid = item->data(static_cast<int>(OBS::SceneDataRole::uuid));
		if (itemUuid == uuid) {
			const char *name = obs_source_get_name(source);

			item->setText(name);
			break;
		}
	}
}

void SceneList::updateLayoutFromSettings()
{
	bool isGridMode = config_get_bool(App()->GetUserConfig(), "BasicWindow", "gridMode");

	applyGridMode(isGridMode);
}

void SceneList::attachMediator(OBS::CanvasMediator *mediator)
{
	if (this->mediator == mediator) {
		return;
	}

	// Disconnect old signals
	for (auto &connection : connections) {
		disconnect(connection);
	}

	connections.clear();

	this->mediator = mediator;

	if (!mediator) {
		return;
	}

	rebuildFromMediator();

	QPointer<OBS::CanvasMediator> safeMediator = mediator;
	// Set up signals for new mediator
	// Inbound signals
	connections.push_back(
		connect(safeMediator, &OBS::CanvasMediator::sceneAdded, this, &SceneList::rebuildFromMediator));
	connections.push_back(
		connect(safeMediator, &OBS::CanvasMediator::sceneRemoved, this, &SceneList::rebuildFromMediator));
	connections.push_back(
		connect(safeMediator, &OBS::CanvasMediator::sceneOrderChanged, this, &SceneList::rebuildFromMediator));
	connections.push_back(connect(safeMediator, &OBS::CanvasMediator::previewChanged, this,
				      &SceneList::applySelectionFromMediator));
	connections.push_back(
		connect(safeMediator, &OBS::CanvasMediator::sceneRenamed, this, &SceneList::onSceneRenamed));

	// Outbound signals
	connections.push_back(connect(sceneTree, &SceneTree::scenesReordered, this, &SceneList::onSceneDropped));
	connections.push_back(connect(this, &SceneList::sceneReorderFinished, safeMediator,
				      &OBS::CanvasMediator::commitSceneOrderChanges));
	connections.push_back(
		connect(this, &SceneList::sceneSelected, safeMediator, [safeMediator](const QString &uuid) {
			OBSSourceAutoRelease source = obs_get_source_by_uuid(uuid.toUtf8().constData());
			OBSScene scene = obs_scene_from_source(source);

			if (!source || !scene) {
				return;
			}

			safeMediator->setPreviewScene(scene);
		}));
}
