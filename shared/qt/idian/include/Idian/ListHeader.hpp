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

#include <Idian/RowList.hpp>
#include <Idian/ToggleSwitch.hpp>

#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QWidget>

namespace idian {

class ListHeader : public QFrame, public Utils {
	Q_OBJECT

public:
	ListHeader(QWidget *parent = nullptr);
	ListHeader(QWidget *parent, QString title);
	ListHeader(QWidget *parent, QString title, QString description);

	void setTitle(QString name);
	void setDescription(QString desc);

	void showTitle(bool visible);
	void showDescription(bool visible);

	void setCheckable(bool check);
	bool isCheckable() { return checkable; }

private:
	Utils *widgetUtils;

	QHBoxLayout *layout_ = nullptr;

	QLabel *nameLabel = nullptr;
	QLabel *descriptionLabel = nullptr;

	ToggleSwitch *toggleSwitch = nullptr;
	bool checkable = false;

signals:
	void toggled(bool enable);
};
} // namespace idian
