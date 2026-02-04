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

#pragma once

#include <OBSApp.hpp>
#include "ui_OBSBasicSourceSelect.h"

#include <components/FlowLayout.hpp>
#include <components/SourceSelectButton.hpp>
#include <utility/undo_stack.hpp>
#include <widgets/OBSBasic.hpp>

#include <QButtonGroup>
#include <QDialog>

constexpr int UNVERSIONED_ID_ROLE = Qt::UserRole + 1;
constexpr int DEPRECATED_ROLE = Qt::UserRole + 2;

constexpr const char *RECENT_TYPE_ID = "_recent";
constexpr int RECENT_LIST_LIMIT = 16;

class OBSBasicSourceSelect : public QDialog {
	Q_OBJECT

public:
	OBSBasicSourceSelect(OBSBasic *parent, undo_stack &undo_s);
	~OBSBasicSourceSelect();

	OBSSource newSource;

	static void sourcePaste(SourceCopyInfo &info, bool duplicate);

private:
	std::unique_ptr<Ui::OBSBasicSourceSelect> ui;
	QString selectedTypeId{RECENT_TYPE_ID};
	undo_stack &undo_s;

	QPointer<QButtonGroup> sourceButtons;

	std::vector<OBSSignal> signalHandlers;
	static void obsSourceCreated(void *param, calldata_t *calldata);
	static void obsSourceRemoved(void *param, calldata_t *calldata);

	std::vector<obs_weak_source_t *> weakSources;

	QPointer<FlowLayout> existingFlowLayout = nullptr;

	void refreshSources();
	void updateExistingSources(int limit = 0);

	static bool enumSourcesCallback(void *data, obs_source_t *source);

	void rebuildSourceTypeList();

	int lastSelectedIndex = -1;
	std::vector<std::string> selectedItems;
	void addSelectedItem(std::string uuid);
	void removeSelectedItem(std::string uuid);
	void clearSelectedItems();

	SourceSelectButton *findButtonForUuid(std::string uuid);

	void createNew();
	void addExisting(std::string uuid, bool visible);

	void updateButtonVisibility();

signals:
	void sourcesUpdated();
	void selectedItemsChanged();

public slots:
	void on_createNewSource_clicked(bool checked);
	void addSelectedSources();

	void handleSourceCreated();
	void handleSourceRemoved(QString uuid);

	void sourceTypeSelected(QListWidgetItem *current, QListWidgetItem *previous);

	void sourceButtonToggled(QAbstractButton *button, bool checked);
	void sourceDropped(QString uuid);
};
