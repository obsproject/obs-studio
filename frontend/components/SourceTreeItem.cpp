#include "SourceTreeItem.hpp"

#include <components/OBSSourceLabel.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QCheckBox>
#include <QLineEdit>
#include <QPainter>

#include "plugin-manager/PluginManager.hpp"

#include "moc_SourceTreeItem.cpp"

static inline OBSScene GetCurrentScene()
{
	OBSBasic *main = OBSBasic::Get();
	return main->GetCurrentScene();
}

SourceTreeItem::SourceTreeItem(SourceTree *tree_, OBSSceneItem sceneitem_) : tree(tree_), sceneitem(sceneitem_)
{
	setAttribute(Qt::WA_TranslucentBackground);
	setMouseTracking(true);

	obs_source_t *source = obs_sceneitem_get_source(sceneitem);
	const char *name = obs_source_get_name(source);

	OBSDataAutoRelease privData = obs_sceneitem_get_private_settings(sceneitem);
	int preset = obs_data_get_int(privData, "color-preset");

	if (preset == 1) {
		const char *color = obs_data_get_string(privData, "color");
		std::string col = "background: ";
		col += color;
		setStyleSheet(col.c_str());
	} else if (preset > 1) {
		setStyleSheet("");
		setProperty("bgColor", preset - 1);
	} else {
		setStyleSheet("background: none");
	}

	OBSBasic *main = OBSBasic::Get();
	const char *id = obs_source_get_id(source);

	bool sourceVisible = obs_sceneitem_visible(sceneitem);

	if (tree->iconsVisible) {
		QIcon icon;

		if (strcmp(id, "scene") == 0)
			icon = main->GetSceneIcon();
		else if (strcmp(id, "group") == 0)
			icon = main->GetGroupIcon();
		else
			icon = main->GetSourceIcon(id);

		QPixmap pixmap = icon.pixmap(QSize(16, 16));

		iconLabel = new QLabel();
		iconLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
		iconLabel->setPixmap(pixmap);
		iconLabel->setEnabled(sourceVisible);
		iconLabel->setStyleSheet("background: none");
		iconLabel->setProperty("class", "source-icon");
	}

	vis = new QCheckBox();
	vis->setProperty("class", "checkbox-icon indicator-visibility");
	vis->setChecked(sourceVisible);
	vis->setAccessibleName(QTStr("Basic.Main.Sources.Visibility"));
	vis->setAccessibleDescription(QTStr("Basic.Main.Sources.VisibilityDescription").arg(name));
	vis->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

	lock = new QCheckBox();
	lock->setProperty("class", "checkbox-icon indicator-lock");
	lock->setChecked(obs_sceneitem_locked(sceneitem));
	lock->setAccessibleName(QTStr("Basic.Main.Sources.Lock"));
	lock->setAccessibleDescription(QTStr("Basic.Main.Sources.LockDescription").arg(name));
	lock->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);

	label = new OBSSourceLabel(source);
	label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
	label->setAttribute(Qt::WA_TranslucentBackground);
	label->setEnabled(sourceVisible);

	const char *sourceId = obs_source_get_unversioned_id(source);
	switch (obs_source_load_state(sourceId)) {
	case OBS_MODULE_DISABLED:
	case OBS_MODULE_MISSING:
		label->setStyleSheet("QLabel {color: #CC0000;}");
		break;
	default:
		break;
	}

#ifdef __APPLE__
	vis->setAttribute(Qt::WA_LayoutUsesWidgetRect);
	lock->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif

	boxLayout = new QHBoxLayout();

	boxLayout->setContentsMargins(0, 0, 0, 0);
	boxLayout->setSpacing(0);
	if (iconLabel) {
		boxLayout->addWidget(iconLabel);
		boxLayout->addSpacing(2);
	}
	boxLayout->addWidget(label);
	boxLayout->addWidget(vis);
	boxLayout->addWidget(lock);

	Update(false);

	setLayout(boxLayout);

	/* --------------------------------------------------------- */

	auto setItemVisible = [this](bool val) {
		obs_scene_t *scene = obs_sceneitem_get_scene(sceneitem);
		obs_source_t *scenesource = obs_scene_get_source(scene);
		int64_t id = obs_sceneitem_get_id(sceneitem);
		const char *name = obs_source_get_name(scenesource);
		const char *uuid = obs_source_get_uuid(scenesource);
		obs_source_t *source = obs_sceneitem_get_source(sceneitem);

		auto undo_redo = [](const std::string &uuid, int64_t id, bool val) {
			OBSSourceAutoRelease s = obs_get_source_by_uuid(uuid.c_str());
			obs_scene_t *sc = obs_group_or_scene_from_source(s);
			obs_sceneitem_t *si = obs_scene_find_sceneitem_by_id(sc, id);
			if (si)
				obs_sceneitem_set_visible(si, val);
		};

		QString str = QTStr(val ? "Undo.ShowSceneItem" : "Undo.HideSceneItem");

		OBSBasic *main = OBSBasic::Get();
		main->undo_s.add_action(str.arg(obs_source_get_name(source), name),
					std::bind(undo_redo, std::placeholders::_1, id, !val),
					std::bind(undo_redo, std::placeholders::_1, id, val), uuid, uuid);

		QSignalBlocker sourcesSignalBlocker(this);
		obs_sceneitem_set_visible(sceneitem, val);
	};

	auto setItemLocked = [this](bool checked) {
		QSignalBlocker sourcesSignalBlocker(this);
		obs_sceneitem_set_locked(sceneitem, checked);
	};

	connect(vis, &QAbstractButton::clicked, setItemVisible);
	connect(lock, &QAbstractButton::clicked, setItemLocked);
}

void SourceTreeItem::paintEvent(QPaintEvent *event)
{
	QStyleOption opt;
	opt.initFrom(this);
	QPainter p(this);
	style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

	QWidget::paintEvent(event);
}

void SourceTreeItem::DisconnectSignals()
{
	sigs.clear();
}

void SourceTreeItem::Clear()
{
	DisconnectSignals();
	sceneitem = nullptr;
}

void SourceTreeItem::ReconnectSignals()
{
	if (!sceneitem)
		return;

	DisconnectSignals();

	/* --------------------------------------------------------- */

	auto removeItem = [](void *data, calldata_t *cd) {
		SourceTreeItem *this_ = static_cast<SourceTreeItem *>(data);
		obs_sceneitem_t *curItem = (obs_sceneitem_t *)calldata_ptr(cd, "item");
		obs_scene_t *curScene = (obs_scene_t *)calldata_ptr(cd, "scene");

		if (curItem == this_->sceneitem) {
			QMetaObject::invokeMethod(this_->tree, "Remove", Q_ARG(OBSSceneItem, curItem),
						  Q_ARG(OBSScene, curScene));
			curItem = nullptr;
		}
		if (!curItem)
			QMetaObject::invokeMethod(this_, "Clear");
	};

	auto itemVisible = [](void *data, calldata_t *cd) {
		SourceTreeItem *this_ = static_cast<SourceTreeItem *>(data);
		obs_sceneitem_t *curItem = (obs_sceneitem_t *)calldata_ptr(cd, "item");
		bool visible = calldata_bool(cd, "visible");

		if (curItem == this_->sceneitem)
			QMetaObject::invokeMethod(this_, "VisibilityChanged", Q_ARG(bool, visible));
	};

	auto itemLocked = [](void *data, calldata_t *cd) {
		SourceTreeItem *this_ = static_cast<SourceTreeItem *>(data);
		obs_sceneitem_t *curItem = (obs_sceneitem_t *)calldata_ptr(cd, "item");
		bool locked = calldata_bool(cd, "locked");

		if (curItem == this_->sceneitem)
			QMetaObject::invokeMethod(this_, "LockedChanged", Q_ARG(bool, locked));
	};

	auto itemSelect = [](void *data, calldata_t *cd) {
		SourceTreeItem *this_ = static_cast<SourceTreeItem *>(data);
		obs_sceneitem_t *curItem = (obs_sceneitem_t *)calldata_ptr(cd, "item");

		if (curItem == this_->sceneitem)
			QMetaObject::invokeMethod(this_, "Select");
	};

	auto itemDeselect = [](void *data, calldata_t *cd) {
		SourceTreeItem *this_ = static_cast<SourceTreeItem *>(data);
		obs_sceneitem_t *curItem = (obs_sceneitem_t *)calldata_ptr(cd, "item");

		if (curItem == this_->sceneitem)
			QMetaObject::invokeMethod(this_, "Deselect");
	};

	auto reorderGroup = [](void *data, calldata_t *) {
		SourceTreeItem *this_ = static_cast<SourceTreeItem *>(data);
		QMetaObject::invokeMethod(this_->tree, "ReorderItems");
	};

	obs_scene_t *scene = obs_sceneitem_get_scene(sceneitem);
	obs_source_t *sceneSource = obs_scene_get_source(scene);
	signal_handler_t *signal = obs_source_get_signal_handler(sceneSource);

	sigs.emplace_back(signal, "remove", removeItem, this);
	sigs.emplace_back(signal, "item_remove", removeItem, this);
	sigs.emplace_back(signal, "item_visible", itemVisible, this);
	sigs.emplace_back(signal, "item_locked", itemLocked, this);
	sigs.emplace_back(signal, "item_select", itemSelect, this);
	sigs.emplace_back(signal, "item_deselect", itemDeselect, this);

	if (obs_sceneitem_is_group(sceneitem)) {
		obs_source_t *source = obs_sceneitem_get_source(sceneitem);
		signal = obs_source_get_signal_handler(source);

		sigs.emplace_back(signal, "reorder", reorderGroup, this);
	}

	/* --------------------------------------------------------- */

	auto removeSource = [](void *data, calldata_t *) {
		SourceTreeItem *this_ = static_cast<SourceTreeItem *>(data);
		this_->DisconnectSignals();
		this_->sceneitem = nullptr;
		QMetaObject::invokeMethod(this_->tree, "RefreshItems");
	};

	obs_source_t *source = obs_sceneitem_get_source(sceneitem);
	signal = obs_source_get_signal_handler(source);
	sigs.emplace_back(signal, "remove", removeSource, this);
}

void SourceTreeItem::mouseDoubleClickEvent(QMouseEvent *event)
{
	QWidget::mouseDoubleClickEvent(event);

	if (expand) {
		expand->setChecked(!expand->isChecked());
	} else {
		obs_source_t *source = obs_sceneitem_get_source(sceneitem);
		OBSBasic *main = OBSBasic::Get();
		if (obs_source_configurable(source)) {
			main->CreatePropertiesWindow(source);
		}
	}
}

void SourceTreeItem::enterEvent(QEnterEvent *event)
{
	QWidget::enterEvent(event);

	OBSBasicPreview *preview = OBSBasicPreview::Get();

	std::lock_guard<std::mutex> lock(preview->selectMutex);
	preview->hoveredPreviewItems.clear();
	preview->hoveredPreviewItems.push_back(sceneitem);
}

void SourceTreeItem::leaveEvent(QEvent *event)
{
	QWidget::leaveEvent(event);

	OBSBasicPreview *preview = OBSBasicPreview::Get();

	std::lock_guard<std::mutex> lock(preview->selectMutex);
	preview->hoveredPreviewItems.clear();
}

bool SourceTreeItem::IsEditing()
{
	return editor != nullptr;
}

void SourceTreeItem::EnterEditMode()
{
	setFocusPolicy(Qt::StrongFocus);
	int index = boxLayout->indexOf(label);
	boxLayout->removeWidget(label);
	editor = new QLineEdit(label->text());
	editor->setStyleSheet("background: none");
	editor->selectAll();
	editor->installEventFilter(this);
	boxLayout->insertWidget(index, editor);
	setFocusProxy(editor);
}

void SourceTreeItem::ExitEditMode(bool save)
{
	ExitEditModeInternal(save);

	if (tree->undoSceneData) {
		OBSBasic *main = OBSBasic::Get();
		main->undo_s.pop_disabled();

		OBSData redoSceneData = main->BackupScene(GetCurrentScene());

		QString text = QTStr("Undo.GroupItems").arg(newName.c_str());
		main->CreateSceneUndoRedoAction(text, tree->undoSceneData, redoSceneData);

		tree->undoSceneData = nullptr;
	}
}

void SourceTreeItem::ExitEditModeInternal(bool save)
{
	if (!editor) {
		return;
	}

	OBSBasic *main = OBSBasic::Get();
	OBSScene scene = main->GetCurrentScene();

	newName = QT_TO_UTF8(editor->text());

	setFocusProxy(nullptr);
	int index = boxLayout->indexOf(editor);
	boxLayout->removeWidget(editor);
	delete editor;
	editor = nullptr;
	setFocusPolicy(Qt::NoFocus);
	boxLayout->insertWidget(index, label);
	setFocus();

	/* ----------------------------------------- */
	/* check for empty string                    */

	if (!save)
		return;

	if (newName.empty()) {
		OBSMessageBox::information(main, QTStr("NoNameEntered.Title"), QTStr("NoNameEntered.Text"));
		return;
	}

	/* ----------------------------------------- */
	/* Check for same name                       */

	obs_source_t *source = obs_sceneitem_get_source(sceneitem);
	if (newName == obs_source_get_name(source))
		return;

	/* ----------------------------------------- */
	/* check for existing source                 */

	OBSSourceAutoRelease existingSource = obs_get_source_by_name(newName.c_str());
	bool exists = !!existingSource;

	if (exists) {
		OBSMessageBox::information(main, QTStr("NameExists.Title"), QTStr("NameExists.Text"));
		return;
	}

	/* ----------------------------------------- */
	/* rename                                    */

	QSignalBlocker sourcesSignalBlocker(this);
	std::string prevName(obs_source_get_name(source));
	std::string scene_uuid = obs_source_get_uuid(main->GetCurrentSceneSource());
	auto undo = [scene_uuid, prevName, main](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
		obs_source_set_name(source, prevName.c_str());

		OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
		main->SetCurrentScene(scene_source.Get(), true);
	};

	std::string editedName = newName;

	auto redo = [scene_uuid, main, editedName](const std::string &data) {
		OBSSourceAutoRelease source = obs_get_source_by_uuid(data.c_str());
		obs_source_set_name(source, editedName.c_str());

		OBSSourceAutoRelease scene_source = obs_get_source_by_uuid(scene_uuid.c_str());
		main->SetCurrentScene(scene_source.Get(), true);
	};

	const char *uuid = obs_source_get_uuid(source);
	main->undo_s.add_action(QTStr("Undo.Rename").arg(newName.c_str()), undo, redo, uuid, uuid);

	obs_source_set_name(source, newName.c_str());
}

bool SourceTreeItem::eventFilter(QObject *object, QEvent *event)
{
	if (editor != object)
		return false;

	if (LineEditCanceled(event)) {
		QMetaObject::invokeMethod(this, "ExitEditMode", Qt::QueuedConnection, Q_ARG(bool, false));
		return true;
	}
	if (LineEditChanged(event)) {
		QMetaObject::invokeMethod(this, "ExitEditMode", Qt::QueuedConnection, Q_ARG(bool, true));
		return true;
	}

	return false;
}

void SourceTreeItem::VisibilityChanged(bool visible)
{
	if (iconLabel) {
		iconLabel->setEnabled(visible);
	}
	label->setEnabled(visible);
	vis->setChecked(visible);
}

void SourceTreeItem::LockedChanged(bool locked)
{
	lock->setChecked(locked);
	OBSBasic::Get()->UpdateEditMenu();
}

void SourceTreeItem::Update(bool force)
{
	OBSScene scene = GetCurrentScene();
	obs_scene_t *itemScene = obs_sceneitem_get_scene(sceneitem);

	Type newType;

	/* ------------------------------------------------- */
	/* if it's a group item, insert group checkbox       */

	if (obs_sceneitem_is_group(sceneitem)) {
		newType = Type::Group;

		/* ------------------------------------------------- */
		/* if it's a group sub-item                          */

	} else if (itemScene != scene) {
		newType = Type::SubItem;

		/* ------------------------------------------------- */
		/* if it's a regular item                            */

	} else {
		newType = Type::Item;
	}

	/* ------------------------------------------------- */

	if (!force && newType == type) {
		return;
	}

	/* ------------------------------------------------- */

	ReconnectSignals();

	if (spacer) {
		boxLayout->removeItem(spacer);
		delete spacer;
		spacer = nullptr;
	}

	if (type == Type::Group) {
		boxLayout->removeWidget(expand);
		expand->deleteLater();
		expand = nullptr;
	}

	type = newType;

	if (type == Type::SubItem) {
		spacer = new QSpacerItem(16, 1);
		boxLayout->insertItem(0, spacer);

	} else if (type == Type::Group) {
		expand = new QCheckBox();
		expand->setProperty("class", "checkbox-icon indicator-expand");
		expand->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
#ifdef __APPLE__
		expand->setAttribute(Qt::WA_LayoutUsesWidgetRect);
#endif
		boxLayout->insertWidget(0, expand);

		OBSDataAutoRelease data = obs_sceneitem_get_private_settings(sceneitem);
		expand->blockSignals(true);
		expand->setChecked(obs_data_get_bool(data, "collapsed"));
		expand->blockSignals(false);

		connect(expand, &QPushButton::toggled, this, &SourceTreeItem::ExpandClicked);

	} else {
		spacer = new QSpacerItem(3, 1);
		boxLayout->insertItem(0, spacer);
	}
}

void SourceTreeItem::ExpandClicked(bool checked)
{
	OBSDataAutoRelease data = obs_sceneitem_get_private_settings(sceneitem);

	obs_data_set_bool(data, "collapsed", checked);

	if (!checked)
		tree->GetStm()->ExpandGroup(sceneitem);
	else
		tree->GetStm()->CollapseGroup(sceneitem);
}

void SourceTreeItem::Select()
{
	tree->SelectItem(sceneitem, true);
	OBSBasic::Get()->UpdateContextBarDeferred();
	OBSBasic::Get()->UpdateEditMenu();
}

void SourceTreeItem::Deselect()
{
	tree->SelectItem(sceneitem, false);
	OBSBasic::Get()->UpdateContextBarDeferred();
	OBSBasic::Get()->UpdateEditMenu();
}
