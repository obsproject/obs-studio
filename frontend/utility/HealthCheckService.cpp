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

#include "HealthCheckService.hpp"

#include <OBSApp.hpp>
#include <dialogs/HealthCheckDialog.hpp>

namespace OBS {
HealthCheckService::HealthCheckService(QObject *parent) : QObject(parent) {}

void HealthCheckService::registerItem(HealthCheckItem *item)
{
	if (!item) {
		return;
	}

	QString id = item->id();
	registry[id] = QPointer<HealthCheckItem>(item);

	connect(item, &QObject::destroyed, this, [this, id]() { unregisterItem(id); });
	connect(item, &HealthCheckItem::statusChanged, this, [this, item]() {
		if (item->status() != getGlobalStatus()) {
			refreshGlobalStatus();
		}
	});

	emit itemListChanged();
}

void HealthCheckService::unregisterItem(const QString &id)
{
	registry.erase(id);
	refreshGlobalStatus();

	emit itemListChanged();
}

std::vector<QPointer<HealthCheckItem>> HealthCheckService::getInvalidItems()
{
	std::vector<QPointer<HealthCheckItem>> activeIssues;

	for (const auto &[id, item] : registry) {
		if (item.isNull()) {
			continue;
		}

		if (item->status() != HealthStatus::Valid) {
			activeIssues.emplace_back(item);
		}
	}

	return activeIssues;
}

HealthStatus HealthCheckService::getGlobalStatus()
{
	return globalStatus;
}

void HealthCheckService::refreshGlobalStatus()
{
	totalInvalidCount = 0;
	HealthStatus globalStatus = HealthStatus::Valid;

	auto it = registry.begin();
	while (it != registry.end()) {
		if (it->second.isNull()) {
			it = registry.erase(it);
			continue;
		}

		HealthStatus itemStatus = it->second->status();

		if (itemStatus != HealthStatus::Valid) {
			++totalInvalidCount;
		}

		switch (globalStatus) {
		case HealthStatus::Warning:
			if (itemStatus == HealthStatus::Critical) {
				globalStatus = itemStatus;
			}
			break;
		case HealthStatus::Valid:
			globalStatus = itemStatus;
			break;
		case HealthStatus::Critical:
		default:
			break;
		}

		++it;
	}

	this->globalStatus = globalStatus;
	emit globalStatusChanged(globalStatus);
}

HealthCheckItem *HealthCheckService::createItem(QObject *parent, QString id, QString title)
{
	auto *item = new HealthCheckItem(HealthCheckItem::PassKey{}, parent, id, title);
	registerItem(item);

	return item;
}
} // namespace OBS
