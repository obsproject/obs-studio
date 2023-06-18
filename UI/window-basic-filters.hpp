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

#pragma once

#include <QWidget>
#include <memory>
#include <obs.hpp>

#include "properties-view.hpp"

class OBSBasic;
class QMenu;

#include "ui_OBSBasicFilters.h"

class OBSBasicFilters : public QWidget {
	Q_OBJECT

private:
	OBSBasic *main;

	std::unique_ptr<Ui::OBSBasicFilters> ui;
	OBSSource source;
	OBSPropertiesView *view = nullptr;

	OBSSignal addSignal;
	OBSSignal removeSignal;
	OBSSignal reorderSignal;

	OBSSignal updatePropertiesSignal;

	inline OBSSource GetFilter(int row);

	void UpdateFilters();
	void UpdatePropertiesView(int row);

	static void OBSSourceFilterAdded(void *param, calldata_t *data);
	static void OBSSourceFilterRemoved(void *param, calldata_t *data);
	static void OBSSourceReordered(void *param, calldata_t *data);
	static void UpdateProperties(void *data, calldata_t *params);

	QMenu *CreateAddFilterPopupMenu();

	void AddNewFilter(const char *id);
	void ReorderFilter(QListWidget *list, obs_source_t *filter, size_t idx);

	void CustomContextMenu(const QPoint &pos);
	void EditItem(QListWidgetItem *item);
	void DuplicateItem(QListWidgetItem *item);

	void FilterNameEdited(QWidget *editor, QListWidget *list);

	void delete_filter(OBSSource filter);

	bool isAsync;

	bool editActive = false;

private slots:
	void AddFilter(OBSSource filter, bool focus = true);
	void RemoveFilter(OBSSource filter);
	void ReorderFilters();
	void RenameFilter();
	void DuplicateFilter();

	void AddFilterFromAction();

	void on_addFilter_clicked();
	void on_removeFilter_clicked();
	void on_moveFilterUp_clicked();
	void on_moveFilterDown_clicked();
	void on_filtersList_currentRowChanged(int row);
	void on_filtersList_customContextMenuRequested(const QPoint &pos);

	void on_actionRemoveFilter_triggered();
	void on_actionMoveUp_triggered();
	void on_actionMoveDown_triggered();

	void on_actionRenameFilter_triggered();

	void FilterNameEdited(QWidget *editor);

	void CopyFilter();
	void PasteFilter();

public slots:
	void ResetFilters();

public:
	OBSBasicFilters(QWidget *parent, OBSSource source_, bool isAsync);
	~OBSBasicFilters();

	inline void UpdateSource(obs_source_t *target)
	{
		if (source == target)
			UpdateFilters();
	}

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
	virtual bool nativeEvent(const QByteArray &eventType, void *message,
				 qintptr *result) override;
#else
	virtual bool nativeEvent(const QByteArray &eventType, void *message,
				 long *result) override;
#endif
};
