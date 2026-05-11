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

namespace OBS {
class HealthCheckItem;

// When a HealthCheckItem has an action, a button will be shown in the health check dialog.
class HealthCheckAction : public QObject {

public:
	// Limits constructor to only HealthCheckItem.
	class PassKey {
		friend class HealthCheckItem;
		PassKey() = default;
	};

	// This class must be instantiated through `HealthCheckItem::createAction()`.
	HealthCheckAction(PassKey, QObject *parent);
	~HealthCheckAction() = default;

	HealthCheckAction(const HealthCheckAction &) = delete;
	HealthCheckAction(HealthCheckAction &&) = delete;
	HealthCheckAction &operator=(const HealthCheckAction &) = delete;
	HealthCheckAction &operator=(HealthCheckAction &&) = delete;

	// Text displayed for a widget representing this action.
	void setText(QString text);
	const QString &text();

	// The function that is called when the action is triggered.
	void setCallback(std::function<void()> func);

private:
	QString text_{""};
	std::function<void()> callback = nullptr;

public slots:
	void trigger();
};
} // namespace OBS
