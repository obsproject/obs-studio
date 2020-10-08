/******************************************************************************
    Copyright (C) 2013 by Hugh Bailey <obs.jim@gmail.com>
    Copyright (C) 2014 by Zachary Lund <admin@computerquip.com>

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

#include <obs-config.h>
#include "obs-app.hpp"

#include <QGuiApplication>
#include <QScreen>

#include <unistd.h>
#include <sstream>
#include <locale.h>

#include "platform.hpp"
using namespace std;

static inline bool check_path(const char *data, const char *path,
			      string &output)
{
	ostringstream str;
	str << path << data;
	output = str.str();

	printf("Attempted path: %s\n", output.c_str());

	return (access(output.c_str(), R_OK) == 0);
}

#define INSTALL_DATA_PATH OBS_INSTALL_PREFIX OBS_DATA_PATH "/obs-studio/"

bool GetDataFilePath(const char *data, string &output)
{
	char *data_path = getenv("OBS_DATA_PATH");
	if (data_path != NULL) {
		if (check_path(data, data_path, output))
			return true;
	}

	if (check_path(data, OBS_DATA_PATH "/obs-studio/", output))
		return true;
	if (check_path(data, INSTALL_DATA_PATH, output))
		return true;

	return false;
}

bool InitApplicationBundle()
{
	return true;
}

string GetDefaultVideoSavePath()
{
	return string(getenv("HOME"));
}

vector<string> GetPreferredLocales()
{
	setlocale(LC_ALL, "");
	vector<string> matched;
	string messages = setlocale(LC_MESSAGES, NULL);
	if (!messages.size() || messages == "C" || messages == "POSIX")
		return {};

	if (messages.size() > 2)
		messages[2] = '-';

	for (auto &locale_pair : GetLocaleNames()) {
		auto &locale = locale_pair.first;
		if (locale == messages.substr(0, locale.size()))
			return {locale};

		if (locale.substr(0, 2) == messages.substr(0, 2))
			matched.push_back(locale);
	}

	return matched;
}

bool IsAlwaysOnTop(QWidget *window)
{
	return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

void SetAlwaysOnTop(QWidget *window, bool enable)
{
	Qt::WindowFlags flags = window->windowFlags();

	if (enable)
		flags |= Qt::WindowStaysOnTopHint;
	else
		flags &= ~Qt::WindowStaysOnTopHint;

	window->setWindowFlags(flags);
	window->show();
}
