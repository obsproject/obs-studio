/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
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

#include "platform.hpp"

#include <OBSApp.hpp>

#include <obs-config.h>
#include <util/bmem.h>
#include <util/platform.h>

#include <QDBusConnection>
#include <QDBusReply>

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <condition_variable>
#include <mutex>
#endif
#include <sstream>
#include <string>
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <thread>
#endif
#include <vector>

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <fcntl.h>
#include <libprocstat.h>
#include <pthread_np.h>
#endif
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/param.h>
#endif
#ifdef __linux__
#include <sys/socket.h>
#endif
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/sysctl.h>
#endif
#ifdef __linux__
#include <sys/un.h>
#endif
#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <sys/user.h>
#endif
#include <unistd.h>

using std::ostringstream;
using std::string;
using std::vector;

#ifdef __linux__
void CheckIfAlreadyRunning(bool &already_running)
{
	int uniq = socket(AF_LOCAL, SOCK_DGRAM | SOCK_CLOEXEC, 0);

	if (uniq == -1) {
		blog(LOG_ERROR, "Failed to check for running instance, socket: %d", errno);
		already_running = 0;
		return;
	}

	struct sockaddr_un bindInfo;
	memset(&bindInfo, 0, sizeof(sockaddr_un));
	bindInfo.sun_family = AF_LOCAL;
	auto bindInfoStrlen = snprintf(bindInfo.sun_path + 1, sizeof(bindInfo.sun_path) - 1, "%s %d %s",
				       "/com/obsproject", getpid(), App()->GetVersionString().c_str());

	int bindErr = bind(uniq, (struct sockaddr *)&bindInfo, sizeof(sa_family_t) + 1 + bindInfoStrlen);
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
#if defined(__FreeBSD__) || defined(__DragonFly__)
struct RunOnce {
	std::thread thr;
	static const char *thr_name;
	std::condition_variable cv;
	std::condition_variable wait_cv;
	std::mutex mtx;
	bool exiting = false;
	bool name_changed = false;

	void thr_proc()
	{
		std::unique_lock<std::mutex> lk(mtx);
		pthread_set_name_np(pthread_self(), thr_name);
		name_changed = true;
		wait_cv.notify_all();
		cv.wait(lk, [this]() { return exiting; });
	}

	~RunOnce()
	{
		if (thr.joinable()) {
			std::unique_lock<std::mutex> lk(mtx);
			exiting = true;
			cv.notify_one();
			lk.unlock();
			thr.join();
		}
	}
} RO;
const char *RunOnce::thr_name = "OBS runonce";

void CheckIfAlreadyRunning(bool &already_running)
{
	std::string tmpfile_name = "/tmp/obs-studio.lock." + std::to_string(geteuid());
	int fd = open(tmpfile_name.c_str(), O_RDWR | O_CREAT | O_EXLOCK, 0600);
	if (fd == -1) {
		already_running = true;
		return;
	}

	already_running = false;

	procstat *ps = procstat_open_sysctl();
	unsigned int count;
	auto procs = procstat_getprocs(ps, KERN_PROC_UID | KERN_PROC_INC_THREAD, geteuid(), &count);
	for (unsigned int i = 0; i < count; i++) {
		if (!strncmp(procs[i].ki_tdname, RunOnce::thr_name, sizeof(procs[i].ki_tdname))) {
			already_running = true;
			break;
		}
	}
	procstat_freeprocs(ps, procs);
	procstat_close(ps);

	RO.thr = std::thread(std::mem_fn(&RunOnce::thr_proc), &RO);

	{
		std::unique_lock<std::mutex> lk(RO.mtx);
		RO.wait_cv.wait(lk, []() { return RO.name_changed; });
	}

	unlink(tmpfile_name.c_str());
	close(fd);
}
#endif

static inline bool check_path(const char *data, const char *path, string &output)
{
	ostringstream str;
	str << path << data;
	output = str.str();

	blog(LOG_DEBUG, "Attempted path: %s", output.c_str());

	return (access(output.c_str(), R_OK) == 0);
}

#define INSTALL_DATA_PATH OBS_INSTALL_PREFIX "/" OBS_DATA_PATH "/obs-studio/"

bool GetDataFilePath(const char *data, string &output)
{
	char *data_path = getenv("OBS_DATA_PATH");
	if (data_path != NULL) {
		if (check_path(data, data_path, output))
			return true;
	}

	char *relative_data_path = os_get_executable_path_ptr("../" OBS_DATA_PATH "/obs-studio/");

	if (relative_data_path) {
		bool result = check_path(data, relative_data_path, output);
		bfree(relative_data_path);

		if (result) {
			return true;
		}
	}

	if (check_path(data, OBS_DATA_PATH "/obs-studio/", output))
		return true;

	if (check_path(data, INSTALL_DATA_PATH, output))
		return true;

	return false;
}

string GetDefaultVideoSavePath()
{
	return string(getenv("HOME"));
}

vector<string> GetPreferredLocales()
{
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

bool SetDisplayAffinitySupported(void)
{
	// Not implemented yet
	return false;
}

// Not implemented yet
void TaskbarOverlayInit() {}
void TaskbarOverlaySetStatus(TaskbarOverlayStatus) {}

bool HighContrastEnabled()
{
	QDBusReply<QVariant> reply;
	QDBusMessage msgXdpSettingsVersion = QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop",
									    "/org/freedesktop/portal/desktop",
									    "org.freedesktop.DBus.Properties", "Get");
	msgXdpSettingsVersion << "org.freedesktop.portal.Settings"
			      << "version";

	reply = QDBusConnection::sessionBus().call(msgXdpSettingsVersion);

	if (!reply.isValid()) {
		blog(LOG_WARNING, "Get on org.freedesktop.portal.Settings returned an invalid reply");
		return false;
	}

	/* NOTE: org.freedesktop.portal.Settings got its contrast settings after
	 * the ReadOne method. So assumes that if ReadOne is not available, contrast
	 * isn't available either. */
	if (uint32_t version = reply.value().toUInt() < 2) {
		blog(LOG_WARNING, "org.freedesktop.portal.Settings version %u does not support ReadOne", version);
		return false;
	}

	/* NOTE: If contrast is not available if will return 0 (false). */
	QDBusMessage msgXdpSettingsContrast =
		QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
					       "org.freedesktop.portal.Settings", "ReadOne");

	msgXdpSettingsContrast << "org.freedesktop.appearance"
			       << "contrast";

	reply = QDBusConnection::sessionBus().call(msgXdpSettingsContrast);

	if (!reply.isValid()) {
		blog(LOG_WARNING, "ReadOne on org.freedesktop.portal.Settings returned an invalid reply");
		return false;
	}

	return reply.value().toUInt() != 0;
}
