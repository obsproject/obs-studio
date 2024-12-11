/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>
                          Zachary Lund <admin@computerquip.com>
                          Philippe Groarke <philippe.groarke@gmail.com>

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

#include "OBSBasic.hpp"

#ifdef YOUTUBE_ENABLED
#include <dialogs/OBSYoutubeActions.hpp>
#include <docks/YouTubeAppDock.hpp>
#include <utility/YoutubeApiWrappers.hpp>
#endif

#include <qt-wrappers.hpp>

using namespace std;

extern bool cef_js_avail;

#ifdef YOUTUBE_ENABLED
void OBSBasic::YouTubeActionDialogOk(const QString &broadcast_id, const QString &stream_id, const QString &key,
				     bool autostart, bool autostop, bool start_now)
{
	//blog(LOG_DEBUG, "Stream key: %s", QT_TO_UTF8(key));
	obs_service_t *service_obj = GetService();
	OBSDataAutoRelease settings = obs_service_get_settings(service_obj);

	const std::string a_key = QT_TO_UTF8(key);
	obs_data_set_string(settings, "key", a_key.c_str());

	const std::string b_id = QT_TO_UTF8(broadcast_id);
	obs_data_set_string(settings, "broadcast_id", b_id.c_str());

	const std::string s_id = QT_TO_UTF8(stream_id);
	obs_data_set_string(settings, "stream_id", s_id.c_str());

	obs_service_update(service_obj, settings);
	autoStartBroadcast = autostart;
	autoStopBroadcast = autostop;
	broadcastReady = true;

	emit BroadcastStreamReady(broadcastReady);

	if (start_now)
		QMetaObject::invokeMethod(this, "StartStreaming");
}

void OBSBasic::YoutubeStreamCheck(const std::string &key)
{
	YoutubeApiWrappers *apiYouTube(dynamic_cast<YoutubeApiWrappers *>(GetAuth()));
	if (!apiYouTube) {
		/* technically we should never get here -Lain */
		QMetaObject::invokeMethod(this, "ForceStopStreaming", Qt::QueuedConnection);
		youtubeStreamCheckThread->deleteLater();
		blog(LOG_ERROR, "==========================================");
		blog(LOG_ERROR, "%s: Uh, hey, we got here", __FUNCTION__);
		blog(LOG_ERROR, "==========================================");
		return;
	}

	int timeout = 0;
	json11::Json json;
	QString id = key.c_str();

	while (StreamingActive()) {
		if (timeout == 14) {
			QMetaObject::invokeMethod(this, "ForceStopStreaming", Qt::QueuedConnection);
			break;
		}

		if (!apiYouTube->FindStream(id, json)) {
			QMetaObject::invokeMethod(this, "DisplayStreamStartError", Qt::QueuedConnection);
			QMetaObject::invokeMethod(this, "StopStreaming", Qt::QueuedConnection);
			break;
		}

		auto item = json["items"][0];
		auto status = item["status"]["streamStatus"].string_value();
		if (status == "active") {
			emit BroadcastStreamActive();
			break;
		} else {
			QThread::sleep(1);
			timeout++;
		}
	}

	youtubeStreamCheckThread->deleteLater();
}

void OBSBasic::ShowYouTubeAutoStartWarning()
{
	auto msgBox = []() {
		QMessageBox msgbox(App()->GetMainWindow());
		msgbox.setWindowTitle(QTStr("YouTube.Actions.AutoStartStreamingWarning.Title"));
		msgbox.setText(QTStr("YouTube.Actions.AutoStartStreamingWarning"));
		msgbox.setIcon(QMessageBox::Icon::Information);
		msgbox.addButton(QMessageBox::Ok);

		QCheckBox *cb = new QCheckBox(QTStr("DoNotShowAgain"));
		msgbox.setCheckBox(cb);

		msgbox.exec();

		if (cb->isChecked()) {
			config_set_bool(App()->GetUserConfig(), "General", "WarnedAboutYouTubeAutoStart", true);
			config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
		}
	};

	bool warned = config_get_bool(App()->GetUserConfig(), "General", "WarnedAboutYouTubeAutoStart");
	if (!warned) {
		QMetaObject::invokeMethod(App(), "Exec", Qt::QueuedConnection, Q_ARG(VoidFunc, msgBox));
	}
}
#endif

void OBSBasic::BroadcastButtonClicked()
{
	if (!broadcastReady || (!broadcastActive && !outputHandler->StreamingActive())) {
		SetupBroadcast();
		return;
	}

	if (!autoStartBroadcast) {
#ifdef YOUTUBE_ENABLED
		std::shared_ptr<YoutubeApiWrappers> ytAuth = dynamic_pointer_cast<YoutubeApiWrappers>(auth);
		if (ytAuth.get()) {
			if (!ytAuth->StartLatestBroadcast()) {
				auto last_error = ytAuth->GetLastError();
				if (last_error.isEmpty())
					last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
				if (!ytAuth->GetTranslatedError(last_error))
					last_error = QTStr("YouTube.Actions.Error.BroadcastTransitionFailed")
							     .arg(last_error, ytAuth->GetBroadcastId());

				OBSMessageBox::warning(this, QTStr("Output.BroadcastStartFailed"), last_error, true);
				return;
			}
		}
#endif
		broadcastActive = true;
		autoStartBroadcast = true; // and clear the flag

		emit BroadcastStreamStarted(autoStopBroadcast);
	} else if (!autoStopBroadcast) {
#ifdef YOUTUBE_ENABLED
		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingStream");
		if (confirm && isVisible()) {
			QMessageBox::StandardButton button = OBSMessageBox::question(
				this, QTStr("ConfirmStop.Title"), QTStr("YouTube.Actions.AutoStopStreamingWarning"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;
		}

		std::shared_ptr<YoutubeApiWrappers> ytAuth = dynamic_pointer_cast<YoutubeApiWrappers>(auth);
		if (ytAuth.get()) {
			if (!ytAuth->StopLatestBroadcast()) {
				auto last_error = ytAuth->GetLastError();
				if (last_error.isEmpty())
					last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
				if (!ytAuth->GetTranslatedError(last_error))
					last_error = QTStr("YouTube.Actions.Error.BroadcastTransitionFailed")
							     .arg(last_error, ytAuth->GetBroadcastId());

				OBSMessageBox::warning(this, QTStr("Output.BroadcastStopFailed"), last_error, true);
			}
		}
#endif
		broadcastActive = false;
		broadcastReady = false;

		autoStopBroadcast = true;
		QMetaObject::invokeMethod(this, "StopStreaming");
		emit BroadcastStreamReady(broadcastReady);
		SetBroadcastFlowEnabled(true);
	}
}

void OBSBasic::SetBroadcastFlowEnabled(bool enabled)
{
	emit BroadcastFlowEnabled(enabled);
}

void OBSBasic::SetupBroadcast()
{
#ifdef YOUTUBE_ENABLED
	Auth *const auth = GetAuth();
	if (IsYouTubeService(auth->service())) {
		OBSYoutubeActions dialog(this, auth, broadcastReady);
		connect(&dialog, &OBSYoutubeActions::ok, this, &OBSBasic::YouTubeActionDialogOk);
		dialog.exec();
	}
#endif
}

#ifdef YOUTUBE_ENABLED
YouTubeAppDock *OBSBasic::GetYouTubeAppDock()
{
	return youtubeAppDock;
}

#ifndef SEC_TO_NSEC
#define SEC_TO_NSEC 1000000000
#endif

void OBSBasic::NewYouTubeAppDock()
{
	if (!cef_js_avail)
		return;

	/* make sure that the youtube app dock can't be immediately recreated.
	 * dumb hack. blame chromium. or this particular dock. or both. if CEF
	 * creates/destroys/creates a widget too quickly it can lead to a
	 * crash. */
	uint64_t ts = os_gettime_ns();
	if ((ts - lastYouTubeAppDockCreationTime) < (5ULL * SEC_TO_NSEC))
		return;

	lastYouTubeAppDockCreationTime = ts;

	if (youtubeAppDock)
		RemoveDockWidget(youtubeAppDock->objectName());

	youtubeAppDock = new YouTubeAppDock("YouTube Live Control Panel");
}

void OBSBasic::DeleteYouTubeAppDock()
{
	if (!cef_js_avail)
		return;

	if (youtubeAppDock)
		RemoveDockWidget(youtubeAppDock->objectName());

	youtubeAppDock = nullptr;
}
#endif
