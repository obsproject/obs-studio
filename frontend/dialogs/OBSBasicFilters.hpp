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

#include "ui_OBSBasicFilters.h"

#include <QDialog>

class OBSBasic;
class OBSPropertiesView;

class OBSBasicFilters : public QDialog {
	Q_OBJECT

private:
	OBSBasic *main;

	std::unique_ptr<Ui::OBSBasicFilters> ui;
	OBSSource source;
	OBSPropertiesView *view = nullptr;

	OBSSignal addSignal;
	OBSSignal removeSignal;
	OBSSignal reorderSignal;

	OBSSignal removeSourceSignal;
	OBSSignal renameSourceSignal;
	OBSSignal updatePropertiesSignal;

	inline OBSSource GetFilter(int row, bool async);

	void UpdateFilters();
	void UpdateSplitter();
	void UpdateSplitter(bool show_splitter_frame);
	void UpdatePropertiesView(int row, bool async);

	static void OBSSourceFilterAdded(void *param, calldata_t *data);
	static void OBSSourceFilterRemoved(void *param, calldata_t *data);
	static void OBSSourceReordered(void *param, calldata_t *data);
	static void SourceRemoved(void *param, calldata_t *data);
	static void SourceRenamed(void *param, calldata_t *data);
	static void UpdateProperties(void *data, calldata_t *params);
	static void DrawPreview(void *data, uint32_t cx, uint32_t cy);

	QMenu *CreateAddFilterPopupMenu(bool async);

	void AddNewFilter(const char *id);
	void ReorderFilter(QListWidget *list, obs_source_t *filter, size_t idx);

	void CustomContextMenu(const QPoint &pos, bool async);
	void EditItem(QListWidgetItem *item, bool async);
	void DuplicateItem(QListWidgetItem *item);

	void FilterNameEdited(QWidget *editor, QListWidget *list);

	void delete_filter(OBSSource filter);

	bool isAsync;

	int noPreviewMargin;

	bool editActive = false;

private slots:
	void AddFilter(OBSSource filter, bool focus = true);
	void RemoveFilter(OBSSource filter);
	void ReorderFilters();
	void RenameAsyncFilter();
	void RenameEffectFilter();
	void ResetFilters();

	void on_addAsyncFilter_clicked();
	void on_removeAsyncFilter_clicked();
	void on_moveAsyncFilterUp_clicked();
	void on_moveAsyncFilterDown_clicked();
	void on_asyncFilters_currentRowChanged(int row);
	void on_asyncFilters_customContextMenuRequested(const QPoint &pos);
	void on_asyncFilters_GotFocus();

	void on_addEffectFilter_clicked();
	void on_removeEffectFilter_clicked();
	void on_moveEffectFilterUp_clicked();
	void on_moveEffectFilterDown_clicked();
	void on_effectFilters_currentRowChanged(int row);
	void on_effectFilters_customContextMenuRequested(const QPoint &pos);
	void on_effectFilters_GotFocus();

	void on_actionRemoveFilter_triggered();
	void on_actionMoveUp_triggered();
	void on_actionMoveDown_triggered();

	void on_actionRenameFilter_triggered();

	void CopyFilter();
	void PasteFilter();

	void FiltersMoved(const QModelIndex &srcParent, int srcIdxStart, int srcIdxEnd, const QModelIndex &dstParent,
			  int dstIdx);

public:
	OBSBasicFilters(QWidget *parent, OBSSource source_);
	~OBSBasicFilters();

	void Init();

	inline void UpdateSource(obs_source_t *target)
	{
		if (source == target)
			UpdateFilters();
	}

protected:
	virtual void closeEvent(QCloseEvent *event) override;
	virtual bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
};
