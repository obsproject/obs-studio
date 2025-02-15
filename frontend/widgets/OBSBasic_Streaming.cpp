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

#include <components/UIValidation.hpp>
#ifdef YOUTUBE_ENABLED
#include <docks/YouTubeAppDock.hpp>
#include <utility/YoutubeApiWrappers.hpp>
#endif

#include <qt-wrappers.hpp>

#define STREAMING_START "==== Streaming Start ==============================================="
#define STREAMING_STOP "==== Streaming Stop ================================================"

void OBSBasic::DisplayStreamStartError()
{
	QString message = !outputHandler->lastError.empty() ? QTStr(outputHandler->lastError.c_str())
							    : QTStr("Output.StartFailedGeneric");

	emit StreamingStopped();

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StartStreaming"));
		sysTrayStream->setEnabled(true);
	}

	QMessageBox::critical(this, QTStr("Output.StartStreamFailed"), message);
}

void OBSBasic::StartStreaming()
{
	if (outputHandler->StreamingActive())
		return;
	if (disableOutputsRef)
		return;

	if (auth && auth->broadcastFlow()) {
		if (!broadcastActive && !broadcastReady) {
			QMessageBox no_broadcast(this);
			no_broadcast.setText(QTStr("Output.NoBroadcast.Text"));
			QPushButton *SetupBroadcast =
				no_broadcast.addButton(QTStr("Basic.Main.SetupBroadcast"), QMessageBox::YesRole);
			no_broadcast.setDefaultButton(SetupBroadcast);
			no_broadcast.addButton(QTStr("Close"), QMessageBox::NoRole);
			no_broadcast.setIcon(QMessageBox::Information);
			no_broadcast.setWindowTitle(QTStr("Output.NoBroadcast.Title"));
			no_broadcast.exec();

			if (no_broadcast.clickedButton() == SetupBroadcast)
				QMetaObject::invokeMethod(this, "SetupBroadcast");
			return;
		}
	}

	emit StreamingPreparing();

	if (sysTrayStream) {
		sysTrayStream->setEnabled(false);
		sysTrayStream->setText("Basic.Main.PreparingStream");
	}

	auto finish_stream_setup = [&](bool setupStreamingResult) {
		if (!setupStreamingResult) {
			DisplayStreamStartError();
			return;
		}

		OnEvent(OBS_FRONTEND_EVENT_STREAMING_STARTING);

		SaveProject();

		emit StreamingStarting(autoStartBroadcast);

		if (sysTrayStream)
			sysTrayStream->setText("Basic.Main.Connecting");

		if (!outputHandler->StartStreaming(service)) {
			DisplayStreamStartError();
			return;
		}

		if (autoStartBroadcast) {
			emit BroadcastStreamStarted(autoStopBroadcast);
			broadcastActive = true;
		}

		bool recordWhenStreaming =
			config_get_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming");
		if (recordWhenStreaming)
			StartRecording();

		bool replayBufferWhileStreaming =
			config_get_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
		if (replayBufferWhileStreaming)
			StartReplayBuffer();

#ifdef YOUTUBE_ENABLED
		if (!autoStartBroadcast)
			OBSBasic::ShowYouTubeAutoStartWarning();
#endif
	};

	setupStreamingGuard = outputHandler->SetupStreaming(service, finish_stream_setup);
}

void OBSBasic::StopStreaming()
{
	SaveProject();

	if (outputHandler->StreamingActive())
		outputHandler->StopStreaming(streamingStopping);

	// special case: force reset broadcast state if
	// no autostart and no autostop selected
	if (!autoStartBroadcast && !broadcastActive) {
		broadcastActive = false;
		autoStartBroadcast = true;
		autoStopBroadcast = true;
		broadcastReady = false;
	}

	if (autoStopBroadcast) {
		broadcastActive = false;
		broadcastReady = false;
	}

	emit BroadcastStreamReady(broadcastReady);

	OnDeactivate();

	bool recordWhenStreaming = config_get_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming");
	bool keepRecordingWhenStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
	if (recordWhenStreaming && !keepRecordingWhenStreamStops)
		StopRecording();

	bool replayBufferWhileStreaming =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
	bool keepReplayBufferStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
	if (replayBufferWhileStreaming && !keepReplayBufferStreamStops)
		StopReplayBuffer();
}

void OBSBasic::ForceStopStreaming()
{
	SaveProject();

	if (outputHandler->StreamingActive())
		outputHandler->StopStreaming(true);

	// special case: force reset broadcast state if
	// no autostart and no autostop selected
	if (!autoStartBroadcast && !broadcastActive) {
		broadcastActive = false;
		autoStartBroadcast = true;
		autoStopBroadcast = true;
		broadcastReady = false;
	}

	if (autoStopBroadcast) {
		broadcastActive = false;
		broadcastReady = false;
	}

	emit BroadcastStreamReady(broadcastReady);

	OnDeactivate();

	bool recordWhenStreaming = config_get_bool(App()->GetUserConfig(), "BasicWindow", "RecordWhenStreaming");
	bool keepRecordingWhenStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepRecordingWhenStreamStops");
	if (recordWhenStreaming && !keepRecordingWhenStreamStops)
		StopRecording();

	bool replayBufferWhileStreaming =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "ReplayBufferWhileStreaming");
	bool keepReplayBufferStreamStops =
		config_get_bool(App()->GetUserConfig(), "BasicWindow", "KeepReplayBufferStreamStops");
	if (replayBufferWhileStreaming && !keepReplayBufferStreamStops)
		StopReplayBuffer();
}

void OBSBasic::StreamDelayStarting(int sec)
{
	emit StreamingStarted(true);

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StopStreaming"));
		sysTrayStream->setEnabled(true);
	}

	ui->statusbar->StreamDelayStarting(sec);

	OnActivate();
}

void OBSBasic::StreamDelayStopping(int sec)
{
	emit StreamingStopped(true);

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StartStreaming"));
		sysTrayStream->setEnabled(true);
	}

	ui->statusbar->StreamDelayStopping(sec);

	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
}

void OBSBasic::StreamingStart()
{
	emit StreamingStarted();
	OBSOutputAutoRelease output = obs_frontend_get_streaming_output();
	ui->statusbar->StreamStarted(output);

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StopStreaming"));
		sysTrayStream->setEnabled(true);
	}

#ifdef YOUTUBE_ENABLED
	if (!autoStartBroadcast) {
		// get a current stream key
		obs_service_t *service_obj = GetService();
		OBSDataAutoRelease settings = obs_service_get_settings(service_obj);
		std::string key = obs_data_get_string(settings, "stream_id");
		if (!key.empty() && !youtubeStreamCheckThread) {
			youtubeStreamCheckThread = CreateQThread([this, key] { YoutubeStreamCheck(key); });
			youtubeStreamCheckThread->setObjectName("YouTubeStreamCheckThread");
			youtubeStreamCheckThread->start();
		}
	}
#endif

	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STARTED);

	OnActivate();

#ifdef YOUTUBE_ENABLED
	if (YouTubeAppDock::IsYTServiceSelected())
		youtubeAppDock->IngestionStarted();
#endif

	blog(LOG_INFO, STREAMING_START);
}

void OBSBasic::StreamStopping()
{
	emit StreamingStopping();

	if (sysTrayStream)
		sysTrayStream->setText(QTStr("Basic.Main.StoppingStreaming"));

	streamingStopping = true;
	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPING);
}

void OBSBasic::StreamingStop(int code, QString last_error)
{
	const char *errorDescription = "";
	DStr errorMessage;
	bool use_last_error = false;
	bool encode_error = false;
	bool should_reconnect = false;

	/* Ignore stream key error for multitrack output if its internal reconnect handling is active. */
	if (code == OBS_OUTPUT_INVALID_STREAM && outputHandler->multitrackVideo &&
	    outputHandler->multitrackVideo->RestartOnError()) {
		code = OBS_OUTPUT_SUCCESS;
		should_reconnect = true;
	}

	switch (code) {
	case OBS_OUTPUT_BAD_PATH:
		errorDescription = Str("Output.ConnectFail.BadPath");
		break;

	case OBS_OUTPUT_CONNECT_FAILED:
		use_last_error = true;
		errorDescription = Str("Output.ConnectFail.ConnectFailed");
		break;

	case OBS_OUTPUT_INVALID_STREAM:
		errorDescription = Str("Output.ConnectFail.InvalidStream");
		break;

	case OBS_OUTPUT_ENCODE_ERROR:
		encode_error = true;
		break;

	case OBS_OUTPUT_HDR_DISABLED:
		errorDescription = Str("Output.ConnectFail.HdrDisabled");
		break;

	default:
	case OBS_OUTPUT_ERROR:
		use_last_error = true;
		errorDescription = Str("Output.ConnectFail.Error");
		break;

	case OBS_OUTPUT_DISCONNECTED:
		/* doesn't happen if output is set to reconnect.  note that
		 * reconnects are handled in the output, not in the UI */
		use_last_error = true;
		errorDescription = Str("Output.ConnectFail.Disconnected");
	}

	if (use_last_error && !last_error.isEmpty())
		dstr_printf(errorMessage, "%s\n\n%s", errorDescription, QT_TO_UTF8(last_error));
	else
		dstr_copy(errorMessage, errorDescription);

	ui->statusbar->StreamStopped();

	emit StreamingStopped();

	if (sysTrayStream) {
		sysTrayStream->setText(QTStr("Basic.Main.StartStreaming"));
		sysTrayStream->setEnabled(true);
	}

	streamingStopping = false;
	OnEvent(OBS_FRONTEND_EVENT_STREAMING_STOPPED);

	OnDeactivate();

#ifdef YOUTUBE_ENABLED
	if (YouTubeAppDock::IsYTServiceSelected())
		youtubeAppDock->IngestionStopped();
#endif

	blog(LOG_INFO, STREAMING_STOP);

	if (encode_error) {
		QString msg = last_error.isEmpty() ? QTStr("Output.StreamEncodeError.Msg")
						   : QTStr("Output.StreamEncodeError.Msg.LastError").arg(last_error);
		OBSMessageBox::information(this, QTStr("Output.StreamEncodeError.Title"), msg);

	} else if (code != OBS_OUTPUT_SUCCESS && isVisible()) {
		OBSMessageBox::information(this, QTStr("Output.ConnectFail.Title"), QT_UTF8(errorMessage));

	} else if (code != OBS_OUTPUT_SUCCESS && !isVisible()) {
		SysTrayNotify(QT_UTF8(errorDescription), QSystemTrayIcon::Warning);
	}

	// Reset broadcast button state/text
	if (!broadcastActive)
		SetBroadcastFlowEnabled(auth && auth->broadcastFlow());
	if (should_reconnect)
		QMetaObject::invokeMethod(this, "StartStreaming", Qt::QueuedConnection);
}

void OBSBasic::StreamActionTriggered()
{
	if (outputHandler->StreamingActive()) {
		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingStream");

#ifdef YOUTUBE_ENABLED
		if (isVisible() && auth && IsYouTubeService(auth->service()) && autoStopBroadcast) {
			QMessageBox::StandardButton button = OBSMessageBox::question(
				this, QTStr("ConfirmStop.Title"), QTStr("YouTube.Actions.AutoStopStreamingWarning"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;

			confirm = false;
		}
#endif
		if (confirm && isVisible()) {
			QMessageBox::StandardButton button =
				OBSMessageBox::question(this, QTStr("ConfirmStop.Title"), QTStr("ConfirmStop.Text"),
							QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;
		}

		StopStreaming();
	} else {
		if (!UIValidation::NoSourcesConfirmation(this))
			return;

		Auth *auth = GetAuth();

		auto action = (auth && auth->external()) ? StreamSettingsAction::ContinueStream
							 : UIValidation::StreamSettingsConfirmation(this, service);
		switch (action) {
		case StreamSettingsAction::ContinueStream:
			break;
		case StreamSettingsAction::OpenSettings:
			on_action_Settings_triggered();
			return;
		case StreamSettingsAction::Cancel:
			return;
		}

		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStartingStream");

		bool bwtest = false;

		if (this->auth) {
			OBSDataAutoRelease settings = obs_service_get_settings(service);
			bwtest = obs_data_get_bool(settings, "bwtest");
			// Disable confirmation if this is going to open broadcast setup
			if (auth && auth->broadcastFlow() && !broadcastReady && !broadcastActive)
				confirm = false;
		}

		if (bwtest && isVisible()) {
			QMessageBox::StandardButton button = OBSMessageBox::question(this, QTStr("ConfirmBWTest.Title"),
										     QTStr("ConfirmBWTest.Text"));

			if (button == QMessageBox::No)
				return;
		} else if (confirm && isVisible()) {
			QMessageBox::StandardButton button =
				OBSMessageBox::question(this, QTStr("ConfirmStart.Title"), QTStr("ConfirmStart.Text"),
							QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No)
				return;
		}

		StartStreaming();
	}
}

bool OBSBasic::StreamingActive()
{
	if (!outputHandler)
		return false;
	return outputHandler->StreamingActive();
}
