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

#include <Idian/PropertiesList.hpp>
#include <Idian/Row.hpp>
#include <Idian/ToggleSwitch.hpp>

#include <QLabel>
#include <QLayout>
#include <QMouseEvent>
#include <QWidget>

namespace idian {

class Group : public QFrame, public Utils {
	Q_OBJECT

public:
	Group(QWidget *parent = nullptr);

	PropertiesList *properties() const { return propertyList; }

	void addRow(GenericRow *row) const;

	void setTitle(QString name);
	void setDescription(QString desc);

	void showTitle(bool visible);
	void showDescription(bool visible);

	void setCheckable(bool check);
	bool isCheckable() { return checkable; }

private:
	QVBoxLayout *layout = nullptr;

	QWidget *headerContainer = nullptr;
	QHBoxLayout *headerLayout = nullptr;
	QWidget *labelContainer = nullptr;
	QVBoxLayout *labelLayout = nullptr;
	QWidget *controlContainer = nullptr;
	QVBoxLayout *controlLayout = nullptr;

	QWidget *contentsContainer = nullptr;
	QVBoxLayout *contentsLayout = nullptr;

	QLabel *nameLabel = nullptr;
	QLabel *descriptionLabel = nullptr;

	PropertiesList *propertyList = nullptr;

	ToggleSwitch *toggleSwitch = nullptr;
	bool checkable = false;
};
} // namespace idian
