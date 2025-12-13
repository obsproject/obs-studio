/******************************************************************************
    Copyright (C) 2019-2020 by Dillon Pentz <dillon@vodbox.io>

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

#include "ui_OBSImporter.h"

#include <QDialog>
#include <QPointer>

class ImporterModel;

class OBSImporter : public QDialog {
	Q_OBJECT

	QPointer<ImporterModel> optionsModel;
	std::unique_ptr<Ui::OBSImporter> ui;

public:
	explicit OBSImporter(QWidget *parent = nullptr);

	void addImportOption(QString path, bool automatic);

protected:
	virtual void dropEvent(QDropEvent *ev) override;
	virtual void dragEnterEvent(QDragEnterEvent *ev) override;

public slots:
	void browseImport();
	void importCollections();
	void dataChanged();
};
