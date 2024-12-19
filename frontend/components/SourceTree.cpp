#include "SourceTree.hpp"
#include "SourceTreeDelegate.hpp"

#include <widgets/OBSBasic.hpp>

#include <QPainter>

#include "moc_SourceTree.cpp"

static inline OBSScene GetCurrentScene()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	return main->GetCurrentScene();
}

/* moves a scene item index (blame linux distros for using older Qt builds) */
static inline void MoveItem(QVector<OBSSceneItem> &items, int oldIdx, int newIdx)
{
	OBSSceneItem item = items[oldIdx];
	items.remove(oldIdx);
	items.insert(newIdx, item);
}

SourceTree::SourceTree(QWidget *parent_) : QListView(parent_)
{
	SourceTreeModel *stm_ = new SourceTreeModel(this);
	setModel(stm_);
	setStyleSheet(QString("*[bgColor=\"1\"]{background-color:rgba(255,68,68,33%);}"
			      "*[bgColor=\"2\"]{background-color:rgba(255,255,68,33%);}"
			      "*[bgColor=\"3\"]{background-color:rgba(68,255,68,33%);}"
			      "*[bgColor=\"4\"]{background-color:rgba(68,255,255,33%);}"
			      "*[bgColor=\"5\"]{background-color:rgba(68,68,255,33%);}"
			      "*[bgColor=\"6\"]{background-color:rgba(255,68,255,33%);}"
			      "*[bgColor=\"7\"]{background-color:rgba(68,68,68,33%);}"
			      "*[bgColor=\"8\"]{background-color:rgba(255,255,255,33%);}"));

	UpdateNoSourcesMessage();
	connect(App(), &OBSApp::StyleChanged, this, &SourceTree::UpdateNoSourcesMessage);
	connect(App(), &OBSApp::StyleChanged, this, &SourceTree::UpdateIcons);

	setItemDelegate(new SourceTreeDelegate(this));
}

void SourceTree::UpdateIcons()
{
	SourceTreeModel *stm = GetStm();
	stm->SceneChanged();
}

void SourceTree::SetIconsVisible(bool visible)
{
	SourceTreeModel *stm = GetStm();

	iconsVisible = visible;
	stm->SceneChanged();
}

void SourceTree::ResetWidgets()
{
	OBSScene scene = GetCurrentScene();

	SourceTreeModel *stm = GetStm();
	stm->UpdateGroupState(false);

	for (int i = 0; i < stm->items.count(); i++) {
		QModelIndex index = stm->createIndex(i, 0, nullptr);
		setIndexWidget(index, new SourceTreeItem(this, stm->items[i]));
	}
}

void SourceTree::UpdateWidget(const QModelIndex &idx, obs_sceneitem_t *item)
{
	setIndexWidget(idx, new SourceTreeItem(this, item));
}

void SourceTree::UpdateWidgets(bool force)
{
	SourceTreeModel *stm = GetStm();

	for (int i = 0; i < stm->items.size(); i++) {
		obs_sceneitem_t *item = stm->items[i];
		SourceTreeItem *widget = GetItemWidget(i);

		if (!widget) {
			UpdateWidget(stm->createIndex(i, 0), item);
		} else {
			widget->Update(force);
		}
	}
}

void SourceTree::SelectItem(obs_sceneitem_t *sceneitem, bool select)
{
	SourceTreeModel *stm = GetStm();
	int i = 0;

	for (; i < stm->items.count(); i++) {
		if (stm->items[i] == sceneitem)
			break;
	}

	if (i == stm->items.count())
		return;

	QModelIndex index = stm->createIndex(i, 0);
	if (index.isValid() && select != selectionModel()->isSelected(index))
		selectionModel()->select(index, select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}

void SourceTree::mouseDoubleClickEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton)
		QListView::mouseDoubleClickEvent(event);
}

void SourceTree::dropEvent(QDropEvent *event)
{
	if (event->source() != this) {
		QListView::dropEvent(event);
		return;
	}

	OBSBasic *main = OBSBasic::Get();

	OBSScene scene = GetCurrentScene();
	obs_source_t *scenesource = obs_scene_get_source(scene);
	SourceTreeModel *stm = GetStm();
	auto &items = stm->items;
	QModelIndexList indices = selectedIndexes();

	DropIndicatorPosition indicator = dropIndicatorPosition();
	int row = indexAt(event->position().toPoint()).row();
	bool emptyDrop = row == -1;

	if (emptyDrop) {
		if (!items.size()) {
			QListView::dropEvent(event);
			return;
		}

		row = items.size() - 1;
		indicator = QAbstractItemView::BelowItem;
	}

	/* --------------------------------------- */
	/* store destination group if moving to a  */
	/* group                                   */

	obs_sceneitem_t *dropItem = items[row]; /* item being dropped on */
	bool itemIsGroup = obs_sceneitem_is_group(dropItem);

	obs_sceneitem_t *dropGroup = itemIsGroup ? dropItem : obs_sceneitem_get_group(scene, dropItem);

	/* not a group if moving above the group */
	if (indicator == QAbstractItemView::AboveItem && itemIsGroup)
		dropGroup = nullptr;
	if (emptyDrop)
		dropGroup = nullptr;

	/* --------------------------------------- */
	/* remember to remove list items if        */
	/* dropping on collapsed group             */

	bool dropOnCollapsed = false;
	if (dropGroup) {
		obs_data_t *data = obs_sceneitem_get_private_settings(dropGroup);
		dropOnCollapsed = obs_data_get_bool(data, "collapsed");
		obs_data_release(data);
	}

	if (indicator == QAbstractItemView::BelowItem || indicator == QAbstractItemView::OnItem ||
	    indicator == QAbstractItemView::OnViewport)
		row++;

	if (row < 0 || row > stm->items.count()) {
		QListView::dropEvent(event);
		return;
	}

	/* --------------------------------------- */
	/* determine if any base group is selected */

	bool hasGroups = false;
	for (int i = 0; i < indices.size(); i++) {
		obs_sceneitem_t *item = items[indices[i].row()];
		if (obs_sceneitem_is_group(item)) {
			hasGroups = true;
			break;
		}
	}

	/* --------------------------------------- */
	/* if dropping a group, detect if it's     */
	/* below another group                     */

	obs_sceneitem_t *itemBelow;
	if (row == stm->items.count())
		itemBelow = nullptr;
	else
		itemBelow = stm->items[row];

	if (hasGroups) {
		if (!itemBelow || obs_sceneitem_get_group(scene, itemBelow) != dropGroup) {
			dropGroup = nullptr;
			dropOnCollapsed = false;
		}
	}

	/* --------------------------------------- */
	/* if dropping groups on other groups,     */
	/* disregard as invalid drag/drop          */

	if (dropGroup && hasGroups) {
		QListView::dropEvent(event);
		return;
	}

	/* --------------------------------------- */
	/* save undo data                          */
	std::vector<obs_source_t *> sources;
	for (int i = 0; i < indices.size(); i++) {
		obs_sceneitem_t *item = items[indices[i].row()];
		if (obs_sceneitem_get_scene(item) != scene)
			sources.push_back(obs_scene_get_source(obs_sceneitem_get_scene(item)));
	}
	if (dropGroup)
		sources.push_back(obs_sceneitem_get_source(dropGroup));
	OBSData undo_data = main->BackupScene(scene, &sources);

	/* --------------------------------------- */
	/* if selection includes base group items, */
	/* include all group sub-items and treat   */
	/* them all as one                         */

	if (hasGroups) {
		/* remove sub-items if selected */
		for (int i = indices.size() - 1; i >= 0; i--) {
			obs_sceneitem_t *item = items[indices[i].row()];
			obs_scene_t *itemScene = obs_sceneitem_get_scene(item);

			if (itemScene != scene) {
				indices.removeAt(i);
			}
		}

		/* add all sub-items of selected groups */
		for (int i = indices.size() - 1; i >= 0; i--) {
			obs_sceneitem_t *item = items[indices[i].row()];

			if (obs_sceneitem_is_group(item)) {
				for (int j = items.size() - 1; j >= 0; j--) {
					obs_sceneitem_t *subitem = items[j];
					obs_sceneitem_t *subitemGroup = obs_sceneitem_get_group(scene, subitem);

					if (subitemGroup == item) {
						QModelIndex idx = stm->createIndex(j, 0);
						indices.insert(i + 1, idx);
					}
				}
			}
		}
	}

	/* --------------------------------------- */
	/* build persistent indices                */

	QList<QPersistentModelIndex> persistentIndices;
	persistentIndices.reserve(indices.count());
	for (QModelIndex &index : indices)
		persistentIndices.append(index);
	std::sort(persistentIndices.begin(), persistentIndices.end());

	/* --------------------------------------- */
	/* move all items to destination index     */

	int r = row;
	for (auto &persistentIdx : persistentIndices) {
		int from = persistentIdx.row();
		int to = r;
		int itemTo = to;

		if (itemTo > from)
			itemTo--;

		if (itemTo != from) {
			stm->beginMoveRows(QModelIndex(), from, from, QModelIndex(), to);
			MoveItem(items, from, itemTo);
			stm->endMoveRows();
		}

		r = persistentIdx.row() + 1;
	}

	std::sort(persistentIndices.begin(), persistentIndices.end());
	int firstIdx = persistentIndices.front().row();
	int lastIdx = persistentIndices.back().row();

	/* --------------------------------------- */
	/* reorder scene items in back-end         */

	QVector<struct obs_sceneitem_order_info> orderList;
	obs_sceneitem_t *lastGroup = nullptr;
	int insertCollapsedIdx = 0;

	auto insertCollapsed = [&](obs_sceneitem_t *item) {
		struct obs_sceneitem_order_info info;
		info.group = lastGroup;
		info.item = item;

		orderList.insert(insertCollapsedIdx++, info);
	};

	using insertCollapsed_t = decltype(insertCollapsed);

	auto preInsertCollapsed = [](obs_scene_t *, obs_sceneitem_t *item, void *param) {
		(*reinterpret_cast<insertCollapsed_t *>(param))(item);
		return true;
	};

	auto insertLastGroup = [&]() {
		OBSDataAutoRelease data = obs_sceneitem_get_private_settings(lastGroup);
		bool collapsed = obs_data_get_bool(data, "collapsed");

		if (collapsed) {
			insertCollapsedIdx = 0;
			obs_sceneitem_group_enum_items(lastGroup, preInsertCollapsed, &insertCollapsed);
		}

		struct obs_sceneitem_order_info info;
		info.group = nullptr;
		info.item = lastGroup;
		orderList.insert(0, info);
	};

	auto updateScene = [&]() {
		struct obs_sceneitem_order_info info;

		for (int i = 0; i < items.size(); i++) {
			obs_sceneitem_t *item = items[i];
			obs_sceneitem_t *group;

			if (obs_sceneitem_is_group(item)) {
				if (lastGroup) {
					insertLastGroup();
				}
				lastGroup = item;
				continue;
			}

			if (!hasGroups && i >= firstIdx && i <= lastIdx)
				group = dropGroup;
			else
				group = obs_sceneitem_get_group(scene, item);

			if (lastGroup && lastGroup != group) {
				insertLastGroup();
			}

			lastGroup = group;

			info.group = group;
			info.item = item;
			orderList.insert(0, info);
		}

		if (lastGroup) {
			insertLastGroup();
		}

		obs_scene_reorder_items2(scene, orderList.data(), orderList.size());
	};

	using updateScene_t = decltype(updateScene);

	auto preUpdateScene = [](void *data, obs_scene_t *) {
		(*reinterpret_cast<updateScene_t *>(data))();
	};

	ignoreReorder = true;
	obs_scene_atomic_update(scene, preUpdateScene, &updateScene);
	ignoreReorder = false;

	/* --------------------------------------- */
	/* save redo data                          */

	OBSData redo_data = main->BackupScene(scene, &sources);

	/* --------------------------------------- */
	/* add undo/redo action                    */

	const char *scene_name = obs_source_get_name(scenesource);
	QString action_name = QTStr("Undo.ReorderSources").arg(scene_name);
	main->CreateSceneUndoRedoAction(action_name, undo_data, redo_data);

	/* --------------------------------------- */
	/* remove items if dropped in to collapsed */
	/* group                                   */

	if (dropOnCollapsed) {
		stm->beginRemoveRows(QModelIndex(), firstIdx, lastIdx);
		items.remove(firstIdx, lastIdx - firstIdx + 1);
		stm->endRemoveRows();
	}

	/* --------------------------------------- */
	/* update widgets and accept event         */

	UpdateWidgets(true);

	event->accept();
	event->setDropAction(Qt::CopyAction);

	QListView::dropEvent(event);
}

void SourceTree::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
	{
		QSignalBlocker sourcesSignalBlocker(this);
		SourceTreeModel *stm = GetStm();

		QModelIndexList selectedIdxs = selected.indexes();
		QModelIndexList deselectedIdxs = deselected.indexes();

		for (int i = 0; i < selectedIdxs.count(); i++) {
			int idx = selectedIdxs[i].row();
			obs_sceneitem_select(stm->items[idx], true);
		}

		for (int i = 0; i < deselectedIdxs.count(); i++) {
			int idx = deselectedIdxs[i].row();
			obs_sceneitem_select(stm->items[idx], false);
		}
	}
	QListView::selectionChanged(selected, deselected);
}

void SourceTree::NewGroupEdit(int row)
{
	if (!Edit(row)) {
		OBSBasic *main = OBSBasic::Get();
		main->undo_s.pop_disabled();

		blog(LOG_WARNING, "Uh, somehow the edit didn't process, this "
				  "code should never be reached.\nAnd by "
				  "\"never be reached\", I mean that "
				  "theoretically, it should be\nimpossible "
				  "for this code to be reached. But if this "
				  "code is reached,\nfeel free to laugh at "
				  "Lain, because apparently it is, in fact, "
				  "actually\npossible for this code to be "
				  "reached. But I mean, again, theoretically\n"
				  "it should be impossible. So if you see "
				  "this in your log, just know that\nit's "
				  "really dumb, and depressing. But at least "
				  "the undo/redo action is\nstill covered, so "
				  "in theory things *should* be fine. But "
				  "it's entirely\npossible that they might "
				  "not be exactly. But again, yea. This "
				  "really\nshould not be possible.");

		OBSData redoSceneData = main->BackupScene(GetCurrentScene());

		QString text = QTStr("Undo.GroupItems").arg("Unknown");
		main->CreateSceneUndoRedoAction(text, undoSceneData, redoSceneData);

		undoSceneData = nullptr;
	}
}

bool SourceTree::Edit(int row)
{
	SourceTreeModel *stm = GetStm();
	if (row < 0 || row >= stm->items.count())
		return false;

	QModelIndex index = stm->createIndex(row, 0);
	QWidget *widget = indexWidget(index);
	SourceTreeItem *itemWidget = reinterpret_cast<SourceTreeItem *>(widget);
	if (itemWidget->IsEditing()) {
#ifdef __APPLE__
		itemWidget->ExitEditMode(true);
#endif
		return false;
	}

	itemWidget->EnterEditMode();
	edit(index);
	return true;
}

bool SourceTree::MultipleBaseSelected() const
{
	SourceTreeModel *stm = GetStm();
	QModelIndexList selectedIndices = selectedIndexes();

	OBSScene scene = GetCurrentScene();

	if (selectedIndices.size() < 1) {
		return false;
	}

	for (auto &idx : selectedIndices) {
		obs_sceneitem_t *item = stm->items[idx.row()];
		if (obs_sceneitem_is_group(item)) {
			return false;
		}

		obs_scene *itemScene = obs_sceneitem_get_scene(item);
		if (itemScene != scene) {
			return false;
		}
	}

	return true;
}

bool SourceTree::GroupsSelected() const
{
	SourceTreeModel *stm = GetStm();
	QModelIndexList selectedIndices = selectedIndexes();

	OBSScene scene = GetCurrentScene();

	if (selectedIndices.size() < 1) {
		return false;
	}

	for (auto &idx : selectedIndices) {
		obs_sceneitem_t *item = stm->items[idx.row()];
		if (!obs_sceneitem_is_group(item)) {
			return false;
		}
	}

	return true;
}

bool SourceTree::GroupedItemsSelected() const
{
	SourceTreeModel *stm = GetStm();
	QModelIndexList selectedIndices = selectedIndexes();
	OBSScene scene = GetCurrentScene();

	if (!selectedIndices.size()) {
		return false;
	}

	for (auto &idx : selectedIndices) {
		obs_sceneitem_t *item = stm->items[idx.row()];
		obs_scene *itemScene = obs_sceneitem_get_scene(item);

		if (itemScene != scene) {
			return true;
		}
	}

	return false;
}

void SourceTree::Remove(OBSSceneItem item, OBSScene scene)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	GetStm()->Remove(item);
	main->SaveProject();

	if (!main->SavingDisabled()) {
		obs_source_t *sceneSource = obs_scene_get_source(scene);
		obs_source_t *itemSource = obs_sceneitem_get_source(item);
		blog(LOG_INFO, "User Removed source '%s' (%s) from scene '%s'", obs_source_get_name(itemSource),
		     obs_source_get_id(itemSource), obs_source_get_name(sceneSource));
	}
}

void SourceTree::GroupSelectedItems()
{
	QModelIndexList indices = selectedIndexes();
	std::sort(indices.begin(), indices.end());
	GetStm()->GroupSelectedItems(indices);
}

void SourceTree::UngroupSelectedGroups()
{
	QModelIndexList indices = selectedIndexes();
	GetStm()->UngroupSelectedGroups(indices);
}

void SourceTree::AddGroup()
{
	GetStm()->AddGroup();
}

void SourceTree::UpdateNoSourcesMessage()
{
	QString file = !App()->IsThemeDark() ? ":res/images/no_sources.svg" : "theme:Dark/no_sources.svg";
	iconNoSources.load(file);

	QTextOption opt(Qt::AlignHCenter);
	opt.setWrapMode(QTextOption::WordWrap);
	textNoSources.setTextOption(opt);
	textNoSources.setText(QTStr("NoSources.Label").replace("\n", "<br/>"));

	textPrepared = false;
}

void SourceTree::paintEvent(QPaintEvent *event)
{
	SourceTreeModel *stm = GetStm();
	if (stm && !stm->items.count()) {
		QPainter p(viewport());

		if (!textPrepared) {
			textNoSources.prepare(QTransform(), p.font());
			textPrepared = true;
		}

		QRectF iconRect = iconNoSources.viewBoxF();
		iconRect.setSize(QSizeF(32.0, 32.0));

		QSizeF iconSize = iconRect.size();
		QSizeF textSize = textNoSources.size();
		QSizeF thisSize = size();
		const qreal spacing = 16.0;

		qreal totalHeight = iconSize.height() + spacing + textSize.height();

		qreal x = thisSize.width() / 2.0 - iconSize.width() / 2.0;
		qreal y = thisSize.height() / 2.0 - totalHeight / 2.0;
		iconRect.moveTo(std::round(x), std::round(y));
		iconNoSources.render(&p, iconRect);

		x = thisSize.width() / 2.0 - textSize.width() / 2.0;
		y += spacing + iconSize.height();
		p.drawStaticText(x, y, textNoSources);
	} else {
		QListView::paintEvent(event);
	}
}
