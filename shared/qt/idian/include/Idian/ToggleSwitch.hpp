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

#include <Idian/Utils.hpp>

#include <QAbstractButton>
#include <QAccessibleWidget>
#include <QBrush>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOptionButton>
#include <QWidget>

namespace idian {

class ToggleSwitch : public QAbstractButton, public Utils {
	Q_OBJECT
	Q_PROPERTY(int xpos MEMBER xPos WRITE setPos)
	Q_PROPERTY(QColor background MEMBER backgroundInactive DESIGNABLE true)
	Q_PROPERTY(QColor background_hover MEMBER backgroundInactiveHover DESIGNABLE true)
	Q_PROPERTY(QColor background_checked MEMBER backgroundActive DESIGNABLE true)
	Q_PROPERTY(QColor background_checked_hover MEMBER backgroundActiveHover DESIGNABLE true)
	Q_PROPERTY(QColor handleColor MEMBER handleColor DESIGNABLE true)
	Q_PROPERTY(int handleSize MEMBER handleSize DESIGNABLE true)
	Q_PROPERTY(float blend MEMBER blend WRITE setBlend DESIGNABLE false)

public:
	ToggleSwitch(QWidget *parent = nullptr);
	ToggleSwitch(bool defaultState, QWidget *parent = nullptr);

	QSize sizeHint() const override;

	void setPos(int x)
	{
		xPos = x;
		update();
	}

	void setBlend(float newBlend)
	{
		blend = newBlend;
		update();
	}

	void setDelayed(bool state);
	bool isDelayed() { return delayed; }

	void setStatus(bool status);
	void setPending(bool pending);

public slots:
	void click();

signals:
	void pendingChecked();
	void pendingUnchecked();

protected:
	void changeEvent(QEvent *event) override;
	void paintEvent(QPaintEvent *) override;
	void showEvent(QShowEvent *) override;
	void enterEvent(QEnterEvent *) override;
	void leaveEvent(QEvent *) override;
	void keyReleaseEvent(QKeyEvent *) override;
	void mouseReleaseEvent(QMouseEvent *) override;

	void focusInEvent(QFocusEvent *e) override
	{
		Utils::showKeyFocused(e);
		QAbstractButton::focusInEvent(e);
	}

	void focusOutEvent(QFocusEvent *e) override
	{
		Utils::hideKeyFocused(e);
		QAbstractButton::focusOutEvent(e);
	}

private slots:
	void onClicked(bool checked);

private:
	int xPos;
	int onPos;
	int offPos;
	int margin;

	float blend = 0.0f;

	bool delayed = false;
	bool pendingStatus = false;

	void animateHandlePosition();

	void updateBackgroundColor();
	QColor backgroundInactive;
	QColor backgroundInactiveHover;
	QColor backgroundActive;
	QColor backgroundActiveHover;
	QColor handleColor;
	int handleSize = 18;

	QPropertyAnimation *animHandle = nullptr;
	QPropertyAnimation *animBgColor = nullptr;
};

} // namespace idian
