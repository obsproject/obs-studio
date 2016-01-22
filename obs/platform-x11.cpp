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

/* Here we use xinerama to fetch data about monitor geometry
 * Even if there are not multiple monitors, this should still work.
 */

#include <obs-config.h>
#include "obs-app.hpp"

#include <xcb/xcb.h>
#include <xcb/xinerama.h> 
#include <xcb/randr.h> 
#include <unistd.h>
#include <sstream>
#include <locale.h>

#include "platform.hpp"
using namespace std;

static inline bool check_path(const char* data, const char *path,
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

void GetMonitors(vector<MonitorInfo> &monitors)
{
	xcb_connection_t* xcb_conn;

	monitors.clear();
	xcb_conn = xcb_connect(NULL, NULL);

	bool use_xinerama = false;
	if (xcb_get_extension_data(xcb_conn, &xcb_xinerama_id)->present) {
		xcb_xinerama_is_active_cookie_t xinerama_cookie;
		xcb_xinerama_is_active_reply_t* xinerama_reply = NULL;

		xinerama_cookie = xcb_xinerama_is_active(xcb_conn);
		xinerama_reply = xcb_xinerama_is_active_reply(xcb_conn,
						        xinerama_cookie, NULL);

		if (xinerama_reply && xinerama_reply->state != 0)
			use_xinerama = true;
		free(xinerama_reply);
	}

	if (use_xinerama) {
		xcb_xinerama_query_screens_cookie_t screens_cookie;
		xcb_xinerama_query_screens_reply_t* screens_reply = NULL;
		xcb_xinerama_screen_info_iterator_t iter;

		screens_cookie = xcb_xinerama_query_screens(xcb_conn);
		screens_reply = xcb_xinerama_query_screens_reply(xcb_conn,
							screens_cookie, NULL);
		iter = xcb_xinerama_query_screens_screen_info_iterator(
								screens_reply);

		for(; iter.rem; xcb_xinerama_screen_info_next(&iter)) {
			monitors.emplace_back(iter.data->x_org,
					      iter.data->y_org,
					      iter.data->width,
					      iter.data->height);
		}
		free(screens_reply);
	} else {
		// no xinerama so fall back to basic x11 calls
		xcb_screen_iterator_t iter;

		iter = xcb_setup_roots_iterator(xcb_get_setup(xcb_conn));
		for(; iter.rem; xcb_screen_next(&iter)) {
			monitors.emplace_back(0,0,iter.data->width_in_pixels,
					          iter.data->height_in_pixels);
		}
	}

	xcb_disconnect(xcb_conn);
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
			return {locale};
	}

	return {};
}

bool IsAlwaysOnTop(QMainWindow *window)
{
	return (window->windowFlags() & Qt::WindowStaysOnTopHint) != 0;
}

void SetAlwaysOnTop(QMainWindow *window, bool enable)
{
	Qt::WindowFlags flags = window->windowFlags();

	if (enable)
		flags |= Qt::WindowStaysOnTopHint;
	else
		flags &= ~Qt::WindowStaysOnTopHint;

	window->setWindowFlags(flags);
	window->show();
}
