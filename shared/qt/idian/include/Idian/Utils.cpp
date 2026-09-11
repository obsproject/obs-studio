/******************************************************************************
    Copyright (C) 2025 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <Idian/Utils.hpp>

#include <Idian/StateEventFilter.hpp>

#include <QAbstractButton>
#include <QLabel>
#include <QPainter>
#include <QStyleOptionButton>
#include <QStyleOptionFrame>

namespace idian {
void Utils::polishChildren(QWidget *widget)
{
	for (QWidget *child : widget->findChildren<QWidget *>()) {
		repolish(child);
	}
}

void Utils::repolish(QWidget *widget)
{
	widget->style()->polish(widget);
}

void Utils::addClass(QWidget *widget, const QString &classname)
{
	if (!classNameIsValid(classname)) {
		return;
	}

	QVariant current = widget->property("class");

	QStringList classList = current.toString().split(" ");
	if (classList.contains(classname)) {
		return;
	}

	classList.removeDuplicates();
	classList.removeAll("");
	classList.append(classname);

	QString newClasses = classList.isEmpty() ? "" : classList.join(" ");
	widget->setProperty("class", newClasses);

	repolish(widget);
}

void Utils::removeClass(QWidget *widget, const QString &classname)
{
	if (!classNameIsValid(classname)) {
		return;
	}

	QVariant current = widget->property("class");
	if (current.isNull()) {
		return;
	}

	QStringList classList = current.toString().split(" ");
	if (!classList.contains(classname, Qt::CaseSensitive)) {
		return;
	}

	classList.removeDuplicates();
	classList.removeAll("");
	classList.removeAll(classname);

	QString newClasses = classList.isEmpty() ? "" : classList.join(" ");
	widget->setProperty("class", newClasses);

	repolish(widget);
}

void Utils::toggleClass(QWidget *widget, const QString &classname, bool toggle)
{
	if (toggle) {
		addClass(widget, classname);
	} else {
		removeClass(widget, classname);
	}
}

void Utils::applyColorToIcon(QAbstractButton *button)
{
	if (button && !button->icon().isNull()) {
		// Filter is on a widget with an icon set, update its colors
		QStyleOptionButton opt;
		opt.initFrom(button);

		QColor color = opt.palette.color(QPalette::ButtonText);
		QPixmap tinted = recolorPixmap(button->icon().pixmap(button->iconSize(), QIcon::Normal), color);
		QIcon tintedIcon;
		tintedIcon.addPixmap(tinted, QIcon::Normal);
		tintedIcon.addPixmap(tinted, QIcon::Disabled);

		button->setIcon(tintedIcon);
	}
}

void Utils::applyColorToIcon(QLabel *label)
{
	if (label && !label->pixmap().isNull()) {
		QStyleOptionFrame opt;
		opt.initFrom(label);

		QColor color = opt.palette.color(QPalette::Text);
		QPixmap tinted = recolorPixmap(label->pixmap(), color);

		label->setPixmap(tinted);
	}
}

QPixmap Utils::recolorPixmap(const QPixmap &src, const QColor &color)
{
	QImage img = src.toImage();
	QPainter p(&img);
	p.setCompositionMode(QPainter::CompositionMode_SourceIn);
	p.fillRect(img.rect(), color);
	p.end();
	return QPixmap::fromImage(img);
}

// Updates the dynamic property 'class' on a widget with values in response to certain interaction events.
// Ex. `hover` when the widget is hovered.
// Widgets can then be styled via CSS class-style rules like .hover.
void Utils::applyStateStylingEventFilter(QWidget *widget)
{
	widget->installEventFilter(new StateEventFilter(widget));
}
} // namespace idian
