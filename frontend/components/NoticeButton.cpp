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

#include "NoticeButton.hpp"

#include <Idian/Utils.hpp>

namespace OBS {
NoticeButton::NoticeButton(QWidget *parent) : QPushButton(parent), idian::Utils(this)
{
	setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
	idian::Utils::addClass("text-bold");
}

NoticeButton::NoticeButton(QWidget *parent, QString text, OBS::NoticeStyle style) : NoticeButton(parent)
{
	setText(text);
	setStyle(style);
}

void NoticeButton::setStyle(NoticeStyle style)
{
	applyStyle(currentStyle, false);
	applyStyle(style, true);

	currentStyle = style;
}

void NoticeButton::applyStyle(NoticeStyle style, bool enable)
{
	switch (style) {
	case NoticeStyle::Primary:
		idian::Utils::toggleClass("primary", enable);
		break;
	case NoticeStyle::Secondary:
		idian::Utils::toggleClass("secondary", enable);
		break;
	case NoticeStyle::Info:
		idian::Utils::toggleClass("info", enable);
		break;
	case NoticeStyle::Success:
		idian::Utils::toggleClass("success", enable);
		break;
	case NoticeStyle::Warning:
		idian::Utils::toggleClass("warning", enable);
		break;
	case NoticeStyle::Danger:
		idian::Utils::toggleClass("danger", enable);
		break;
	default:
		break;
	}
}
} // namespace OBS
