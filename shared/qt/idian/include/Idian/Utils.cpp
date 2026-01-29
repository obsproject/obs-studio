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
#include <QPainter>
#include <QStyleOptionButton>

namespace idian {
void Utils::applyColorToIcon(QWidget *widget)
{
	QAbstractButton *button = qobject_cast<QAbstractButton *>(widget);
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

QPixmap Utils::recolorPixmap(const QPixmap &src, const QColor &color)
{
	QImage img = src.toImage();
	QPainter p(&img);
	p.setCompositionMode(QPainter::CompositionMode_SourceIn);
	p.fillRect(img.rect(), color);
	p.end();
	return QPixmap::fromImage(img);
}

void Utils::applyStateStylingEventFilter(QWidget *widget)
{
	widget->installEventFilter(new StateEventFilter(this, widget));
}
} // namespace idian
