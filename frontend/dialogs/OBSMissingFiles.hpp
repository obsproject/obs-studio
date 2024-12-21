/******************************************************************************
    Copyright (C) 2019 by Dillon Pentz <dillon@vodbox.io>

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

#include "ui_OBSMissingFiles.h"

#include <obs.hpp>

#include <QDialog>
#include <QPointer>

class MissingFilesModel;

class OBSMissingFiles : public QDialog {
	Q_OBJECT
	Q_PROPERTY(QIcon warningIcon READ GetWarningIcon WRITE SetWarningIcon DESIGNABLE true)

	QPointer<MissingFilesModel> filesModel;
	std::unique_ptr<Ui::OBSMissingFiles> ui;

public:
	explicit OBSMissingFiles(obs_missing_files_t *files, QWidget *parent = nullptr);
	virtual ~OBSMissingFiles() override;

	void addMissingFile(const char *originalPath, const char *sourceName);

	QIcon GetWarningIcon();
	void SetWarningIcon(const QIcon &icon);

private:
	void saveFiles();
	void browseFolders();

	obs_missing_files_t *fileStore;

public slots:
	void dataChanged();
};
