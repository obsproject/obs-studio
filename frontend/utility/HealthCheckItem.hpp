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

#include <utility/HealthCheckAction.hpp>

#include <QObject>
#include <QPointer>

#include <string>
#include <functional>

namespace OBS {
enum class HealthStatus { Valid, Warning, Critical };

class HealthCheckService;
class HealthCheckAction;

// Created via HealthCheckService::createItem().
class HealthCheckItem : public QObject {
	Q_OBJECT

public:
	// Limits constructor to only HealthCheckService.
	class PassKey {
		friend class HealthCheckService;
		PassKey() = default;
	};

	HealthCheckItem(PassKey, QObject *parent, QString id, QString title);
	~HealthCheckItem() = default;

	HealthCheckItem(const HealthCheckItem &) = delete;
	HealthCheckItem &operator=(const HealthCheckItem &) = delete;

	const QString &id() const { return id_; }
	const QString &title() const { return title_; }

	void setMessage(QString message);
	const QString &message() const { return message_; }

	void setStatus(HealthStatus status);
	void setStatus(HealthStatus status, QString message);
	const HealthStatus &status() const { return status_; }
	QString statusText() const;

	static QString statusText(HealthStatus status);

	HealthCheckAction *action() { return action_; }
	HealthCheckAction *createAction()
	{
		action_ = new HealthCheckAction(HealthCheckAction::PassKey{}, this);
		return action_;
	};

private:
	QString id_{""};

	QString title_{""};
	QString message_{""};

	HealthStatus status_ = HealthStatus::Valid;

	QPointer<HealthCheckAction> action_;

signals:
	void statusChanged();
};
} // namespace OBS
