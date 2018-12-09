/******************************************************************************
    Copyright (C) 2018 by Hugh Bailey <obs.jim@gmail.com>

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

#include <QDir>
#include <QThread>
#include <QMessageBox>
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#ifdef BROWSER_AVAILABLE
#include <browser-panel.hpp>
#endif

struct QCef;
struct QCefCookieManager;

extern QCef              *cef;
extern QCefCookieManager *panel_cookies;

#ifdef BROWSER_AVAILABLE
static void InitBrowserSafeBlockEvent()
{
	QEventLoop eventLoop;

	auto wait = [&] ()
	{
		cef->wait_for_browser_init();
		QMetaObject::invokeMethod(&eventLoop, "quit",
				Qt::QueuedConnection);
	};

	QScopedPointer<QThread> thread(CreateQThread(wait));
	thread->start();
	eventLoop.exec();
	thread->wait();
}

static void InitBrowserSafeBlockMsgBox()
{
	QMessageBox dlg;
	dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	dlg.setWindowTitle(QTStr("BrowserPanelInit.Title"));
	dlg.setText(QTStr("BrowserPanelInit.Text"));
	dlg.setStandardButtons(0);

	auto wait = [&] ()
	{
		cef->wait_for_browser_init();
		QMetaObject::invokeMethod(&dlg, "accept", Qt::QueuedConnection);
	};

	QScopedPointer<QThread> thread(CreateQThread(wait));
	thread->start();
	dlg.exec();
	thread->wait();
}

static void InitPanelCookieManager()
{
	if (!cef)
		return;
	if (panel_cookies)
		return;

	const char *profile = config_get_string(App()->GlobalConfig(),
			"Basic", "Profile");

	std::string sub_path;
	sub_path += "obs_profile_cookies/";
	sub_path += profile;

	panel_cookies = cef->create_cookie_manager(sub_path);
}
#endif

void OBSBasic::InitBrowserPanelSafeBlock(bool showDialog)
{
#ifdef BROWSER_AVAILABLE
	if (!cef)
		return;
	if (cef->init_browser()) {
		InitPanelCookieManager();
		return;
	}

	if (showDialog)
		InitBrowserSafeBlockMsgBox();
	else
		InitBrowserSafeBlockEvent();
	InitPanelCookieManager();
#else
	UNUSED_PARAMETER(showDialog);
#endif
}

void DestroyPanelCookieManager()
{
#ifdef BROWSER_AVAILABLE
	delete panel_cookies;
	panel_cookies = nullptr;
#endif
}

void DeletePanelCookies(const char *profile)
{
#ifdef BROWSER_AVAILABLE
	if (!cef)
		return;

	std::string sub_path;
	sub_path += "obs_profile_cookies/";
	sub_path += profile;

	std::string path = cef->get_cookie_path(sub_path);
	QDir dir(path.c_str());
	dir.removeRecursively();
#else
	UNUSED_PARAMETER(profile);
#endif
}

void DuplicateCurrentCookieProfile(const std::string &newProfile)
{
#ifdef BROWSER_AVAILABLE
	if (panel_cookies) {
		std::string sub_path;
		sub_path += "cookies\\";
		sub_path += newProfile;

		panel_cookies->FlushStore();
		panel_cookies->SetStoragePath(sub_path);
	}
#else
	UNUSED_PARAMETER(newProfile);
#endif
}
