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

#include "OBSIdianWidget.hpp"
#include "OBSPropertiesList.hpp"
#include "OBSToggleSwitch.hpp"
#include "OBSComboBox.hpp"
#include "OBSSpinBox.hpp"
#include "OBSDoubleSpinBox.hpp"

/**
* Base class mostly so adding stuff to a list is easier
*/
class OBSActionRow : public QFrame, public OBSIdianUtils {
	Q_OBJECT

public:
	OBSActionRow(QWidget *parent = nullptr) : QFrame(parent), OBSIdianUtils(this) { setAccessibleName(""); };
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
class OBSActionRowWidget : public OBSActionRow {
	Q_OBJECT

public:
	OBSActionRowWidget(QWidget *parent = nullptr);

	void setPrefix(QWidget *w, bool autoConnect = true);
	void setSuffix(QWidget *w, bool autoConnect = true);

	bool hasPrefix() { return _prefix; }
	bool hasSuffix() { return _suffix; }

	QWidget *prefix() const { return _prefix; }
	QWidget *suffix() const { return _suffix; }

	void setPrefixEnabled(bool enabled);
	void setSuffixEnabled(bool enabled);

	void setTitle(QString name);
	void setDescription(QString description);

	void showTitle(bool visible);
	void showDescription(bool visible);

	void setBuddy(QWidget *w);

	void setChangeCursor(bool change);

signals:
	void clicked();

protected:
	void enterEvent(QEnterEvent *) override;
	void leaveEvent(QEvent *) override;
	void mouseReleaseEvent(QMouseEvent *) override;
	void keyReleaseEvent(QKeyEvent *) override;
	bool hasDescription() const { return descriptionLabel != nullptr; }

	void focusInEvent(QFocusEvent *event) override
	{
		OBSIdianUtils::showKeyFocused(event);
		QFrame::focusInEvent(event);
	}

	void focusOutEvent(QFocusEvent *event) override
	{
		OBSIdianUtils::hideKeyFocused(event);
		QFrame::focusOutEvent(event);
	}

private:
	QGridLayout *layout;

	QVBoxLayout *labelLayout = nullptr;

	QLabel *nameLabel = nullptr;
	QLabel *descriptionLabel = nullptr;

	QWidget *_prefix = nullptr;
	QWidget *_suffix = nullptr;

	QWidget *buddyWidget = nullptr;

	void connectBuddyWidget(QWidget *widget);
	bool changeCursor = false;
};

/**
* Collapsible row expand button
*/
class OBSActionRowExpandButton : public QAbstractButton, public OBSIdianUtils {
	Q_OBJECT

private:
	QPixmap extendDown;
	QPixmap extendUp;

	friend class OBSCollapsibleRowWidget;

protected:
	explicit OBSActionRowExpandButton(QWidget *parent = nullptr);

	void paintEvent(QPaintEvent *) override;

	void focusInEvent(QFocusEvent *event) override
	{
		OBSIdianUtils::showKeyFocused(event);
		QAbstractButton::focusInEvent(event);
	}

	void focusOutEvent(QFocusEvent *event) override
	{
		OBSIdianUtils::hideKeyFocused(event);
		QAbstractButton::focusOutEvent(event);
	}
};

class OBSCollapsibleRowFrame : protected QFrame, protected OBSIdianUtils {
	Q_OBJECT

signals:
	void clicked();

protected:
	explicit OBSCollapsibleRowFrame(QWidget *parent = nullptr);

	void enterEvent(QEnterEvent *) override;
	void leaveEvent(QEvent *) override;

	void focusInEvent(QFocusEvent *event) override
	{
		OBSIdianUtils::showKeyFocused(event);
		QWidget::focusInEvent(event);
	}

	void focusOutEvent(QFocusEvent *event) override
	{
		OBSIdianUtils::hideKeyFocused(event);
		QWidget::focusOutEvent(event);
	}

private:
	friend class OBSCollapsibleRowWidget;
};

/**
* Collapsible Generic OBS property container
*/
class OBSCollapsibleRowWidget : public OBSActionRow {
	Q_OBJECT

public:
	// ToDo: Figure out a better way to handle those options
	OBSCollapsibleRowWidget(const QString &name, QWidget *parent = nullptr);
	OBSCollapsibleRowWidget(const QString &name, const QString &desc = nullptr, QWidget *parent = nullptr);

	void setCheckable(bool check);
	bool isCheckable() { return checkable; }

	void addRow(OBSActionRow *actionRow);

private:
	void toggleVisibility();

	QPixmap extendDown;
	QPixmap extendUp;

	QVBoxLayout *layout;
	OBSCollapsibleRowFrame *rowWidget;
	QHBoxLayout *rowLayout;

	OBSActionRowWidget *actionRow;
	QFrame *expandFrame;
	QHBoxLayout *btnLayout;
	OBSActionRowExpandButton *expandButton;
	OBSPropertiesList *propertyList;

	OBSToggleSwitch *toggleSwitch = nullptr;
	bool checkable = false;
};
