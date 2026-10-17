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

#include "HealthCheckWidget.hpp"

#include <components/HealthCheckInfoRow.hpp>
#include <components/HealthCheckStatusLabel.hpp>
#include <utility/HealthCheckItem.hpp>

#include <Idian/RowList.hpp>

#include <QPushButton>

HealthCheckWidget::HealthCheckWidget(QWidget *parent, OBS::HealthCheckItem *item)
	: idian::CollapsibleGroup(parent),
	  healthItem(item)
{
	setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);
	setExpanded(true);

	auto *statusLabel = new HealthCheckStatusLabel(this, item);
	row()->layout()->insertWidget(0, statusLabel);

	auto *titleLabel = new QLabel(QString("%2").arg(item->title()), this);
	idian::Utils::addClass(titleLabel, "text-title");
	row()->layout()->insertWidget(1, titleLabel);

	auto *infoRow = new HealthCheckInfoRow(this, item);
	list()->addRow(infoRow);
}
