/******************************************************************************
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

#include <QObject>
#include <QPointer>
#include <QWidget>

class PreviewProgramSizeObserver : public QObject {
	Q_OBJECT

	QPointer<QWidget> left;
	QPointer<QWidget> right;

	QPointer<QWidget> leftContainer;
	QPointer<QWidget> rightContainer;

	QSize leftOriginalMaxSize;
	QSize rightOriginalMaxSize;

	int leftTargetSize{};
	int rightTargetSize{};

	QPointer<QWidget> ancestorContainer;

	bool updating = false;

	Qt::Orientation orientation = Qt::Horizontal;

	std::pair<QWidget *, QWidget *> findSiblingParents(QWidget *a, QWidget *b);

	void syncContainerSizes(int containerSizeDelta);

public:
	PreviewProgramSizeObserver(QWidget *widgetLeft, QWidget *widgetRight, QObject *parent = nullptr);
	~PreviewProgramSizeObserver();

	void setOrientation(Qt::Orientation orientation_);

protected:
	bool eventFilter(QObject *target, QEvent *event) override;
};
