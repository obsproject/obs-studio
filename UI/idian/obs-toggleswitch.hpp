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
#include <QEvent>
#include <QBrush>
#include <QPropertyAnimation>
#include <QPainter>
#include <QMouseEvent>
#include <QAbstractButton>

class OBSToggleSwitch : public QAbstractButton {
	Q_OBJECT
	Q_PROPERTY(int xpos MEMBER xPos WRITE setPos)
	Q_PROPERTY(QColor backgroundInactive MEMBER backgroundInactive
			   DESIGNABLE true)
	Q_PROPERTY(
		QColor backgroundActive MEMBER backgroundActive DESIGNABLE true)
	Q_PROPERTY(QColor handle MEMBER handle DESIGNABLE true)
	Q_PROPERTY(int margin MEMBER margin DESIGNABLE true)
	Q_PROPERTY(int height MEMBER targetHeight DESIGNABLE true)
	Q_PROPERTY(float blend MEMBER blend WRITE setBlend DESIGNABLE false)

public:
	OBSToggleSwitch(QWidget *parent = nullptr);
	OBSToggleSwitch(bool defaultState, QWidget *parent = nullptr);

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

	void setManualStatusChange(bool manual) { manualStatusChange = manual; }
	void setStatus(bool status);

protected:
	void paintEvent(QPaintEvent *) override;

	void enterEvent(QEnterEvent *) override;

private slots:
	void onClicked(bool checked);

private:
	int xPos;
	int onPos;
	int offPos;
	int margin;
	int targetHeight;

	float blend = 0.0f;

	bool waiting = false;
	bool manualStatus = false;
	bool manualStatusChange = false;

	QColor backgroundInactive;
	QColor backgroundActive;
	QColor handle;

	QPropertyAnimation *animation = nullptr;
	QPropertyAnimation *blendAnimation = nullptr;
};
