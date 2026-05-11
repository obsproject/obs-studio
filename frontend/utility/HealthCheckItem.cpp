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

#include "HealthCheckItem.hpp"

#include <OBSApp.hpp>

namespace OBS {
HealthCheckItem::HealthCheckItem(PassKey, QObject *parent, QString id, QString title)
	: QObject(parent),
	  id_(std::move(id)),
	  title_(std::move(title))
{
}

void HealthCheckItem::setMessage(QString message)
{
	message_ = message;

	emit statusChanged();
}

void HealthCheckItem::setStatus(HealthStatus status)
{
	status_ = status;

	emit statusChanged();
}

void HealthCheckItem::setStatus(HealthStatus status, QString message)
{
	status_ = status;
	message_ = message;

	emit statusChanged();
}

QString HealthCheckItem::statusText() const
{
	return HealthCheckItem::statusText(status());
}

QString HealthCheckItem::statusText(HealthStatus status)
{
	switch (status) {
	case HealthStatus::Valid:
		return QTStr("HealthCheck.Status.Valid");
	case HealthStatus::Warning:
		return QTStr("HealthCheck.Status.Warning");
	case HealthStatus::Critical:
		return QTStr("HealthCheck.Status.Critical");
	default:
		return QTStr("HealthCheck.Status.Invalid");
	}
}
} // namespace OBS
