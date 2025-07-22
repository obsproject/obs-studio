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

#include <QWidget>
#include <QKeyEvent>
#include <QFocusEvent>

class AlignmentSelector : public QWidget {
	Q_OBJECT

	friend class AccessibleAlignmentSelector;
	friend class AccessibleAlignmentCell;

public:
	explicit AlignmentSelector(QWidget *parent = nullptr);

	QSize sizeHint() const override;
	QSize minimumSizeHint() const override;
	Qt::Alignment value() const;
	int currentIndex() const;

	void setAlignment(Qt::Alignment alignment);
	void setCurrentIndex(int index);

signals:
	void valueChanged(Qt::Alignment value);
	void currentIndexChanged(int value);

protected:
	QRect cellRect(int index) const;

	void paintEvent(QPaintEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void keyPressEvent(QKeyEvent *event) override;
	void focusInEvent(QFocusEvent *event) override;
	void focusOutEvent(QFocusEvent *event) override;

private:
	Qt::Alignment alignment = Qt::AlignTop | Qt::AlignLeft;

	int hoveredCell = -1;
	int focusedCell = 4;
	int selectedCell = 4;

	QRect gridRect() const;

	void moveFocusedCell(int moveX, int moveY);
	void setFocusedCell(int cell);
	void selectCell(int cell);
	Qt::Alignment cellAlignment(int index) const;
};
