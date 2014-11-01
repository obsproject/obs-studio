/******************************************************************************
    Copyright (C) 2015 by Hugh Bailey <obs.jim@gmail.com>

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

#include "window-namedialog.hpp"
#include "window-basic-filters.hpp"
#include "display-helpers.hpp"
#include "qt-wrappers.hpp"
#include "visibility-item-widget.hpp"
#include "obs-app.hpp"

#include <QMessageBox>
#include <QCloseEvent>
#include <string>
#include <QMenu>
#include <QVariant>

using namespace std;

Q_DECLARE_METATYPE(OBSSource);

OBSBasicFilters::OBSBasicFilters(QWidget *parent, OBSSource source_)
	: QDialog                      (parent),
	  ui                           (new Ui::OBSBasicFilters),
	  source                       (source_),
	  addSignal                    (obs_source_get_signal_handler(source),
	                                "filter_add",
	                                OBSBasicFilters::OBSSourceFilterAdded,
	                                this),
	  removeSignal                 (obs_source_get_signal_handler(source),
	                                "filter_remove",
	                                OBSBasicFilters::OBSSourceFilterRemoved,
	                                this),
	  reorderSignal                (obs_source_get_signal_handler(source),
	                                "reorder_filters",
	                                OBSBasicFilters::OBSSourceReordered,
	                                this),
	  removeSourceSignal           (obs_source_get_signal_handler(source),
	                                "remove",
	                                OBSBasicFilters::SourceRemoved, this),
	  renameSourceSignal           (obs_source_get_signal_handler(source),
	                                "rename",
	                                OBSBasicFilters::SourceRenamed, this)
{
	ui->setupUi(this);
	UpdateFilters();

	ui->asyncFilters->setItemDelegate(
			new VisibilityItemDelegate(ui->asyncFilters));
	ui->effectFilters->setItemDelegate(
			new VisibilityItemDelegate(ui->effectFilters));

	const char *name = obs_source_get_name(source);
	setWindowTitle(QTStr("Basic.Filters.Title").arg(QT_UTF8(name)));

	connect(ui->preview, SIGNAL(DisplayResized()),
			this, SLOT(OnPreviewResized()));

	connect(ui->asyncFilters->itemDelegate(),
			SIGNAL(closeEditor(QWidget*,
					QAbstractItemDelegate::EndEditHint)),
			this,
			SLOT(AsyncFilterNameEdited(QWidget*,
					QAbstractItemDelegate::EndEditHint)));

	connect(ui->effectFilters->itemDelegate(),
			SIGNAL(closeEditor(QWidget*,
					QAbstractItemDelegate::EndEditHint)),
			this,
			SLOT(EffectFilterNameEdited(QWidget*,
					QAbstractItemDelegate::EndEditHint)));

	uint32_t flags = obs_source_get_output_flags(source);
	bool audio     = (flags & OBS_SOURCE_AUDIO) != 0;
	bool audioOnly = (flags & OBS_SOURCE_VIDEO) == 0;
	bool async     = (flags & OBS_SOURCE_ASYNC) != 0;

	if (!async && !audio) {
		ui->asyncWidget->setVisible(false);
		ui->separatorLine->setVisible(false);
	}
	if (audioOnly) {
		ui->effectWidget->setVisible(false);
		ui->separatorLine->setVisible(false);
	}

	if (audioOnly || (audio && !async))
		ui->asyncLabel->setText(QTStr("Basic.Filters.AudioFilters"));
}

OBSBasicFilters::~OBSBasicFilters()
{
	ui->asyncFilters->clear();
	ui->effectFilters->clear();
	QApplication::sendPostedEvents(this);
}

void OBSBasicFilters::Init()
{
	gs_init_data init_data = {};

	show();

	QSize previewSize = GetPixelSize(ui->preview);
	init_data.cx      = uint32_t(previewSize.width());
	init_data.cy      = uint32_t(previewSize.height());
	init_data.format  = GS_RGBA;
	QTToGSWindow(ui->preview->winId(), init_data.window);

	display = obs_display_create(&init_data);

	if (display)
		obs_display_add_draw_callback(display,
				OBSBasicFilters::DrawPreview, this);
}

inline OBSSource OBSBasicFilters::GetFilter(int row, bool async)
{
	if (row == -1)
		return OBSSource();

	QListWidget *list = async ? ui->asyncFilters : ui->effectFilters;
	QListWidgetItem *item = list->item(row);
	if (!item)
		return OBSSource();

	QVariant v = item->data(Qt::UserRole);
	return v.value<OBSSource>();
}

void OBSBasicFilters::UpdatePropertiesView(int row, bool async)
{
	delete view;
	view = nullptr;

	OBSSource filter = GetFilter(row, async);
	if (!filter)
		return;

	obs_data_t *settings = obs_source_get_settings(filter);

	view = new OBSPropertiesView(settings, filter,
			(PropertiesReloadCallback)obs_source_properties,
			(PropertiesUpdateCallback)obs_source_update);

	obs_data_release(settings);

	view->setMaximumHeight(250);
	view->setMinimumHeight(150);
	ui->rightLayout->addWidget(view);
	view->show();
}

void OBSBasicFilters::AddFilter(OBSSource filter)
{
	uint32_t flags = obs_source_get_output_flags(filter);
	bool async = (flags & OBS_SOURCE_ASYNC) != 0;
	QListWidget *list = async ? ui->asyncFilters : ui->effectFilters;

	QListWidgetItem *item = new QListWidgetItem();
	Qt::ItemFlags itemFlags = item->flags();

	item->setFlags(itemFlags | Qt::ItemIsEditable);
	item->setData(Qt::UserRole, QVariant::fromValue(filter));

	list->addItem(item);
	list->setCurrentItem(item);
	SetupVisibilityItem(list, item, filter);
}

void OBSBasicFilters::RemoveFilter(OBSSource filter)
{
	uint32_t flags = obs_source_get_output_flags(filter);
	bool async = (flags & OBS_SOURCE_ASYNC) != 0;
	QListWidget *list = async ? ui->asyncFilters : ui->effectFilters;

	for (int i = 0; i < list->count(); i++) {
		QListWidgetItem *item = list->item(i);
		QVariant v = item->data(Qt::UserRole);
		OBSSource curFilter = v.value<OBSSource>();

		if (filter == curFilter) {
			delete item;
			break;
		}
	}
}

struct FilterOrderInfo {
	int asyncIdx = 0;
	int effectIdx = 0;
	OBSBasicFilters *window;

	inline FilterOrderInfo(OBSBasicFilters *window_) : window(window_) {}
};

void OBSBasicFilters::ReorderFilter(QListWidget *list,
		obs_source_t *filter, size_t idx)
{
	int count = list->count();

	for (int i = 0; i < count; i++) {
		QListWidgetItem *listItem = list->item(i);
		QVariant v = listItem->data(Qt::UserRole);
		OBSSource filterItem = v.value<OBSSource>();

		if (filterItem == filter) {
			if ((int)idx != i) {
				bool sel = (list->currentRow() == i);

				listItem = list->takeItem(i);
				if (listItem)  {
					list->insertItem((int)idx, listItem);
					SetupVisibilityItem(list,
							listItem, filterItem);

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

	obs_source_enum_filters(source,
			[] (obs_source_t*, obs_source_t *filter, void *p)
			{
				FilterOrderInfo *info =
					reinterpret_cast<FilterOrderInfo*>(p);
				uint32_t flags;
				bool async;

				flags = obs_source_get_output_flags(filter);
				async = (flags & OBS_SOURCE_ASYNC) != 0;

				if (async) {
					info->window->ReorderFilter(
						info->window->ui->asyncFilters,
						filter, info->asyncIdx++);
				} else {
					info->window->ReorderFilter(
						info->window->ui->effectFilters,
						filter, info->effectIdx++);
				}
			}, &info);
}

void OBSBasicFilters::UpdateFilters()
{
	if (!source)
		return;

	ui->effectFilters->clear();
	ui->asyncFilters->clear();

	obs_source_enum_filters(source,
			[] (obs_source_t*, obs_source_t *filter, void *p)
			{
				OBSBasicFilters *window =
					reinterpret_cast<OBSBasicFilters*>(p);

				window->AddFilter(filter);
			}, this);
}

static bool filter_compatible(bool async, uint32_t sourceFlags,
		uint32_t filterFlags)
{
	bool filterVideo = (filterFlags & OBS_SOURCE_VIDEO) != 0;
	bool filterAsync = (filterFlags & OBS_SOURCE_ASYNC) != 0;
	bool filterAudio = (filterFlags & OBS_SOURCE_AUDIO) != 0;
	bool audio       = (sourceFlags & OBS_SOURCE_AUDIO) != 0;
	bool audioOnly   = (sourceFlags & OBS_SOURCE_VIDEO) == 0;
	bool asyncSource = (sourceFlags & OBS_SOURCE_ASYNC) != 0;

	if (async && ((audioOnly && filterVideo) || (!audio && !asyncSource)))
		return false;

	return (async && (filterAudio || filterAsync)) ||
		(!async && !filterAudio && !filterAsync);
}

QMenu *OBSBasicFilters::CreateAddFilterPopupMenu(bool async)
{
	uint32_t sourceFlags = obs_source_get_output_flags(source);
	const char *type;
	bool foundValues = false;
	size_t idx = 0;

	QMenu *popup = new QMenu(QTStr("Add"), this);
	while (obs_enum_filter_types(idx++, &type)) {
		const char *name = obs_source_get_display_name(
				OBS_SOURCE_TYPE_FILTER, type);
		uint32_t filterFlags = obs_get_source_output_flags(
				OBS_SOURCE_TYPE_FILTER, type);

		if (!filter_compatible(async, sourceFlags, filterFlags))
			continue;

		QAction *popupItem = new QAction(QT_UTF8(name), this);
		popupItem->setData(QT_UTF8(type));
		connect(popupItem, SIGNAL(triggered(bool)),
				this, SLOT(AddFilterFromAction()));
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
		obs_source_t *existing_filter;
		string name = obs_source_get_display_name(
				OBS_SOURCE_TYPE_FILTER, id);

		bool success = NameDialog::AskForName(this,
				QTStr("Basic.Filters.AddFilter.Title"),
				QTStr("Basic.FIlters.AddFilter.Text"), name,
				QT_UTF8(name.c_str()));
		if (!success)
			return;

		if (name.empty()) {
			QMessageBox::information(this,
					QTStr("NoNameEntered.Title"),
					QTStr("NoNameEntered.Text"));
			AddNewFilter(id);
			return;
		}

		existing_filter = obs_source_get_filter_by_name(source,
				name.c_str());
		if (existing_filter) {
			QMessageBox::information(this,
					QTStr("NameExists.Title"),
					QTStr("NameExists.Text"));
			obs_source_release(existing_filter);
			AddNewFilter(id);
			return;
		}

		obs_source_t *filter = obs_source_create(OBS_SOURCE_TYPE_FILTER,
				id, name.c_str(), nullptr, nullptr);
		if (filter) {
			obs_source_filter_add(source, filter);
			obs_source_release(filter);
		}
	}
}

void OBSBasicFilters::AddFilterFromAction()
{
	QAction *action = qobject_cast<QAction*>(sender());
	if (!action)
		return;

	AddNewFilter(QT_TO_UTF8(action->data().toString()));
}

void OBSBasicFilters::OnPreviewResized()
{
	if (resizeTimer)
		killTimer(resizeTimer);
	resizeTimer = startTimer(100);
}

void OBSBasicFilters::closeEvent(QCloseEvent *event)
{
	QDialog::closeEvent(event);
	if (!event->isAccepted())
		return;

	// remove draw callback and release display in case our drawable
	// surfaces go away before the destructor gets called
	obs_display_remove_draw_callback(display,
			OBSBasicFilters::DrawPreview, this);
	display = nullptr;
}

void OBSBasicFilters::timerEvent(QTimerEvent *event)
{
	if (event->timerId() == resizeTimer) {
		killTimer(resizeTimer);
		resizeTimer = 0;

		QSize size = GetPixelSize(ui->preview);
		obs_display_resize(display, size.width(), size.height());
	}
}

/* OBS Signals */

void OBSBasicFilters::OBSSourceFilterAdded(void *param, calldata_t *data)
{
	OBSBasicFilters *window = reinterpret_cast<OBSBasicFilters*>(param);
	obs_source_t *filter = (obs_source_t*)calldata_ptr(data, "filter");

	QMetaObject::invokeMethod(window, "AddFilter",
			Q_ARG(OBSSource, OBSSource(filter)));
}

void OBSBasicFilters::OBSSourceFilterRemoved(void *param, calldata_t *data)
{
	OBSBasicFilters *window = reinterpret_cast<OBSBasicFilters*>(param);
	obs_source_t *filter = (obs_source_t*)calldata_ptr(data, "filter");

	QMetaObject::invokeMethod(window, "RemoveFilter",
			Q_ARG(OBSSource, OBSSource(filter)));
}

void OBSBasicFilters::OBSSourceReordered(void *param, calldata_t *data)
{
	QMetaObject::invokeMethod(reinterpret_cast<OBSBasicFilters*>(param),
			"ReorderFilters");

	UNUSED_PARAMETER(data);
}

void OBSBasicFilters::SourceRemoved(void *data, calldata_t *params)
{
	UNUSED_PARAMETER(params);

	QMetaObject::invokeMethod(static_cast<OBSBasicFilters*>(data),
	                "close");
}

void OBSBasicFilters::SourceRenamed(void *data, calldata_t *params)
{
	const char *name = calldata_string(params, "new_name");
	QString title = QTStr("Basic.Filters.Title").arg(QT_UTF8(name));

	QMetaObject::invokeMethod(static_cast<OBSBasicFilters*>(data),
	                "setWindowTitle", Q_ARG(QString, title));
}

void OBSBasicFilters::DrawPreview(void *data, uint32_t cx, uint32_t cy)
{
	OBSBasicFilters *window = static_cast<OBSBasicFilters*>(data);

	if (!window->source)
		return;

	uint32_t sourceCX = max(obs_source_get_width(window->source), 1u);
	uint32_t sourceCY = max(obs_source_get_height(window->source), 1u);

	int   x, y;
	int   newCX, newCY;
	float scale;

	GetScaleAndCenterPos(sourceCX, sourceCY, cx, cy, x, y, scale);

	newCX = int(scale * float(sourceCX));
	newCY = int(scale * float(sourceCY));

	gs_viewport_push();
	gs_projection_push();
	gs_ortho(0.0f, float(sourceCX), 0.0f, float(sourceCY), -100.0f, 100.0f);
	gs_set_viewport(x, y, newCX, newCY);

	obs_source_video_render(window->source);

	gs_projection_pop();
	gs_viewport_pop();
}

/* Qt Slots */

static bool QueryRemove(QWidget *parent, obs_source_t *source)
{
	const char *name  = obs_source_get_name(source);

	QString text = QTStr("ConfirmRemove.Text");
	text.replace("$1", QT_UTF8(name));

	QMessageBox remove_source(parent);
	remove_source.setText(text);
	QAbstractButton *Yes = remove_source.addButton(QTStr("Yes"),
			QMessageBox::YesRole);
	remove_source.addButton(QTStr("No"), QMessageBox::NoRole);
	remove_source.setIcon(QMessageBox::Question);
	remove_source.setWindowTitle(QTStr("ConfirmRemove.Title"));
	remove_source.exec();

	return Yes == remove_source.clickedButton();
}

void OBSBasicFilters::on_addAsyncFilter_clicked()
{
	QPointer<QMenu> popup = CreateAddFilterPopupMenu(true);
	if (popup)
		popup->exec(QCursor::pos());
}

void OBSBasicFilters::on_removeAsyncFilter_clicked()
{
	OBSSource filter = GetFilter(ui->asyncFilters->currentRow(), true);
	if (filter) {
		if (QueryRemove(this, filter))
			obs_source_filter_remove(source, filter);
	}
}

void OBSBasicFilters::on_moveAsyncFilterUp_clicked()
{
	OBSSource filter = GetFilter(ui->asyncFilters->currentRow(), true);
	if (filter)
		obs_source_filter_set_order(source, filter, OBS_ORDER_MOVE_UP);
}

void OBSBasicFilters::on_moveAsyncFilterDown_clicked()
{
	OBSSource filter = GetFilter(ui->asyncFilters->currentRow(), true);
	if (filter)
		obs_source_filter_set_order(source, filter,
				OBS_ORDER_MOVE_DOWN);
}

void OBSBasicFilters::on_asyncFilters_GotFocus()
{
	UpdatePropertiesView(ui->asyncFilters->currentRow(), true);
}

void OBSBasicFilters::on_asyncFilters_currentRowChanged(int row)
{
	UpdatePropertiesView(row, true);
}

void OBSBasicFilters::on_addEffectFilter_clicked()
{
	QPointer<QMenu> popup = CreateAddFilterPopupMenu(false);
	if (popup)
		popup->exec(QCursor::pos());
}

void OBSBasicFilters::on_removeEffectFilter_clicked()
{
	OBSSource filter = GetFilter(ui->effectFilters->currentRow(), false);
	if (filter) {
		if (QueryRemove(this, filter))
			obs_source_filter_remove(source, filter);
	}
}

void OBSBasicFilters::on_moveEffectFilterUp_clicked()
{
	OBSSource filter = GetFilter(ui->effectFilters->currentRow(), false);
	if (filter)
		obs_source_filter_set_order(source, filter, OBS_ORDER_MOVE_UP);
}

void OBSBasicFilters::on_moveEffectFilterDown_clicked()
{
	OBSSource filter = GetFilter(ui->effectFilters->currentRow(), false);
	if (filter)
		obs_source_filter_set_order(source, filter,
				OBS_ORDER_MOVE_DOWN);
}

void OBSBasicFilters::on_effectFilters_GotFocus()
{
	UpdatePropertiesView(ui->effectFilters->currentRow(), false);
}

void OBSBasicFilters::on_effectFilters_currentRowChanged(int row)
{
	UpdatePropertiesView(row, false);
}

void OBSBasicFilters::CustomContextMenu(const QPoint &pos, bool async)
{
	QListWidget *list = async ? ui->asyncFilters : ui->effectFilters;
	QListWidgetItem *item = list->itemAt(pos);

	QMenu popup(window());

	QPointer<QMenu> addMenu = CreateAddFilterPopupMenu(async);
	if (addMenu)
		popup.addMenu(addMenu);

	if (item) {
		const char *renameSlot = async ?
			SLOT(RenameAsyncFilter()) : SLOT(RenameEffectFilter());
		const char *removeSlot = async ?
			SLOT(on_removeAsyncFilter_clicked()) :
			SLOT(on_removeEffectFilter_clicked());

		popup.addSeparator();
		popup.addAction(QTStr("Rename"), this, renameSlot);
		popup.addAction(QTStr("Remove"), this, removeSlot);
	}

	popup.exec(QCursor::pos());
}

void OBSBasicFilters::EditItem(QListWidgetItem *item, bool async)
{
	Qt::ItemFlags flags = item->flags();
	OBSSource filter    = item->data(Qt::UserRole).value<OBSSource>();
	const char *name    = obs_source_get_name(filter);
	QListWidget *list   = async ? ui->asyncFilters : ui->effectFilters;

	item->setText(QT_UTF8(name));
	item->setFlags(flags | Qt::ItemIsEditable);
	list->removeItemWidget(item);
	list->editItem(item);
	item->setFlags(flags);
}

void OBSBasicFilters::on_asyncFilters_customContextMenuRequested(
		const QPoint &pos)
{
	CustomContextMenu(pos, true);
}

void OBSBasicFilters::on_effectFilters_customContextMenuRequested(
		const QPoint &pos)
{
	CustomContextMenu(pos, false);
}

void OBSBasicFilters::RenameAsyncFilter()
{
	EditItem(ui->asyncFilters->currentItem(), true);
}

void OBSBasicFilters::RenameEffectFilter()
{
	EditItem(ui->effectFilters->currentItem(), false);
}

void OBSBasicFilters::FilterNameEdited(QWidget *editor, QListWidget *list)
{
	QListWidgetItem *listItem = list->currentItem();
	OBSSource filter = listItem->data(Qt::UserRole).value<OBSSource>();
	QLineEdit *edit = qobject_cast<QLineEdit*>(editor);
	string name = QT_TO_UTF8(edit->text().trimmed());

	const char *prevName = obs_source_get_name(filter);
	bool sameName = (name == prevName);
	obs_source_t *foundFilter = nullptr;

	if (!sameName)
		foundFilter = obs_source_get_filter_by_name(source,
				name.c_str());

	if (foundFilter || name.empty() || sameName) {
		listItem->setText(QT_UTF8(prevName));

		if (foundFilter) {
			QMessageBox::information(window(),
				QTStr("NameExists.Title"),
				QTStr("NameExists.Text"));
			obs_source_release(foundFilter);

		} else if (name.empty()) {
			QMessageBox::information(window(),
				QTStr("NoNameEntered.Title"),
				QTStr("NoNameEntered.Text"));
		}
	} else {
		listItem->setText(QT_UTF8(name.c_str()));
		obs_source_set_name(filter, name.c_str());
	}

	listItem->setText(QString());
	SetupVisibilityItem(list, listItem, filter);
}

void OBSBasicFilters::AsyncFilterNameEdited(QWidget *editor,
		QAbstractItemDelegate::EndEditHint endHint)
{
	FilterNameEdited(editor, ui->asyncFilters);
	UNUSED_PARAMETER(endHint);
}

void OBSBasicFilters::EffectFilterNameEdited(QWidget *editor,
		QAbstractItemDelegate::EndEditHint endHint)
{
	FilterNameEdited(editor, ui->effectFilters);
	UNUSED_PARAMETER(endHint);
}
