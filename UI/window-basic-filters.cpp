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

#include "properties-view.hpp"
#include "window-namedialog.hpp"
#include "window-basic-main.hpp"
#include "window-basic-filters.hpp"
#include "display-helpers.hpp"
#include "qt-wrappers.hpp"
#include "visibility-item-widget.hpp"
#include "item-widget-helpers.hpp"
#include "obs-app.hpp"
#include "undo-stack-obs.hpp"

#include <QMessageBox>
#include <QCloseEvent>
#include <obs-data.h>
#include <obs.h>
#include <util/base.h>
#include <vector>
#include <string>
#include <QMenu>
#include <QVariant>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN 1
#include <Windows.h>
#endif

using namespace std;

Q_DECLARE_METATYPE(OBSSource);

static bool filter_compatible(bool async, uint32_t sourceFlags,
			      uint32_t filterFlags)
{
	bool filterVideo = (filterFlags & OBS_SOURCE_VIDEO) != 0;
	bool filterAsync = (filterFlags & OBS_SOURCE_ASYNC) != 0;
	bool filterAudio = (filterFlags & OBS_SOURCE_AUDIO) != 0;
	bool audio = (sourceFlags & OBS_SOURCE_AUDIO) != 0;
	bool audioOnly = (sourceFlags & OBS_SOURCE_VIDEO) == 0;
	bool asyncSource = (sourceFlags & OBS_SOURCE_ASYNC) != 0;

	if (async &&
	    ((audioOnly && filterVideo) || (!audio && !asyncSource) ||
	     (filterAudio && !audio) || (!asyncSource && !filterAudio)))
		return false;

	return (async && (filterAudio || filterAsync)) ||
	       (!async && !filterAudio && !filterAsync);
}

OBSBasicFilters::OBSBasicFilters(QWidget *parent, OBSSource source_,
				 bool isAsync_)
	: QWidget(parent),
	  ui(new Ui::OBSBasicFilters),
	  source(source_),
	  addSignal(obs_source_get_signal_handler(source), "filter_add",
		    OBSBasicFilters::OBSSourceFilterAdded, this),
	  removeSignal(obs_source_get_signal_handler(source), "filter_remove",
		       OBSBasicFilters::OBSSourceFilterRemoved, this),
	  reorderSignal(obs_source_get_signal_handler(source),
			"reorder_filters", OBSBasicFilters::OBSSourceReordered,
			this),
	  isAsync(isAsync_)
{
	main = OBSBasic::Get();

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	ui->filtersList->setItemDelegate(
		new VisibilityItemDelegate(ui->filtersList));

	const char *name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.Filters.Title").arg(QT_UTF8(name)));

#ifndef QT_NO_SHORTCUT
	ui->actionRemoveFilter->setShortcut(
		QApplication::translate("OBSBasicFilters", "Del", nullptr));
#endif // QT_NO_SHORTCUT

	addAction(ui->actionRenameFilter);
	addAction(ui->actionRemoveFilter);
	addAction(ui->actionMoveUp);
	addAction(ui->actionMoveDown);

	installEventFilter(CreateShortcutFilter());

	connect(ui->filtersList->itemDelegate(), SIGNAL(closeEditor(QWidget *)),
		this, SLOT(FilterNameEdited(QWidget *)));

#ifdef __APPLE__
	ui->actionRenameFilter->setShortcut({Qt::Key_Return});
#else
	ui->actionRenameFilter->setShortcut({Qt::Key_F2});
#endif

	UpdateFilters();
}

OBSBasicFilters::~OBSBasicFilters()
{
	ClearListItems(ui->filtersList);
}

inline OBSSource OBSBasicFilters::GetFilter(int row)
{
	if (row == -1)
		return OBSSource();

	QListWidgetItem *item = ui->filtersList->item(row);
	if (!item)
		return OBSSource();

	QVariant v = item->data(Qt::UserRole);
	return v.value<OBSSource>();
}

void FilterChangeUndoRedo(void *vp, obs_data_t *nd_old_settings,
			  obs_data_t *new_settings)
{
	obs_source_t *source = reinterpret_cast<obs_source_t *>(vp);
	obs_source_t *parent = obs_filter_get_parent(source);
	const char *source_name = obs_source_get_name(source);
	OBSBasic *main = OBSBasic::Get();

	OBSDataAutoRelease redo_wrapper = obs_data_create();
	obs_data_set_string(redo_wrapper, "name", source_name);
	obs_data_set_string(redo_wrapper, "settings",
			    obs_data_get_json(new_settings));
	obs_data_set_string(redo_wrapper, "parent",
			    obs_source_get_name(parent));

	OBSDataAutoRelease filter_settings = obs_source_get_settings(source);

	OBSDataAutoRelease undo_wrapper = obs_data_create();
	obs_data_set_string(undo_wrapper, "name", source_name);
	obs_data_set_string(undo_wrapper, "settings",
			    obs_data_get_json(nd_old_settings));
	obs_data_set_string(undo_wrapper, "parent",
			    obs_source_get_name(parent));

	auto undo_redo = [](const std::string &data) {
		OBSDataAutoRelease dat =
			obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease parent_source = obs_get_source_by_name(
			obs_data_get_string(dat, "parent"));
		const char *filter_name = obs_data_get_string(dat, "name");
		OBSSourceAutoRelease filter = obs_source_get_filter_by_name(
			parent_source, filter_name);
		OBSDataAutoRelease new_settings = obs_data_create_from_json(
			obs_data_get_string(dat, "settings"));

		OBSDataAutoRelease current_settings =
			obs_source_get_settings(filter);
		obs_data_clear(current_settings);

		obs_source_update(filter, new_settings);
		obs_source_update_properties(filter);
	};

	main->undo_s.enable();

	std::string name = std::string(obs_source_get_name(source));
	std::string undo_data = obs_data_get_json(undo_wrapper);
	std::string redo_data = obs_data_get_json(redo_wrapper);
	main->undo_s.add_action(QTStr("Undo.Filters").arg(name.c_str()),
				undo_redo, undo_redo, undo_data, redo_data);

	obs_source_update(source, new_settings);
}

void OBSBasicFilters::UpdatePropertiesView(int row)
{
	OBSSource filter = GetFilter(row);
	if (filter && view && view->IsObject(filter)) {
		/* do not recreate properties view if already using a view
		 * with the same object */
		return;
	}

	if (view) {
		updatePropertiesSignal.Disconnect();
		/* Deleting a filter will trigger a visibility change, which will also
		 * trigger a focus change if the focus has not been on the list itself
		 * (e.g. after interacting with the property view).
		 *
		 * When an async filter list is available in the view, it will be the first
		 * candidate to receive focus. If this list is empty, we hide the property
		 * view by default and set the view to a `nullptr`.
		 *
		 * When the call for the visibility change returns, we need to check for
		 * this possibility, as another event might have hidden (and deleted) the
		 * view already.
		 *
		 * macOS might be especially affected as it doesn't switch keyboard focus
		 * to buttons like Windows does. */
		if (view) {
			view->hide();
			view->deleteLater();
			view = nullptr;
		}
	}

	if (!filter)
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(filter);

	auto disabled_undo = [](void *vp, obs_data_t *settings) {
		OBSBasic *main =
			reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
		main->undo_s.disable();
		obs_source_t *source = reinterpret_cast<obs_source_t *>(vp);
		obs_source_update(source, settings);
	};

	view = new OBSPropertiesView(
		settings.Get(), filter,
		(PropertiesReloadCallback)obs_source_properties,
		(PropertiesUpdateCallback)FilterChangeUndoRedo,
		(PropertiesVisualUpdateCb)disabled_undo);

	updatePropertiesSignal.Connect(obs_source_get_signal_handler(filter),
				       "update_properties",
				       OBSBasicFilters::UpdateProperties, this);

	view->setMinimumHeight(150);
	ui->propertiesLayout->addWidget(view);
	view->show();
}

void OBSBasicFilters::UpdateProperties(void *data, calldata_t *)
{
	QMetaObject::invokeMethod(static_cast<OBSBasicFilters *>(data)->view,
				  "ReloadProperties");
}

void OBSBasicFilters::AddFilter(OBSSource filter, bool focus)
{
	uint32_t sourceFlags = obs_source_get_output_flags(source);
	uint32_t filterFlags = obs_source_get_output_flags(filter);

	if (!filter_compatible(isAsync, sourceFlags, filterFlags))
		return;

	QListWidgetItem *item = new QListWidgetItem();
	Qt::ItemFlags itemFlags = item->flags();

	item->setFlags(itemFlags | Qt::ItemIsEditable);
	item->setData(Qt::UserRole, QVariant::fromValue(filter));

	ui->filtersList->addItem(item);
	if (focus)
		ui->filtersList->setCurrentItem(item);

	SetupVisibilityItem(ui->filtersList, item, filter);
}

void OBSBasicFilters::RemoveFilter(OBSSource filter)
{
	for (int i = 0; i < ui->filtersList->count(); i++) {
		QListWidgetItem *item = ui->filtersList->item(i);
		QVariant v = item->data(Qt::UserRole);
		OBSSource curFilter = v.value<OBSSource>();

		if (filter == curFilter) {
			DeleteListItem(ui->filtersList, item);
			break;
		}
	}

	const char *filterName = obs_source_get_name(filter);
	const char *sourceName = obs_source_get_name(source);
	if (!sourceName || !filterName)
		return;

	const char *filterId = obs_source_get_id(filter);

	blog(LOG_INFO, "User removed filter '%s' (%s) from source '%s'",
	     filterName, filterId, sourceName);

	main->SaveProject();
}

struct FilterOrderInfo {
	int idx = 0;
	OBSBasicFilters *window;

	inline FilterOrderInfo(OBSBasicFilters *window_) : window(window_) {}
};

void OBSBasicFilters::ReorderFilter(QListWidget *list, obs_source_t *filter,
				    size_t idx)
{
	int count = list->count();

	for (int i = 0; i < count; i++) {
		QListWidgetItem *listItem = list->item(i);
		QVariant v = listItem->data(Qt::UserRole);
		OBSSource filterItem = v.value<OBSSource>();

		if (filterItem == filter) {
			if ((int)idx != i) {
				bool sel = (list->currentRow() == i);

				listItem = TakeListItem(list, i);
				if (listItem) {
					list->insertItem((int)idx, listItem);
					SetupVisibilityItem(list, listItem,
							    filterItem);

					if (sel)
						list->setCurrentRow((int)idx);
				}
			}

			break;
		}
	}
}

void OBSBasicFilters::ReorderFilters()
{
	FilterOrderInfo info(this);

	obs_source_enum_filters(
		source,
		[](obs_source_t *, obs_source_t *filter, void *p) {
			FilterOrderInfo *info =
				reinterpret_cast<FilterOrderInfo *>(p);

			info->window->ReorderFilter(
				info->window->ui->filtersList, filter,
				info->idx++);
		},
		&info);
}

void OBSBasicFilters::UpdateFilters()
{
	if (!source)
		return;

	ClearListItems(ui->filtersList);

	obs_source_enum_filters(
		source,
		[](obs_source_t *, obs_source_t *filter, void *p) {
			OBSBasicFilters *window =
				reinterpret_cast<OBSBasicFilters *>(p);

			window->AddFilter(filter, false);
		},
		this);

	if (ui->filtersList->count() > 0)
		ui->filtersList->setCurrentItem(ui->filtersList->item(0));

	main->SaveProject();
}

QMenu *OBSBasicFilters::CreateAddFilterPopupMenu()
{
	uint32_t sourceFlags = obs_source_get_output_flags(source);
	const char *type_str;
	bool foundValues = false;
	size_t idx = 0;

	struct FilterInfo {
		string type;
		string name;

		inline FilterInfo(const char *type_, const char *name_)
			: type(type_), name(name_)
		{
		}

		bool operator<(const FilterInfo &r) const
		{
			return name < r.name;
		}
	};

	vector<FilterInfo> types;
	while (obs_enum_filter_types(idx++, &type_str)) {
		const char *name = obs_source_get_display_name(type_str);
		uint32_t caps = obs_get_source_output_flags(type_str);

		if ((caps & OBS_SOURCE_DEPRECATED) != 0)
			continue;
		if ((caps & OBS_SOURCE_CAP_DISABLED) != 0)
			continue;
		if ((caps & OBS_SOURCE_CAP_OBSOLETE) != 0)
			continue;

		types.emplace_back(type_str, name);
	}

	sort(types.begin(), types.end());

	QMenu *popup = new QMenu(QTStr("Add"), this);
	for (FilterInfo &type : types) {
		uint32_t filterFlags =
			obs_get_source_output_flags(type.type.c_str());

		if (!filter_compatible(isAsync, sourceFlags, filterFlags))
			continue;

		QAction *popupItem =
			new QAction(QT_UTF8(type.name.c_str()), this);
		popupItem->setData(QT_UTF8(type.type.c_str()));
		connect(popupItem, SIGNAL(triggered(bool)), this,
			SLOT(AddFilterFromAction()));
		popup->addAction(popupItem);

		foundValues = true;
	}

	if (!foundValues) {
		delete popup;
		popup = nullptr;
	}

	return popup;
}

void OBSBasicFilters::AddNewFilter(const char *id)
{
	if (id && *id) {
		OBSSourceAutoRelease existing_filter;
		string name = obs_source_get_display_name(id);

		QString placeholder = QString::fromStdString(name);
		QString text{placeholder};
		int i = 2;
		while ((existing_filter = obs_source_get_filter_by_name(
				source, QT_TO_UTF8(text)))) {
			text = QString("%1 %2").arg(placeholder).arg(i++);
		}

		bool success = NameDialog::AskForName(
			this, QTStr("Basic.Filters.AddFilter.Title"),
			QTStr("Basic.Filters.AddFilter.Text"), name, text);
		if (!success)
			return;

		if (name.empty()) {
			OBSMessageBox::warning(this,
					       QTStr("NoNameEntered.Title"),
					       QTStr("NoNameEntered.Text"));
			AddNewFilter(id);
			return;
		}

		existing_filter =
			obs_source_get_filter_by_name(source, name.c_str());
		if (existing_filter) {
			OBSMessageBox::warning(this, QTStr("NameExists.Title"),
					       QTStr("NameExists.Text"));
			AddNewFilter(id);
			return;
		}

		OBSDataAutoRelease wrapper = obs_data_create();
		obs_data_set_string(wrapper, "sname",
				    obs_source_get_name(source));
		obs_data_set_string(wrapper, "fname", name.c_str());
		std::string scene_name = obs_source_get_name(
			reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
				->GetCurrentSceneSource());
		auto undo = [scene_name](const std::string &data) {
			obs_source_t *ssource =
				obs_get_source_by_name(scene_name.c_str());
			reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
				->SetCurrentScene(ssource, true);
			obs_source_release(ssource);

			obs_data_t *dat =
				obs_data_create_from_json(data.c_str());
			obs_source_t *source = obs_get_source_by_name(
				obs_data_get_string(dat, "sname"));
			obs_source_t *filter = obs_source_get_filter_by_name(
				source, obs_data_get_string(dat, "fname"));
			obs_source_filter_remove(source, filter);

			obs_data_release(dat);
			obs_source_release(source);
			obs_source_release(filter);
		};

		OBSDataAutoRelease rwrapper = obs_data_create();
		obs_data_set_string(rwrapper, "sname",
				    obs_source_get_name(source));
		auto redo = [scene_name, id = std::string(id),
			     name](const std::string &data) {
			OBSSourceAutoRelease ssource =
				obs_get_source_by_name(scene_name.c_str());
			reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
				->SetCurrentScene(ssource.Get(), true);

			OBSDataAutoRelease dat =
				obs_data_create_from_json(data.c_str());
			OBSSourceAutoRelease source = obs_get_source_by_name(
				obs_data_get_string(dat, "sname"));

			OBSSourceAutoRelease filter = obs_source_create(
				id.c_str(), name.c_str(), nullptr, nullptr);
			if (filter) {
				obs_source_filter_add(source, filter);
			}
		};

		std::string undo_data(obs_data_get_json(wrapper));
		std::string redo_data(obs_data_get_json(rwrapper));
		main->undo_s.add_action(QTStr("Undo.Add").arg(name.c_str()),
					undo, redo, undo_data, redo_data);

		OBSSourceAutoRelease filter =
			obs_source_create(id, name.c_str(), nullptr, nullptr);
		if (filter) {
			const char *sourceName = obs_source_get_name(source);

			blog(LOG_INFO,
			     "User added filter '%s' (%s) "
			     "to source '%s'",
			     name.c_str(), id, sourceName);

			obs_source_filter_add(source, filter);
		}
	}
}

void OBSBasicFilters::AddFilterFromAction()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (!action)
		return;

	AddNewFilter(QT_TO_UTF8(action->data().toString()));
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool OBSBasicFilters::nativeEvent(const QByteArray &, void *message, qintptr *)
#else
bool OBSBasicFilters::nativeEvent(const QByteArray &, void *message, long *)
#endif
{
#ifdef _WIN32
	const MSG &msg = *static_cast<MSG *>(message);
	switch (msg.message) {
	case WM_MOVE:
		for (OBSQTDisplay *const display :
		     findChildren<OBSQTDisplay *>()) {
			display->OnMove();
		}
		break;
	case WM_DISPLAYCHANGE:
		for (OBSQTDisplay *const display :
		     findChildren<OBSQTDisplay *>()) {
			display->OnDisplayChange();
		}
	}
#else
	UNUSED_PARAMETER(message);
#endif

	return false;
}

/* OBS Signals */

void OBSBasicFilters::OBSSourceFilterAdded(void *param, calldata_t *data)
{
	OBSBasicFilters *window = reinterpret_cast<OBSBasicFilters *>(param);
	obs_source_t *filter = (obs_source_t *)calldata_ptr(data, "filter");

	QMetaObject::invokeMethod(window, "AddFilter",
				  Q_ARG(OBSSource, OBSSource(filter)));
}

void OBSBasicFilters::OBSSourceFilterRemoved(void *param, calldata_t *data)
{
	OBSBasicFilters *window = reinterpret_cast<OBSBasicFilters *>(param);
	obs_source_t *filter = (obs_source_t *)calldata_ptr(data, "filter");

	QMetaObject::invokeMethod(window, "RemoveFilter",
				  Q_ARG(OBSSource, OBSSource(filter)));
}

void OBSBasicFilters::OBSSourceReordered(void *param, calldata_t *)
{
	QMetaObject::invokeMethod(reinterpret_cast<OBSBasicFilters *>(param),
				  "ReorderFilters");
}

/* Qt Slots */

static bool QueryRemove(QWidget *parent, obs_source_t *source)
{
	const char *name = obs_source_get_name(source);

	QString text = QTStr("ConfirmRemove.Text").arg(QT_UTF8(name));

	QMessageBox remove_source(parent);
	remove_source.setText(text);
	QAbstractButton *Yes =
		remove_source.addButton(QTStr("Yes"), QMessageBox::YesRole);
	remove_source.addButton(QTStr("No"), QMessageBox::NoRole);
	remove_source.setIcon(QMessageBox::Question);
	remove_source.setWindowTitle(QTStr("ConfirmRemove.Title"));
	remove_source.exec();

	return Yes == remove_source.clickedButton();
}

void OBSBasicFilters::on_addFilter_clicked()
{
	QScopedPointer<QMenu> popup(CreateAddFilterPopupMenu());
	if (popup)
		popup->exec(QCursor::pos());
}

void OBSBasicFilters::on_removeFilter_clicked()
{
	OBSSource filter = GetFilter(ui->filtersList->currentRow());
	if (filter) {
		if (QueryRemove(this, filter))
			delete_filter(filter);
	}
}

void OBSBasicFilters::on_moveFilterUp_clicked()
{
	OBSSource filter = GetFilter(ui->filtersList->currentRow());
	if (filter)
		obs_source_filter_set_order(source, filter, OBS_ORDER_MOVE_UP);
}

void OBSBasicFilters::on_moveFilterDown_clicked()
{
	OBSSource filter = GetFilter(ui->filtersList->currentRow());
	if (filter)
		obs_source_filter_set_order(source, filter,
					    OBS_ORDER_MOVE_DOWN);
}

void OBSBasicFilters::on_filtersList_currentRowChanged(int row)
{
	UpdatePropertiesView(row);
}

void OBSBasicFilters::on_actionRemoveFilter_triggered()
{
	on_removeFilter_clicked();
}

void OBSBasicFilters::on_actionMoveUp_triggered()
{
	on_moveFilterUp_clicked();
}

void OBSBasicFilters::on_actionMoveDown_triggered()
{
	on_moveFilterDown_clicked();
}

void OBSBasicFilters::on_actionRenameFilter_triggered()
{
	RenameFilter();
}

void OBSBasicFilters::CustomContextMenu(const QPoint &pos)
{
	QListWidgetItem *item = ui->filtersList->itemAt(pos);

	QMenu popup(window());

	QPointer<QMenu> addMenu = CreateAddFilterPopupMenu();
	if (addMenu)
		popup.addMenu(addMenu);

	if (item) {
		popup.addSeparator();
		popup.addAction(QTStr("Duplicate"), this,
				SLOT(DuplicateFilter()));
		popup.addSeparator();
		popup.addAction(ui->actionRenameFilter);
		popup.addAction(ui->actionRemoveFilter);
		popup.addSeparator();

		QAction *copyAction = new QAction(QTStr("Copy"));
		copyAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
		connect(copyAction, SIGNAL(triggered()), this,
			SLOT(CopyFilter()));
		copyAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
		addAction(copyAction);
		popup.addAction(copyAction);
	}

	QAction *pasteAction = new QAction(QTStr("Paste"));
	pasteAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	pasteAction->setEnabled(main->copyFilter);
	connect(pasteAction, SIGNAL(triggered()), this, SLOT(PasteFilter()));
	pasteAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_V));
	addAction(pasteAction);
	popup.addAction(pasteAction);

	popup.exec(QCursor::pos());
}

void OBSBasicFilters::EditItem(QListWidgetItem *item)
{
	if (editActive)
		return;

	Qt::ItemFlags flags = item->flags();
	OBSSource filter = item->data(Qt::UserRole).value<OBSSource>();
	const char *name = obs_source_get_name(filter);

	item->setText(QT_UTF8(name));
	item->setFlags(flags | Qt::ItemIsEditable);
	ui->filtersList->removeItemWidget(item);
	ui->filtersList->editItem(item);
	item->setFlags(flags);
	editActive = true;
}

void OBSBasicFilters::DuplicateItem(QListWidgetItem *item)
{
	OBSSource filter = item->data(Qt::UserRole).value<OBSSource>();
	string name = obs_source_get_name(filter);
	OBSSourceAutoRelease existing_filter;

	QString placeholder = QString::fromStdString(name);
	QString text{placeholder};
	int i = 2;
	while ((existing_filter = obs_source_get_filter_by_name(
			source, QT_TO_UTF8(text)))) {
		text = QString("%1 %2").arg(placeholder).arg(i++);
	}

	bool success = NameDialog::AskForName(
		this, QTStr("Basic.Filters.AddFilter.Title"),
		QTStr("Basic.Filters.AddFilter.Text"), name, text);
	if (!success)
		return;

	if (name.empty()) {
		OBSMessageBox::warning(this, QTStr("NoNameEntered.Title"),
				       QTStr("NoNameEntered.Text"));
		DuplicateItem(item);
		return;
	}

	existing_filter = obs_source_get_filter_by_name(source, name.c_str());
	if (existing_filter) {
		OBSMessageBox::warning(this, QTStr("NameExists.Title"),
				       QTStr("NameExists.Text"));
		DuplicateItem(item);
		return;
	}
	bool enabled = obs_source_enabled(filter);
	OBSSourceAutoRelease new_filter =
		obs_source_duplicate(filter, name.c_str(), false);
	if (new_filter) {
		const char *sourceName = obs_source_get_name(source);
		const char *id = obs_source_get_id(new_filter);
		blog(LOG_INFO,
		     "User duplicated filter '%s' (%s) from '%s' "
		     "to source '%s'",
		     name.c_str(), id, name.c_str(), sourceName);
		obs_source_set_enabled(new_filter, enabled);
		obs_source_filter_add(source, new_filter);
	}
}

void OBSBasicFilters::on_filtersList_customContextMenuRequested(
	const QPoint &pos)
{
	CustomContextMenu(pos);
}

void OBSBasicFilters::RenameFilter()
{
	EditItem(ui->filtersList->currentItem());
}

void OBSBasicFilters::DuplicateFilter()
{
	DuplicateItem(ui->filtersList->currentItem());
}

void OBSBasicFilters::FilterNameEdited(QWidget *editor, QListWidget *list)
{
	QListWidgetItem *listItem = list->currentItem();
	OBSSource filter = listItem->data(Qt::UserRole).value<OBSSource>();
	QLineEdit *edit = qobject_cast<QLineEdit *>(editor);
	string name = QT_TO_UTF8(edit->text().trimmed());

	const char *prevName = obs_source_get_name(filter);
	bool sameName = (name == prevName);
	OBSSourceAutoRelease foundFilter = nullptr;

	if (!sameName)
		foundFilter =
			obs_source_get_filter_by_name(source, name.c_str());

	if (foundFilter || name.empty() || sameName) {
		listItem->setText(QT_UTF8(prevName));

		if (foundFilter) {
			OBSMessageBox::information(window(),
						   QTStr("NameExists.Title"),
						   QTStr("NameExists.Text"));
		} else if (name.empty()) {
			OBSMessageBox::information(window(),
						   QTStr("NoNameEntered.Title"),
						   QTStr("NoNameEntered.Text"));
		}
	} else {
		const char *sourceName = obs_source_get_name(source);

		blog(LOG_INFO,
		     "User renamed filter '%s' on source '%s' to '%s'",
		     prevName, sourceName, name.c_str());

		listItem->setText(QT_UTF8(name.c_str()));
		obs_source_set_name(filter, name.c_str());

		std::string scene_name = obs_source_get_name(
			reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
				->GetCurrentSceneSource());
		auto undo = [scene_name, prev = std::string(prevName),
			     name](const std::string &data) {
			OBSSourceAutoRelease ssource =
				obs_get_source_by_name(scene_name.c_str());
			reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
				->SetCurrentScene(ssource.Get(), true);

			OBSSourceAutoRelease source =
				obs_get_source_by_name(data.c_str());
			OBSSourceAutoRelease filter =
				obs_source_get_filter_by_name(source,
							      name.c_str());
			obs_source_set_name(filter, prev.c_str());
		};

		auto redo = [scene_name, prev = std::string(prevName),
			     name](const std::string &data) {
			OBSSourceAutoRelease ssource =
				obs_get_source_by_name(scene_name.c_str());
			reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
				->SetCurrentScene(ssource.Get(), true);

			OBSSourceAutoRelease source =
				obs_get_source_by_name(data.c_str());
			OBSSourceAutoRelease filter =
				obs_source_get_filter_by_name(source,
							      prev.c_str());
			obs_source_set_name(filter, name.c_str());
		};

		std::string undo_data(sourceName);
		std::string redo_data(sourceName);
		main->undo_s.add_action(QTStr("Undo.Rename").arg(name.c_str()),
					undo, redo, undo_data, redo_data);
	}

	listItem->setText(QString());
	SetupVisibilityItem(list, listItem, filter);
	editActive = false;
}

void OBSBasicFilters::FilterNameEdited(QWidget *editor)
{
	FilterNameEdited(editor, ui->filtersList);
}

static bool ConfirmReset(QWidget *parent)
{
	QMessageBox::StandardButton button;

	button = OBSMessageBox::question(parent, QTStr("ConfirmReset.Title"),
					 QTStr("ConfirmReset.Text"),
					 QMessageBox::Yes | QMessageBox::No);

	return button == QMessageBox::Yes;
}

void OBSBasicFilters::ResetFilters()
{
	int row = ui->filtersList->currentRow();

	OBSSource filter = GetFilter(row);

	if (!filter)
		return;

	if (!ConfirmReset(this))
		return;

	OBSDataAutoRelease settings = obs_source_get_settings(filter);

	OBSDataAutoRelease empty_settings = obs_data_create();
	FilterChangeUndoRedo((void *)filter, settings, empty_settings);

	obs_data_clear(settings);

	if (!view->DeferUpdate())
		obs_source_update(filter, nullptr);

	view->ReloadProperties();
}

void OBSBasicFilters::CopyFilter()
{
	OBSSource filter = GetFilter(ui->filtersList->currentRow());
	main->copyFilter = OBSGetWeakRef(filter);
}

void OBSBasicFilters::PasteFilter()
{
	OBSSource filter = OBSGetStrongRef(main->copyFilter);
	if (!filter)
		return;

	OBSDataArrayAutoRelease undo_array = obs_source_backup_filters(source);
	obs_source_copy_single_filter(source, filter);
	OBSDataArrayAutoRelease redo_array = obs_source_backup_filters(source);

	const char *filterName = obs_source_get_name(filter);
	const char *sourceName = obs_source_get_name(source);
	QString text =
		QTStr("Undo.Filters.Paste.Single").arg(filterName, sourceName);

	main->CreateFilterPasteUndoRedoAction(text, source, undo_array,
					      redo_array);
}

void OBSBasicFilters::delete_filter(OBSSource filter)
{
	OBSDataAutoRelease wrapper = obs_save_source(filter);
	std::string parent_name(obs_source_get_name(source));
	obs_data_set_string(wrapper, "undo_name", parent_name.c_str());

	std::string scene_name = obs_source_get_name(
		reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
			->GetCurrentSceneSource());
	auto undo = [scene_name](const std::string &data) {
		OBSSourceAutoRelease ssource =
			obs_get_source_by_name(scene_name.c_str());
		reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
			->SetCurrentScene(ssource.Get(), true);

		OBSDataAutoRelease dat =
			obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_name(
			obs_data_get_string(dat, "undo_name"));
		OBSSourceAutoRelease filter = obs_load_source(dat);
		obs_source_filter_add(source, filter);
	};

	OBSDataAutoRelease rwrapper = obs_data_create();
	obs_data_set_string(rwrapper, "fname", obs_source_get_name(filter));
	obs_data_set_string(rwrapper, "sname", parent_name.c_str());
	auto redo = [scene_name](const std::string &data) {
		OBSSourceAutoRelease ssource =
			obs_get_source_by_name(scene_name.c_str());
		reinterpret_cast<OBSBasic *>(App()->GetMainWindow())
			->SetCurrentScene(ssource.Get(), true);

		OBSDataAutoRelease dat =
			obs_data_create_from_json(data.c_str());
		OBSSourceAutoRelease source = obs_get_source_by_name(
			obs_data_get_string(dat, "sname"));
		OBSSourceAutoRelease filter = obs_source_get_filter_by_name(
			source, obs_data_get_string(dat, "fname"));
		obs_source_filter_remove(source, filter);
	};

	std::string undo_data(obs_data_get_json(wrapper));
	std::string redo_data(obs_data_get_json(rwrapper));
	main->undo_s.add_action(
		QTStr("Undo.Delete").arg(obs_source_get_name(filter)), undo,
		redo, undo_data, redo_data, false);
	obs_source_filter_remove(source, filter);
}
