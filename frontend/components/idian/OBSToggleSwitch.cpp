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

#include "OBSToggleSwitch.hpp"
#include <util/base.h>

#define UNUSED_PARAMETER(param) (void)param

static QColor blendColors(const QColor &color1, const QColor &color2, float ratio)
{
	int r = color1.red() * (1 - ratio) + color2.red() * ratio;
	int g = color1.green() * (1 - ratio) + color2.green() * ratio;
	int b = color1.blue() * (1 - ratio) + color2.blue() * ratio;

	return QColor(r, g, b, 255);
}

OBSToggleSwitch::OBSToggleSwitch(QWidget *parent)
	: QAbstractButton(parent),
	  animHandle(new QPropertyAnimation(this, "xpos", this)),
	  animBgColor(new QPropertyAnimation(this, "blend", this)),
	  OBSIdianUtils(this)
{
	offPos = rect().width() / 2 - 18;
	onPos = rect().width() / 2 + 18;
	xPos = offPos;
	margin = 3;

	setCheckable(true);
	setAccessibleName("ToggleSwitch");

	installEventFilter(this);

	connect(this, &OBSToggleSwitch::clicked, this, &OBSToggleSwitch::onClicked);

	connect(animHandle, &QVariantAnimation::valueChanged, this, &OBSToggleSwitch::updateBackgroundColor);
	connect(animBgColor, &QVariantAnimation::valueChanged, this, &OBSToggleSwitch::updateBackgroundColor);
}

OBSToggleSwitch::OBSToggleSwitch(bool defaultState, QWidget *parent) : OBSToggleSwitch(parent)
{
	setChecked(defaultState);
	if (defaultState) {
		xPos = onPos;
	}
}

void OBSToggleSwitch::animateHandlePosition()
{
	animHandle->setStartValue(xPos);

	int endPos = onPos;

	if ((!isDelayed() && !isChecked()) || (isDelayed() && !pendingStatus))
		endPos = offPos;

	animHandle->setEndValue(endPos);

	animHandle->setDuration(120);
	animHandle->start();
}

void OBSToggleSwitch::updateBackgroundColor()
{
	QColor offColor = underMouse() ? backgroundInactiveHover : backgroundInactive;
	QColor onColor = underMouse() ? backgroundActiveHover : backgroundActive;

	if (!isDelayed()) {
		int offset = isChecked() ? 0 : offPos;
		blend = (float)(xPos - offset) / (float)(onPos);
	}

	QColor bg = blendColors(offColor, onColor, blend);

	if (!isEnabled())
		bg = backgroundInactive;

	setStyleSheet("background: " + bg.name());
}

void OBSToggleSwitch::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::EnabledChange) {
		OBSIdianUtils::toggleClass("disabled", !isEnabled());
		updateBackgroundColor();
	}
}

void OBSToggleSwitch::paintEvent(QPaintEvent *e)
{
	UNUSED_PARAMETER(e);

	QStyleOptionButton opt;
	opt.initFrom(this);
	QPainter p(this);

	bool showChecked = isChecked();
	if (isDelayed()) {
		showChecked = pendingStatus;
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

	QAbstractButton::showEvent(e);
}

void OBSToggleSwitch::click()
{
	if (!isDelayed())
		QAbstractButton::click();

	if (isChecked() == pendingStatus)
		setPending(!isChecked());
}

void OBSToggleSwitch::onClicked(bool checked)
{
	if (delayed)
		return;

	setPending(checked);
}

void OBSToggleSwitch::setStatus(bool status)
{
	if (status == isChecked() && status == pendingStatus)
		return;

	pendingStatus = status;
	setChecked(status);

	if (isChecked()) {
		animBgColor->setStartValue(0.0f);
		animBgColor->setEndValue(1.0f);
	} else {
		animBgColor->setStartValue(1.0f);
		animBgColor->setEndValue(0.0f);
	}

	animBgColor->setEasingCurve(QEasingCurve::InOutCubic);
	animBgColor->setDuration(240);
	animBgColor->start();
}

void OBSToggleSwitch::setPending(bool pending)
{
	pendingStatus = pending;
	animateHandlePosition();

	if (!isDelayed())
		return;

	if (pending) {
		emit pendingChecked();
	} else {
		emit pendingUnchecked();
	}
}

void OBSToggleSwitch::setDelayed(bool state)
{
	delayed = state;
	pendingStatus = isChecked();
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

void OBSToggleSwitch::keyReleaseEvent(QKeyEvent *e)
{
	if (!isDelayed()) {
		QAbstractButton::keyReleaseEvent(e);
		return;
	}

	if (e->key() != Qt::Key_Space) {
		return;
	}

	click();
}

void OBSToggleSwitch::mouseReleaseEvent(QMouseEvent *e)
{
	if (!isDelayed()) {
		QAbstractButton::mouseReleaseEvent(e);
		return;
	}

	if (e->button() != Qt::LeftButton) {
		return;
	}

	click();
}

QSize OBSToggleSwitch::sizeHint() const
{
	return QSize(2 * handleSize, handleSize);
}
