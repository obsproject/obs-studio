/******************************************************************************
    Copyright (C) 2015 by Ruwen Hahn <palana@stunned.de>

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

#include <obs.hpp>

#include <QLabel>

class OBSSourceLabel : public QLabel {
	Q_OBJECT;

public:
	OBSSignal renamedSignal;
	OBSSignal removedSignal;
	OBSSignal destroyedSignal;

	OBSSourceLabel(const obs_source_t *source, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

protected:
	static void obsSourceRenamed(void *data, calldata_t *params);
	static void obsSourceRemoved(void *data, calldata_t *params);
	static void obsSourceDestroyed(void *data, calldata_t *params);
	void mousePressEvent(QMouseEvent *event);

signals:
	void renamed(const char *name);
	void removed();
	void destroyed();
	void clicked();
};
