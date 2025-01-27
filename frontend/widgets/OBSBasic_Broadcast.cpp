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
#ifdef RESTREAM_ENABLED
#include <dialogs/OBSRestreamActions.hpp>
#endif

#include <qt-wrappers.hpp>

using namespace std;

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

#ifdef RESTREAM_ENABLED
	Auth *const auth = GetAuth();
	if (auth && IsRestreamService(auth->service())) {
		auto restreamAuth = dynamic_cast<RestreamAuth *>(auth);
		broadcastReady = restreamAuth->IsBroadcastReady();
		emit BroadcastStreamReady(broadcastReady);
	}
#endif
}

void OBSBasic::SetupBroadcast()
{
#if defined YOUTUBE_ENABLED || defined RESTREAM_ENABLED
	Auth *const auth = GetAuth();
#endif
#ifdef YOUTUBE_ENABLED
	if (IsYouTubeService(auth->service())) {
		OBSYoutubeActions dialog(this, auth, broadcastReady);
		connect(&dialog, &OBSYoutubeActions::ok, this, &OBSBasic::YouTubeActionDialogOk);
		dialog.exec();
	}
#endif
#ifdef RESTREAM_ENABLED
	if (IsRestreamService(auth->service())) {
		OBSRestreamActions dialog(this, auth, broadcastReady);
		connect(&dialog, &OBSRestreamActions::ok, this, &OBSBasic::RestreamActionDialogOk);
		dialog.exec();
		return;
	}
#endif
}
