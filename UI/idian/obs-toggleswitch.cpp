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

#include "obs-toggleswitch.hpp"

#define UNUSED_PARAMETER(param) (void)param

static QColor blendColors(const QColor &color1, const QColor &color2,
			  float ratio)
{
	int r = color1.red() * (1 - ratio) + color2.red() * ratio;
	int g = color1.green() * (1 - ratio) + color2.green() * ratio;
	int b = color1.blue() * (1 - ratio) + color2.blue() * ratio;

	return QColor(r, g, b, 255);
}

OBSToggleSwitch::OBSToggleSwitch(QWidget *parent)
	: QAbstractButton(parent),
	  targetHeight(24),
	  margin(6),
	  animation(new QPropertyAnimation(this, "xpos", this)),
	  blendAnimation(new QPropertyAnimation(this, "blend", this))
{
	offPos = targetHeight / 2;
	onPos = 2 * targetHeight - targetHeight / 2;
	xPos = offPos;

	setCheckable(true);
	setFocusPolicy(Qt::TabFocus);

	connect(this, &OBSToggleSwitch::clicked, this,
		&OBSToggleSwitch::onClicked);
}

OBSToggleSwitch::OBSToggleSwitch(bool defaultState, QWidget *parent)
	: OBSToggleSwitch(parent)
{
	setChecked(defaultState);
	manualStatus = defaultState;
	if (defaultState)
		xPos = onPos;
}

void OBSToggleSwitch::paintEvent(QPaintEvent *e)
{
	UNUSED_PARAMETER(e);

	QPainter p(this);
	p.setPen(Qt::NoPen);

	p.setOpacity((isEnabled() || waiting) ? 1.0f : 0.5f);
	if (!manualStatusChange) {
		int offset = isChecked() ? 0 : offPos;
		blend = (float)(xPos - offset) / (float)(onPos);
	}

	p.setBrush(blendColors(backgroundInactive, backgroundActive, blend));
	p.setRenderHint(QPainter::Antialiasing, true);
	p.drawRoundedRect(QRect(0, 0, 2 * targetHeight, targetHeight),
			  targetHeight / 2, targetHeight / 2);

	p.setBrush(handle);
	p.drawEllipse(QRectF(xPos - (targetHeight / 2 - margin / 2), margin / 2,
			     targetHeight - margin, targetHeight - margin));
}

void OBSToggleSwitch::onClicked(bool checked)
{
	if (manualStatusChange) {
		// ToDo: figure out a prettier way of accomplishing this
		waiting = true;
		setEnabled(false);
	}

	if (checked) {
		animation->setStartValue(xPos);
		animation->setEndValue(onPos);
	} else {
		animation->setStartValue(xPos);
		animation->setEndValue(offPos);
	}

	animation->setDuration(120);
	animation->start();
}

void OBSToggleSwitch::setStatus(bool status)
{
	if (!manualStatusChange)
		return;
	if (status == manualStatus)
		return;

	setEnabled(true);
	waiting = false;
	manualStatus = status;

	if (manualStatus) {
		blendAnimation->setStartValue(0.0f);
		blendAnimation->setEndValue(1.0f);
	} else {
		blendAnimation->setStartValue(1.0f);
		blendAnimation->setEndValue(0.0f);
	}

	blendAnimation->setEasingCurve(QEasingCurve::InOutCubic);
	blendAnimation->setDuration(240);
	blendAnimation->start();
}

void OBSToggleSwitch::enterEvent(QEnterEvent *e)
{
	setCursor(Qt::PointingHandCursor);
	QAbstractButton::enterEvent(e);
}

QSize OBSToggleSwitch::sizeHint() const
{
	return QSize(2 * targetHeight, targetHeight);
}
