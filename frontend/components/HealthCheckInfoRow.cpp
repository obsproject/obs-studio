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

#include "HealthCheckInfoRow.hpp"

#include <utility/HealthCheckItem.hpp>

#include <Idian/RowInfo.hpp>

#include <QPushButton>

HealthCheckInfoRow::HealthCheckInfoRow(QWidget *parent, OBS::HealthCheckItem *item) : idian::Row(parent)
{
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	auto *rowInfo = new idian::RowInfo(this, "", item->message());
	addWidget(rowInfo);

	rowInfo->description()->setWordWrap(true);
	rowInfo->description()->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

	if (item->action() == nullptr) {
		return;
	}

	QPushButton *healthActionButton = new QPushButton(this);
	healthActionButton->setText(item->action()->text());

	addWidget(healthActionButton);
	connect(healthActionButton, &QPushButton::pressed, item->action(), &OBS::HealthCheckAction::trigger);
}
