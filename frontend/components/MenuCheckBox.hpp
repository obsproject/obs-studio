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

#include <QCheckBox>
#include <QPointer>

class QStyleOptionButton;
class QStylePainter;
class QMouseEvent;

class MenuCheckBox : public QCheckBox {
	Q_OBJECT

public:
	explicit MenuCheckBox(const QString &text, QWidget *parent = nullptr);

	void setAction(QAction *action);

protected:
	void focusInEvent(QFocusEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void enterEvent(QEnterEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;

private:
	QPointer<QAction> menuAction = nullptr;
	bool mousePressInside = false;

	bool isHovered = false;
	void setHovered(bool hovered);
};
