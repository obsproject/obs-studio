/******************************************************************************
    Copyright (C) 2023 by Sebastian Beckmann

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

#include <memory>

#include "ui_OBSSceneCollections.h"

#include "scene-collections-util.hpp"

class OBSSceneCollections : public QDialog {
	Q_OBJECT

public:
	OBSSceneCollections(QWidget *parent);
	void refreshList();

signals:
	void collectionsChanged();

private:
	std::unique_ptr<Ui::OBSSceneCollections> ui;

	void setBulkMode(bool bulk);

	struct SelectedRowInfo {
		std::string name;
		std::string file;
		bool is_current_collection;
	};
	QList<SelectedRowInfo> selectedRows();
	SceneCollectionOrder collections_order = kSceneCollectionOrderLastUsed;

	void SCRename(const std::string &current_name, const std::string &file);
	void SCDuplicate(const std::string &name, const std::string &file);
	void SCDelete(const std::string &current_name,
		      const std::string &current_file);
	void SCExport(const std::string &file);

private slots:
	void updateBulkButtons();

	void on_lineeditSearch_textChanged(const QString &text);
	void on_buttonBulkMode_toggled(bool checked);
	void on_buttonSort_pressed();
	void on_buttonNew_pressed();
	void on_buttonImport_pressed();
	void on_buttonExportBulk_pressed();
	void on_buttonDuplicateBulk_pressed();
	void on_buttonDeleteBulk_pressed();

	void RowDoubleClicked(QObject *obj);
};
