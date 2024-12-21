/******************************************************************************
    Copyright (C) 2014 by Ruwen Hahn <palana@stunned.de>

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

#include "ui_OBSRemux.h"

#include <QPointer>
#include <QThread>

class RemuxQueueModel;
class RemuxWorker;

class OBSRemux : public QDialog {
	Q_OBJECT

	QPointer<RemuxQueueModel> queueModel;
	QThread remuxer;
	QPointer<RemuxWorker> worker;

	std::unique_ptr<Ui::OBSRemux> ui;

	const char *recPath;

	virtual void closeEvent(QCloseEvent *event) override;
	virtual void reject() override;

	bool autoRemux;
	QString autoRemuxFile;

public:
	explicit OBSRemux(const char *recPath, QWidget *parent = nullptr, bool autoRemux = false);
	virtual ~OBSRemux() override;

	using job_t = std::shared_ptr<struct media_remux_job>;

	void AutoRemux(QString inFile, QString outFile);

protected:
	virtual void dropEvent(QDropEvent *ev) override;
	virtual void dragEnterEvent(QDragEnterEvent *ev) override;

	void remuxNextEntry();

private slots:
	void rowCountChanged(const QModelIndex &parent, int first, int last);

public slots:
	void updateProgress(float percent);
	void remuxFinished(bool success);
	void beginRemux();
	bool stopRemux();
	void clearFinished();
	void clearAll();

signals:
	void remux(const QString &source, const QString &target);
};
