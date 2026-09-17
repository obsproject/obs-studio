/******************************************************************************
    Copyright (C) 2026 by Taylor Giampaolo <warchamp7@obsproject.com>

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

#include <Idian/ExpandButton.hpp>
#include <Idian/Utils.hpp>

#include <QStyleOptionButton>
#include <QPainter>

namespace idian {
ExpandButton::ExpandButton(QWidget *parent) : QAbstractButton(parent)
{
	widgetUtils = new Utils(this);
	widgetUtils->applyStateStylingEventFilter(this);

	setCheckable(true);
}

void ExpandButton::paintEvent(QPaintEvent *)
{
	QStyleOptionButton opt;
	opt.initFrom(this);
	QPainter p(this);

	bool checked = isChecked();

	opt.state.setFlag(QStyle::State_On, checked);
	opt.state.setFlag(QStyle::State_Off, !checked);

	opt.state.setFlag(QStyle::State_Sunken, checked);

	p.setRenderHint(QPainter::Antialiasing, true);
	p.setRenderHint(QPainter::SmoothPixmapTransform, true);

	style()->drawPrimitive(QStyle::PE_PanelButtonCommand, &opt, &p, this);
	style()->drawPrimitive(QStyle::PE_IndicatorCheckBox, &opt, &p, this);
}
} // namespace idian
