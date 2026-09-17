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

#include <QObject>
#include <QPointer>

#include <vector>
#include <mutex>

namespace OBS {
class HealthCheckItem;
enum class HealthStatus;

class HealthCheckService : public QObject {
	Q_OBJECT

public:
	HealthCheckService(QObject *parent);
	~HealthCheckService() = default;

	HealthCheckService(const HealthCheckService &) = delete;
	HealthCheckService &operator=(const HealthCheckService &) = delete;

	std::vector<QPointer<HealthCheckItem>> getInvalidItems();
	int getInvalidCount() { return totalInvalidCount; }

	HealthStatus getGlobalStatus();

	// Processes all item entries to determine the current 'worst' status amongst them.
	// The result is emitted via the `globalStatusChanged()` signal.
	void refreshGlobalStatus();

	// Creates a new entry in the health check service.
	// Title cannot be changed after creation and should be a general name for the issue being tracked.
	// Items will be shown in the health check dialog whenever their status is not `Valid`.
	HealthCheckItem *createItem(QObject *parent, QString id, QString title);

private:
	int totalInvalidCount{0};
	HealthStatus globalStatus;

	// Registers an item with the service and sets up the relevant event slots.
	// Emits the `itemListChanged()` signal
	void registerItem(HealthCheckItem *item);

	// Removes an item from the service.
	// Emits the `itemListChanged()` signal
	void unregisterItem(const QString &id);

	std::unordered_map<QString, QPointer<HealthCheckItem>> registry;

signals:
	void globalStatusChanged(HealthStatus status);
	void itemListChanged();
};
} // namespace OBS
