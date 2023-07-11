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

#include <QWidget>
#include <QFrame>
#include <QLabel>
#include <QLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QCheckBox>

#include "obs-propertieslist.hpp"
#include "obs-toggleswitch.hpp"

/**
* Base class mostly so adding stuff to a list is easier
*/
class OBSActionBaseClass : public QFrame {
	Q_OBJECT

public:
	OBSActionBaseClass(QWidget *parent = nullptr) : QFrame(parent){};
};

/**
* Proxy for QScrollArea for QSS styling
*/
class OBSScrollArea : public QScrollArea {
	Q_OBJECT
public:
	OBSScrollArea(QWidget *parent = nullptr) : QScrollArea(parent) {}
};

/**
* Generic OBS row widget containing one or more controls
*/
class OBSActionRow : public OBSActionBaseClass {
	Q_OBJECT

public:
	OBSActionRow(const QString &name, QWidget *parent = nullptr);
	OBSActionRow(const QString &name, const QString &desc,
		     QWidget *parent = nullptr);

	void setPrefix(QWidget *w, bool auto_connect = true);
	void setSuffix(QWidget *w, bool auto_connect = true);

	QWidget *prefix() const { return _prefix; }
	QWidget *suffix() const { return _suffix; }

	void setPrefixEnabled(bool enabled);
	void setSuffixEnabled(bool enabled);

signals:
	void clicked();

protected:
	void mouseReleaseEvent(QMouseEvent *) override;
	bool hasSubtitle() const { return descLbl != nullptr; }

private:
	QGridLayout *layout;

	QLabel *nameLbl = nullptr;
	QLabel *descLbl = nullptr;

	QWidget *_prefix = nullptr;
	QWidget *_suffix = nullptr;

	void autoConnectWidget(QWidget *w);
};

/**
* Collapsible Generic OBS property container
*/
class OBSCollapsibleActionRow : public OBSActionBaseClass {
	Q_OBJECT

public:
	// ToDo: Figure out a better way to handle those options
	OBSCollapsibleActionRow(const QString &name,
				const QString &desc = nullptr,
				bool toggleable = false,
				QWidget *parent = nullptr);

	OBSToggleSwitch *getSwitch() const { return sw; }

	void addRow(OBSActionBaseClass *ar);

private:
	void toggleVisibility();

	QPixmap extendDown;
	QPixmap extendUp;

	QVBoxLayout *layout;
	QLabel *extendIcon;

	OBSActionRow *ar;
	OBSToggleSwitch *sw = nullptr;
	OBSPropertiesList *plist;
};
