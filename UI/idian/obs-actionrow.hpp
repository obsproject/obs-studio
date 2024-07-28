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

#include "obs-widgets-base.hpp"
#include "obs-propertieslist.hpp"
#include "obs-toggleswitch.hpp"
#include "obs-combobox.hpp"
#include "obs-controls.hpp"

/**
* Base class mostly so adding stuff to a list is easier
*/
class OBSActionBaseClass : public QFrame, public OBSWidgetUtils {
	Q_OBJECT

public:
	OBSActionBaseClass(QWidget *parent = nullptr)
		: QFrame(parent),
		  OBSWidgetUtils(this)
	{
		setAccessibleName("");
	};
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

	bool hasPrefix() { return _prefix; }
	bool hasSuffix() { return _suffix; }

	QWidget *prefix() const { return _prefix; }
	QWidget *suffix() const { return _suffix; }

	void setPrefixEnabled(bool enabled);
	void setSuffixEnabled(bool enabled);

	void setChangeCursor(bool change);

signals:
	void clicked();

protected:
	void enterEvent(QEnterEvent *) override;
	void leaveEvent(QEvent *) override;
	void mouseReleaseEvent(QMouseEvent *) override;
	void keyReleaseEvent(QKeyEvent *) override;
	bool hasSubtitle() const { return descLbl != nullptr; }

	void focusInEvent(QFocusEvent *e) override
	{
		OBSWidgetUtils::showKeyFocused(e);
		QFrame::focusInEvent(e);
	}
	void focusOutEvent(QFocusEvent *e) override
	{
		OBSWidgetUtils::hideKeyFocused(e);
		QFrame::focusOutEvent(e);
	}

private:
	QGridLayout *layout;

	QLabel *nameLbl = nullptr;
	QLabel *descLbl = nullptr;

	QWidget *_prefix = nullptr;
	QWidget *_suffix = nullptr;

	void autoConnectWidget(QWidget *w);
	bool changeCursor = false;
};

/**
* Collapsible row expand button
*/
class OBSActionCollapseButton : public QAbstractButton {
	Q_OBJECT

public:
	OBSActionCollapseButton(QWidget *parent = nullptr);

private:
	QPixmap extendDown;
	QPixmap extendUp;

protected:
	void paintEvent(QPaintEvent *) override;
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

protected:
	void focusInEvent(QFocusEvent *e) override
	{
		OBSWidgetUtils::showKeyFocused(e);
		QFrame::focusInEvent(e);
	}
	void focusOutEvent(QFocusEvent *e) override
	{
		OBSWidgetUtils::hideKeyFocused(e);
		QFrame::focusOutEvent(e);
	}

private:
	void toggleVisibility();

	QPixmap extendDown;
	QPixmap extendUp;

	QVBoxLayout *layout;

	OBSActionRow *ar;
	OBSActionCollapseButton *collapseBtn;
	OBSToggleSwitch *sw = nullptr;
	OBSPropertiesList *plist;
};
