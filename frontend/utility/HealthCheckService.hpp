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

#include <utility/HealthCheckItem.hpp>

#include <QObject>
#include <QPointer>

#include <vector>
#include <mutex>

class HealthCheckDialog;

namespace OBS {
class HealthCheckService : public QObject {
	Q_OBJECT

public:
	HealthCheckService(QObject *parent);
	~HealthCheckService() = default;

	HealthCheckService(const HealthCheckService &) = delete;
	HealthCheckService &operator=(const HealthCheckService &) = delete;

	void registerItem(HealthCheckItem *item);
	void unregisterItem(const QString &id);

	std::vector<QPointer<HealthCheckItem>> getInvalidItems();
	int getInvalidCount() { return totalInvalidCount; }

	HealthStatus getGlobalStatus();

	void refreshGlobalStatus();

	HealthCheckItem *createItem(QObject *parent, QString id, QString title);

private:
	int totalInvalidCount;
	HealthStatus globalStatus{HealthStatus::Valid};

	std::unordered_map<QString, QPointer<HealthCheckItem>> registry;

signals:
	void globalStatusChanged(HealthStatus status);
	void itemListChanged();
};
} // namespace OBS
