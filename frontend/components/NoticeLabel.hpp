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

#pragma once

#include <Idian/Utils.hpp>

#include <QLabel>

namespace OBS {
enum class NoticeStyle { None, Primary, Secondary, Info, Success, Warning, Danger };

class NoticeLabel : public QLabel, public idian::Utils {
	Q_OBJECT

public:
	NoticeLabel(QWidget *parent);
	NoticeLabel(QWidget *parent, NoticeStyle style);
	~NoticeLabel() = default;

	void setStyle(NoticeStyle style);
	void applyStyle(NoticeStyle style, bool enable);

protected:
	NoticeStyle currentStyle = NoticeStyle::None;
};
} // namespace OBS
