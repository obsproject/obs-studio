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

#include <QMutex>
#include <QObject>

class RemuxWorker : public QObject {
	Q_OBJECT

	QMutex updateMutex;

	bool isWorking;

	float lastProgress;
	void UpdateProgress(float percent);

	explicit RemuxWorker() : isWorking(false) {}
	virtual ~RemuxWorker(){};

private slots:
	void remux(const QString &source, const QString &target);

signals:
	void updateProgress(float percent);
	void remuxFinished(bool success);

	friend class OBSRemux;
};
