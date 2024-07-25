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
#include <util/base.h>

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
	  animation(new QPropertyAnimation(this, "xpos", this)),
	  blendAnimation(new QPropertyAnimation(this, "blend", this))
{
	offPos = rect().width() / 2 - 18;
	onPos = rect().width() / 2 + 18;
	xPos = offPos;
	margin = 3;

	setCheckable(true);
	setAccessibleName("ToggleSwitch");

	connect(this, &OBSToggleSwitch::clicked, this,
		&OBSToggleSwitch::onClicked);

	connect(animation, &QVariantAnimation::valueChanged, this,
		&OBSToggleSwitch::updateBackgroundColor);
	connect(blendAnimation, &QVariantAnimation::valueChanged, this,
		&OBSToggleSwitch::updateBackgroundColor);
}

OBSToggleSwitch::OBSToggleSwitch(bool defaultState, QWidget *parent)
	: OBSToggleSwitch(parent)
{
	setChecked(defaultState);
	manualStatus = defaultState;
	if (defaultState)
		xPos = onPos;
}

void OBSToggleSwitch::updateBackgroundColor()
{
	QColor offColor = underMouse() ? backgroundInactiveHover
				       : backgroundInactive;
	QColor onColor = underMouse() ? backgroundActiveHover
				      : backgroundActive;

	if (!manualStatusChange) {
		int offset = isChecked() ? 0 : offPos;
		blend = (float)(xPos - offset) / (float)(onPos);
	}

	QColor bg = blendColors(offColor, onColor, blend);
	setStyleSheet("background: " + bg.name());
}

void OBSToggleSwitch::paintEvent(QPaintEvent *e)
{
	UNUSED_PARAMETER(e);

	QStyleOptionButton opt;
	opt.initFrom(this);
	QPainter p(this);

	bool showChecked = isChecked();
	if (waiting) {
		showChecked = !showChecked;
	}

	opt.state.setFlag(QStyle::State_On, showChecked);
	opt.state.setFlag(QStyle::State_Off, !showChecked);

	opt.state.setFlag(QStyle::State_Sunken, true);

	style()->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, &p, this);

	p.setRenderHint(QPainter::Antialiasing, true);

	p.setBrush(handleColor);
	p.drawEllipse(QRectF(xPos, margin, handleSize, handleSize));
}

void OBSToggleSwitch::showEvent(QShowEvent *e)
{
	margin = (rect().height() - handleSize) / 2;

	offPos = margin;
	onPos = rect().width() - handleSize - margin;

	xPos = isChecked() ? onPos : offPos;

	updateBackgroundColor();
	style()->polish(this);
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
	updateBackgroundColor();
	QAbstractButton::enterEvent(e);
}

void OBSToggleSwitch::leaveEvent(QEvent *e)
{
	updateBackgroundColor();
	QAbstractButton::leaveEvent(e);
}

QSize OBSToggleSwitch::sizeHint() const
{
	return QSize(2 * handleSize, handleSize);
}
