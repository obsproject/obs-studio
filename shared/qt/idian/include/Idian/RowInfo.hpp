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

#include <QWidget>

class QLabel;
class QVBoxLayout;

namespace idian {
class Utils;

class RowInfo : public QWidget {
	Q_OBJECT

public:
	RowInfo(QWidget *parent);
	RowInfo(QWidget *parent, QString title);
	RowInfo(QWidget *parent, QString title, QString description);
	~RowInfo() = default;

	void setTitle(const QString &title);
	void setDescription(const QString &description);

	void showTitle(bool visible);
	void showDescription(bool visible);

	QLabel *title() { return nameLabel; }
	QLabel *description() { return descriptionLabel; }

private:
	Utils *widgetUtils;

	QVBoxLayout *layout_ = nullptr;

	QLabel *nameLabel = nullptr;
	QLabel *descriptionLabel = nullptr;
};
} // namespace idian
