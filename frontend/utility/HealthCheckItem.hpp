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

// Represents a single diagnostic entry within the health check system.
// An item should have it's status and description updated whenever there are changes related to this entry.
// If the issue can be resolved automatically or the user can be shown where to resolve it, set up an action via
// `createAction()` to be shown in the UI.
class HealthCheckItem : public QObject {
	Q_OBJECT

public:
	// Limits constructor to only HealthCheckService.
	class PassKey {
		friend class HealthCheckService;
		PassKey() = default;
	};

	// This class must be instantiated through `HealthCheckService::createItem()`.
	HealthCheckItem(PassKey, QObject *parent, QString id, QString title);
	~HealthCheckItem() = default;

	HealthCheckItem(const HealthCheckItem &) = delete;
	HealthCheckItem(const HealthCheckItem &&) = delete;
	HealthCheckItem &operator=(const HealthCheckItem &) = delete;
	HealthCheckItem &operator=(HealthCheckItem &&) = delete;

	const QString &id() const { return id_; }
	const QString &title() const { return title_; }

	// Updates the description for this item when shown in the UI.
	// Emits the `statusChanged()` signal.
	void setMessage(QString message);
	const QString &message() const { return message_; }

	// Updates the status of the item.
	// Emits the `statusChanged()` signal.
	void setStatus(HealthStatus status);

	// Updates the status of the item as well as the description.
	// Emits the `statusChanged()` signal.
	void setStatus(HealthStatus status, QString message);

	const HealthStatus &status() const { return status_; }

	// Returns a localized string representing this items current `status`.
	QString statusText() const;

	// Creates and associates a new action with this health check item.
	// The action should be configured by calling `setText()` and `setCallback()`.
	HealthCheckAction *createAction()
	{
		if (action()) {
			assert("HealthCheckItem: Tried to create an action for an item that already has one.");
			return action_;
		}

		action_ = new HealthCheckAction(HealthCheckAction::PassKey{}, this);
		return action_;
	};
	HealthCheckAction *action() { return action_; }

private:
	QString id_{""};

	QString title_{""};
	QString message_{""};

	HealthStatus status_ = HealthStatus::Valid;
	static QString statusText(HealthStatus status);

	QPointer<HealthCheckAction> action_;

signals:
	void statusChanged();
};
} // namespace OBS
