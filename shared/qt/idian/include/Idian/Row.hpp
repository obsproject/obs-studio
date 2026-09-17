/******************************************************************************
    Copyright (C) 2023 by Dennis Sädtler <dennis@obsproject.com>

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

#include <Idian/RowInfo.hpp>
#include <Idian/ToggleSwitch.hpp>
#include <Idian/Utils.hpp>

#include <QCheckBox>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QScrollArea>
#include <QWidget>

namespace idian {
class ExpandButton;
class RowList;
class RowInfo;

// Row widget containing one or more controls
class Row : public QFrame, public Utils {
	Q_OBJECT

public:
	Row(QWidget *parent = nullptr);

	void setChangeCursor(bool change);

	QHBoxLayout *layout() { return rowLayout; }

	// Convenience function to add a widget to the rows layout.
	void addWidget(QWidget *widget) { layout()->addWidget(widget); }

	// Convenience function to add a widget to the rows layout and then set it as the buddy.
	void addBuddy(QWidget *widget)
	{
		layout()->addWidget(widget);
		setBuddy(widget);
	}

	// Sets this widget as the buddy of the Row.
	// The idian StateEventFilter on Row means certain style-able classes are applied to it based on interaction
	// events. The buddy widget will be repolished when these events happen. This allows the buddy widget to be
	// styled differently based on the state of the row. For certain widgets, a slot will also be triggered
	// on the buddy widget.
	void setBuddy(QWidget *w);

signals:
	void clicked();

protected:
	void enterEvent(QEnterEvent *) override;
	void leaveEvent(QEvent *) override;
	void mouseReleaseEvent(QMouseEvent *) override;
	void keyReleaseEvent(QKeyEvent *) override;

private:
	QHBoxLayout *rowLayout;

	QWidget *buddyWidget = nullptr;

	void connectBuddyWidget(QWidget *widget);
	bool changeCursor = false;
};
} // namespace idian
