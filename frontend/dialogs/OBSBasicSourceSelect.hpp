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

#include "ui_OBSBasicSourceSelect.h"

#include <QButtonGroup>
#include <components/FlowLayout.hpp>
#include <components/SourceSelectButton.hpp>
#include <utility/undo_stack.hpp>
#include <widgets/OBSBasic.hpp>

#include "OBSApp.hpp"

#include <QDialog>

enum SourceTypeRoles {
	UNVERSIONED_ID = Qt::UserRole + 1,
};

class OBSBasicSourceSelect : public QDialog {
	Q_OBJECT

private:
	std::unique_ptr<Ui::OBSBasicSourceSelect> ui;
	QString sourceTypeId;
	undo_stack &undo_s;

	QPointer<QButtonGroup> sourceButtons;

	std::vector<obs_source_t *> sources;
	std::vector<obs_source_t *> groups;

	FlowLayout *existingFlowLayout = nullptr;

	void getSources();
	void updateExistingSources(int limit = 0);

	static bool enumSourcesCallback(void *data, obs_source_t *source);
	static bool enumGroupsCallback(void *data, obs_source_t *source);

	static void OBSSourceRemoved(void *data, calldata_t *calldata);
	static void OBSSourceAdded(void *data, calldata_t *calldata);

	//static int getSortedButtonPosition(const QList<QPushButton *> *list, const char *name);
	//QPointer<QPushButton> createTypeButton(const char *type, const char *name);

	void getSourceTypes();
	void setSelectedSourceType(QListWidgetItem *item);

	SourceSelectButton *selectedSource = nullptr;
	void setSelectedSource(SourceSelectButton *button);

	void createNewSource();

signals:
	void sourcesUpdated();

private slots:
	void on_createNewSource_clicked(bool checked);
	void addExistingSource();

	// void SourceAdded(OBSSource source);
	// void SourceRemoved(OBSSource source);
	void sourceTypeSelected(QListWidgetItem *current, QListWidgetItem *previous);
	void sourceTypeClicked(QListWidgetItem *clicked);

	void sourceButtonClicked(QAbstractButton *button);

public:
	OBSBasicSourceSelect(OBSBasic *parent, undo_stack &undo_s);
	~OBSBasicSourceSelect();

	OBSSource newSource;

	static void SourcePaste(SourceCopyInfo &info, bool duplicate);
};
