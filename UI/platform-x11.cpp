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

#ifdef __linux__
#include <sys/socket.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/un.h>
#endif

using namespace std;

#ifdef __linux__
void RunningInstanceCheck(bool &already_running)
{
	int uniq = socket(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0);

	if (uniq == -1) {
		blog(LOG_ERROR,
		     "Failed to check for running instance, socket: %d", errno);
		already_running = 0;
		return;
	}

	struct sockaddr_un bindInfo;
	memset(&bindInfo, 0, sizeof(sockaddr_un));
	bindInfo.sun_family = AF_LOCAL;
	char *abstactSockName = NULL;
	asprintf(&abstactSockName, "%s %d %s", "/com/obsproject", getpid(),
		 App()->GetVersionString().c_str());
	memmove(bindInfo.sun_path + 1, abstactSockName,
		strlen(abstactSockName));
	free(abstactSockName);

	int bindErr = bind(uniq, (struct sockaddr *)&bindInfo,
			   sizeof(struct sockaddr_un));
	already_running = bindErr == 0 ? 0 : 1;

	if (already_running) {
		return;
	}

	FILE *fp = fopen("/proc/net/unix", "re");

	if (fp == NULL) {
		return;
	}

	char *line = NULL;
	size_t n = 0;
	int obsCnt = 0;
	while (getdelim(&line, &n, ' ', fp) != EOF) {
		line[strcspn(line, "\n")] = '\0';
		if (*line == '@') {
			if (strstr(line, "@/com/obsproject") != NULL) {
				++obsCnt;
			}
		}
	}
	already_running = obsCnt == 1 ? 0 : 1;
	free(line);
	fclose(fp);
}
#endif

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
