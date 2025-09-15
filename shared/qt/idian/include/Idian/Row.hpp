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

#include <Idian/ComboBox.hpp>
#include <Idian/DoubleSpinBox.hpp>
#include <Idian/PropertiesList.hpp>
#include <Idian/SpinBox.hpp>
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

// Base class mostly so adding stuff to a list is easier
class GenericRow : public QFrame, public Utils {
	Q_OBJECT

public:
	GenericRow(QWidget *parent = nullptr) : QFrame(parent), Utils(this) { setAccessibleName(""); };

	virtual void setTitle(const QString &title) = 0;
	virtual void setDescription(const QString &description) = 0;
};

// Row widget containing one or more controls
class Row : public GenericRow {
	Q_OBJECT

public:
	Row(QWidget *parent = nullptr);

	void setPrefix(QWidget *w, bool autoConnect = true);
	void setSuffix(QWidget *w, bool autoConnect = true);

	bool hasPrefix() { return prefix_; }
	bool hasSuffix() { return suffix_; }

	QWidget *prefix() const { return prefix_; }
	QWidget *suffix() const { return suffix_; }

	void setPrefixEnabled(bool enabled);
	void setSuffixEnabled(bool enabled);

	virtual void setTitle(const QString &title) override;
	virtual void setDescription(const QString &description) override;

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
		Utils::showKeyFocused(event);
		QFrame::focusInEvent(event);
	}

	void focusOutEvent(QFocusEvent *event) override
	{
		Utils::hideKeyFocused(event);
		QFrame::focusOutEvent(event);
	}

private:
	QGridLayout *layout;

	QVBoxLayout *labelLayout = nullptr;

	QLabel *nameLabel = nullptr;
	QLabel *descriptionLabel = nullptr;

	QWidget *prefix_ = nullptr;
	QWidget *suffix_ = nullptr;

	QWidget *buddyWidget = nullptr;

	void connectBuddyWidget(QWidget *widget);
	bool changeCursor = false;
};

// Collapsible row expand button
class ExpandButton : public QAbstractButton, public Utils {
	Q_OBJECT

private:
	QPixmap extendDown;
	QPixmap extendUp;

	friend class CollapsibleRow;

protected:
	explicit ExpandButton(QWidget *parent = nullptr);

	void paintEvent(QPaintEvent *) override;

	void focusInEvent(QFocusEvent *event) override
	{
		Utils::showKeyFocused(event);
		QAbstractButton::focusInEvent(event);
	}

	void focusOutEvent(QFocusEvent *event) override
	{
		Utils::hideKeyFocused(event);
		QAbstractButton::focusOutEvent(event);
	}
};

class RowFrame : protected QFrame, protected Utils {
	Q_OBJECT

signals:
	void clicked();

protected:
	explicit RowFrame(QWidget *parent = nullptr);

	void enterEvent(QEnterEvent *) override;
	void leaveEvent(QEvent *) override;

	void focusInEvent(QFocusEvent *event) override
	{
		Utils::showKeyFocused(event);
		QWidget::focusInEvent(event);
	}

	void focusOutEvent(QFocusEvent *event) override
	{
		Utils::hideKeyFocused(event);
		QWidget::focusOutEvent(event);
	}

private:
	friend class CollapsibleRow;
};

// Collapsible Generic OBS property container
class CollapsibleRow : public GenericRow {
	Q_OBJECT

public:
	CollapsibleRow(QWidget *parent = nullptr);

	void setCheckable(bool check);
	bool isCheckable() { return checkable; }

	void setChecked(bool checked);
	bool isChecked() { return toggleSwitch->isChecked(); };

	virtual void setTitle(const QString &title) override;
	virtual void setDescription(const QString &description) override;

	void addRow(GenericRow *actionRow);

signals:
	void toggled(bool checked);

private:
	void toggleVisibility();

	QPixmap extendDown;
	QPixmap extendUp;

	QVBoxLayout *layout;
	RowFrame *rowWidget;
	QHBoxLayout *rowLayout;

	Row *actionRow;
	QFrame *expandFrame;
	QHBoxLayout *btnLayout;
	ExpandButton *expandButton;
	PropertiesList *propertyList;

	ToggleSwitch *toggleSwitch = nullptr;
	bool checkable = false;
};

} // namespace idian
