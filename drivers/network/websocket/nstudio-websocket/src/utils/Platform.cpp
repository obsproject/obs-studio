/*
obs-websocket
Copyright (C) 2016-2021 Stephane Lepin <stephane.lepin@gmail.com>
Copyright (C) 2020-2021 Kyle Manning <tt2468@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program. If not, see <https://www.gnu.org/licenses/>
*/

#include <QCoreApplication>
#include <QCommandLineParser>
#include <QNetworkInterface>
#include <QHostAddress>
#include <obs-frontend-api.h>

#include "Platform.h"
#include "plugin-macros.generated.h"

std::string Utils::Platform::GetLocalAddress()
{
	std::vector<QString> validAddresses;
	for (auto address : QNetworkInterface::allAddresses()) {
		// Exclude addresses which won't work
		if (address == QHostAddress::LocalHost)
			continue;
		else if (address == QHostAddress::LocalHostIPv6)
			continue;
		else if (address.isLoopback())
			continue;
		else if (address.isLinkLocal())
			continue;
		else if (address.isNull())
			continue;

		validAddresses.push_back(address.toString());
	}

	// Return early if no valid addresses were found
	if (validAddresses.size() == 0)
		return "0.0.0.0";

	std::vector<std::pair<QString, uint8_t>> preferredAddresses;
	for (auto address : validAddresses) {
		// Attribute a priority (0 is best) to the address to choose the best picks
		if (address.startsWith("192.168.1.") ||
		    address.startsWith("192.168.0.")) { // Prefer common consumer router network prefixes
			if (address.startsWith("192.168.56."))
				preferredAddresses.push_back(std::make_pair(address,
									    255)); // Ignore virtualbox default
			else
				preferredAddresses.push_back(std::make_pair(address, 0));
		} else if (address.startsWith("172.16.")) { // Slightly less common consumer router network prefixes
			preferredAddresses.push_back(std::make_pair(address, 1));
		} else if (address.startsWith("10.")) { // Even less common consumer router network prefixes
			preferredAddresses.push_back(std::make_pair(address, 2));
		} else { // Set all other addresses to equal priority
			preferredAddresses.push_back(std::make_pair(address, 255));
		}
	}

	// Sort by priority
	std::sort(preferredAddresses.begin(), preferredAddresses.end(),
		  [=](std::pair<QString, uint8_t> a, std::pair<QString, uint8_t> b) { return a.second < b.second; });

	// Return highest priority address
	return preferredAddresses[0].first.toStdString();
}

QString Utils::Platform::GetCommandLineArgument(QString arg)
{
	QCommandLineParser parser;
	QCommandLineOption cmdlineOption(arg, arg, arg, "");
	parser.addOption(cmdlineOption);
	parser.parse(QCoreApplication::arguments());

	if (!parser.isSet(cmdlineOption))
		return "";

	return parser.value(cmdlineOption);
}

bool Utils::Platform::GetCommandLineFlagSet(QString arg)
{
	QCommandLineParser parser;
	QCommandLineOption cmdlineOption(arg, arg, arg, "");
	parser.addOption(cmdlineOption);
	parser.parse(QCoreApplication::arguments());

	return parser.isSet(cmdlineOption);
}

struct SystemTrayNotification {
	QSystemTrayIcon::MessageIcon icon;
	QString title;
	QString body;
};

void Utils::Platform::SendTrayNotification(QSystemTrayIcon::MessageIcon icon, QString title, QString body)
{
	if (!QSystemTrayIcon::isSystemTrayAvailable() || !QSystemTrayIcon::supportsMessages())
		return;

	SystemTrayNotification *notification = new SystemTrayNotification{icon, title, body};

	obs_queue_task(
		OBS_TASK_UI,
		[](void *param) {
			auto notification = static_cast<SystemTrayNotification *>(param);
			void *systemTrayPtr = obs_frontend_get_system_tray();
			if (systemTrayPtr) {
				auto systemTray = static_cast<QSystemTrayIcon *>(systemTrayPtr);
				systemTray->showMessage(notification->title, notification->body, notification->icon);
			}
			delete notification;
		},
		(void *)notification, false);
}
