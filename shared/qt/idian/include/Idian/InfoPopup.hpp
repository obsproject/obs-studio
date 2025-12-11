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

#pragma once

#include <Idian/AbstractPopup.hpp>
#include <Idian/Utils.hpp>

#include <QPushButton>
#include <QLabel>

namespace idian {

class InfoPopup : public idian::AbstractPopup, public idian::Utils {
	Q_OBJECT

	QFrame *guideWidget = nullptr;
	QVBoxLayout *guideLayout = nullptr;

	QWidget *header = nullptr;
	QHBoxLayout *headerLayout = nullptr;

	QPushButton *dismissButton = nullptr;
	QLabel *title = nullptr;
	QLabel *info = nullptr;

public:
	InfoPopup(QWidget *parent);
	~InfoPopup();

	void setTitle(QString text);
	void setInfo(QString text);

public slots:
	void dismiss();

signals:
	void rejected();
	void accepted();
};
} // namespace idian
