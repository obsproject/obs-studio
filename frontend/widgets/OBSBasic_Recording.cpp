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
#include <dialogs/OBSRemux.hpp>
#include <utility/RecordingSegmentMover.hpp>

#include <qt-wrappers.hpp>

#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMenu>

void OBSBasic::on_actionShow_Recordings_triggered()
{
	const char *mode = config_get_string(activeConfiguration, "Output", "Mode");
	const char *type = config_get_string(activeConfiguration, "AdvOut", "RecType");
	const char *adv_path = strcmp(type, "Standard")
				       ? config_get_string(activeConfiguration, "AdvOut", "FFFilePath")
				       : config_get_string(activeConfiguration, "AdvOut", "RecFilePath");
	const char *path = strcmp(mode, "Advanced") ? config_get_string(activeConfiguration, "SimpleOutput", "FilePath")
						    : adv_path;
	QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

#define RECORDING_START "==== Recording Start ==============================================="
#define RECORDING_STOP "==== Recording Stop ================================================"

extern volatile bool replaybuf_active;

void OBSBasic::AutoRemux(QString input, bool no_show)
{
	auto config = Config();

	bool autoRemux = config_get_bool(config, "Video", "AutoRemux");

	if (!autoRemux) {
		return;
	}

	bool isSimpleMode = false;

	const char *mode = config_get_string(config, "Output", "Mode");
	if (!mode) {
		isSimpleMode = true;
	} else {
		isSimpleMode = strcmp(mode, "Simple") == 0;
	}

	if (!isSimpleMode) {
		const char *recType = config_get_string(config, "AdvOut", "RecType");

		bool ffmpegOutput = astrcmpi(recType, "FFmpeg") == 0;

		if (ffmpegOutput) {
			return;
		}
	}

	if (input.isEmpty()) {
		return;
	}

	QFileInfo fi(input);
	QString suffix = fi.suffix();

	/* do not remux if lossless */
	if (suffix.compare("avi", Qt::CaseInsensitive) == 0) {
		return;
	}

	QString path = fi.path();

	QString output = input;
	output.resize(output.size() - suffix.size());

	const obs_encoder_t *videoEncoder = obs_output_get_video_encoder(outputHandler->fileOutput);
	const char *vCodecName = obs_encoder_get_codec(videoEncoder);
	const char *format = config_get_string(config, isSimpleMode ? "SimpleOutput" : "AdvOut", "RecFormat2");

	/* Retain original container for fMP4/fMOV */
	if (strncmp(format, "fragmented", 10) == 0) {
		output += "remuxed." + suffix;
	} else if (strcmp(vCodecName, "prores") == 0) {
		output += "mov";
	} else {
		output += "mp4";
	}

	OBSRemux *remux = new OBSRemux(QT_TO_UTF8(path), this, true);
	if (!no_show) {
		remux->show();
	}
	remux->AutoRemux(input, output);
}

void OBSBasic::StartRecording()
{
	if (outputHandler->RecordingActive()) {
		return;
	}
	if (disableOutputsRef) {
		return;
	}

	if (!OutputPathValid()) {
		OutputPathInvalidMessage();
		return;
	}

	if (!IsFFmpegOutputToURL() && LowDiskSpace()) {
		DiskSpaceMessage();
		return;
	}

	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STARTING);

	SaveProject();

	outputHandler->StartRecording();
}

void OBSBasic::RecordStopping()
{
	emit RecordingStopping();

	if (sysTrayRecord) {
		sysTrayRecord->setText(QTStr("Basic.Main.StoppingRecording"));
	}

	recordingStopping = true;
	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STOPPING);
}

void OBSBasic::StopRecording()
{
	SaveProject();

	if (outputHandler->RecordingActive()) {
		outputHandler->StopRecording(recordingStopping);
	}

	OnDeactivate();
}

void OBSBasic::RecordingStart()
{
	ui->statusbar->RecordingStarted(outputHandler->fileOutput);
	emit RecordingStarted(isRecordingPausable);

	pendingSplitFolder.clear();
	emit RecordingSplitStateChanged(SplitRecordingState::Idle, QString());
	emit RecordingSplitAvailable(RecordingCanSplit());

	if (sysTrayRecord) {
		sysTrayRecord->setText(QTStr("Basic.Main.StopRecording"));
	}

	recordingStopping = false;
	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STARTED);

	if (!diskFullTimer->isActive()) {
		diskFullTimer->start(1000);
	}

	OnActivate();

	blog(LOG_INFO, RECORDING_START);
}

void OBSBasic::RecordingStop(int code, QString last_error)
{
	ui->statusbar->RecordingStopped();
	emit RecordingStopped();

	/* A split was requested but the recording stopped before it happened:
	 * the final file is the segment the user asked to save. */
	QString splitFolder = pendingSplitFolder;
	pendingSplitFolder.clear();
	emit RecordingSplitStateChanged(SplitRecordingState::Idle, QString());
	emit RecordingSplitAvailable(false);

	if (sysTrayRecord) {
		sysTrayRecord->setText(QTStr("Basic.Main.StartRecording"));
	}

	blog(LOG_INFO, RECORDING_STOP);

	if (code == OBS_OUTPUT_UNSUPPORTED && isVisible()) {
		OBSMessageBox::critical(this, QTStr("Output.RecordFail.Title"), QTStr("Output.RecordFail.Unsupported"));

	} else if (code == OBS_OUTPUT_ENCODE_ERROR && isVisible()) {
		QString msg = last_error.isEmpty()
				      ? QTStr("Output.RecordError.EncodeErrorMsg")
				      : QTStr("Output.RecordError.EncodeErrorMsg.LastError").arg(last_error);
		OBSMessageBox::warning(this, QTStr("Output.RecordError.Title"), msg);

	} else if (code == OBS_OUTPUT_NO_SPACE && isVisible()) {
		OBSMessageBox::warning(this, QTStr("Output.RecordNoSpace.Title"), QTStr("Output.RecordNoSpace.Msg"));

	} else if (code != OBS_OUTPUT_SUCCESS && isVisible()) {

		const char *errorDescription;
		DStr errorMessage;
		bool use_last_error = true;

		errorDescription = Str("Output.RecordError.Msg");

		if (use_last_error && !last_error.isEmpty()) {
			dstr_printf(errorMessage, "%s<br><br>%s", errorDescription, QT_TO_UTF8(last_error));
		} else {
			dstr_copy(errorMessage, errorDescription);
		}

		OBSMessageBox::critical(this, QTStr("Output.RecordError.Title"), QT_UTF8(errorMessage));

	} else if (code == OBS_OUTPUT_UNSUPPORTED && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordFail.Unsupported"), QSystemTrayIcon::Warning);

	} else if (code == OBS_OUTPUT_NO_SPACE && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordNoSpace.Msg"), QSystemTrayIcon::Warning);

	} else if (code != OBS_OUTPUT_SUCCESS && !isVisible()) {
		SysTrayNotify(QTStr("Output.RecordError.Msg"), QSystemTrayIcon::Warning);
	} else if (code == OBS_OUTPUT_SUCCESS) {
		if (outputHandler) {
			std::string path = outputHandler->lastRecordingPath;
			QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
			ShowStatusBarMessage(str.arg(QT_UTF8(path.c_str())));
		}
	}

	OnEvent(OBS_FRONTEND_EVENT_RECORDING_STOPPED);

	if (diskFullTimer->isActive()) {
		diskFullTimer->stop();
	}

	if (!splitFolder.isEmpty() && !outputHandler->lastRecordingPath.empty()) {
		/* Remuxing happens after the move, see RecordingSegmentMoved */
		MoveRecordingSegment(QT_UTF8(outputHandler->lastRecordingPath.c_str()), QString(), splitFolder);
	} else {
		AutoRemux(outputHandler->lastRecordingPath.c_str());
	}

	OnDeactivate();
}

void OBSBasic::RecordingFileChanged(QString lastRecordingPath, QString nextRecordingPath)
{
	if (!pendingSplitFolder.isEmpty()) {
		QString folder = pendingSplitFolder;
		pendingSplitFolder.clear();

		/* Remuxing happens after the move, see RecordingSegmentMoved */
		MoveRecordingSegment(lastRecordingPath, nextRecordingPath, folder);
		return;
	}

	QString str = QTStr("Basic.StatusBar.RecordingSavedTo");
	ShowStatusBarMessage(str.arg(lastRecordingPath));

	AutoRemux(lastRecordingPath, true);
}

/* -------------------------------------------------------------------------
 * "Save Recording So Far to Folder"
 *
 * Splits the running recording at the next keyframe (using the outputs'
 * existing split_file mechanism) and moves the finished segment into a folder
 * chosen by the user. Recording and streaming continue uninterrupted.
 * ------------------------------------------------------------------------- */

#define SPLIT_FOLDERS_SECTION "BasicWindow"
#define SPLIT_FOLDERS_KEY "SplitRecordingRecentFolders"
#define MAX_RECENT_SPLIT_FOLDERS 5

bool OBSBasic::RecordingCanSplit()
{
	if (!outputHandler || !outputHandler->fileOutput) {
		return false;
	}

	/* Only the native recording muxers support splitting; the custom
	 * FFmpeg output (incl. Simple mode "Lossless") does not. */
	const char *id = obs_output_get_id(outputHandler->fileOutput);
	return id &&
	       (strcmp(id, "ffmpeg_muxer") == 0 || strcmp(id, "mp4_output") == 0 || strcmp(id, "mov_output") == 0);
}

QStringList OBSBasic::GetRecentSplitFolders() const
{
	QStringList folders;

	const char *json = config_get_string(App()->GetUserConfig(), SPLIT_FOLDERS_SECTION, SPLIT_FOLDERS_KEY);
	if (!json || !*json) {
		return folders;
	}

	const QJsonArray array = QJsonDocument::fromJson(QByteArray(json)).array();
	for (const QJsonValue &value : array) {
		QString folder = value.toString();
		if (!folder.isEmpty()) {
			folders << folder;
		}
	}

	return folders;
}

void OBSBasic::AddRecentSplitFolder(const QString &folder)
{
	QString cleanFolder = QDir::cleanPath(folder);
#ifdef _WIN32
	Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
	Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif

	QStringList folders = GetRecentSplitFolders();
	folders.removeIf([&](const QString &f) { return QDir::cleanPath(f).compare(cleanFolder, cs) == 0; });
	folders.prepend(cleanFolder);
	while (folders.size() > MAX_RECENT_SPLIT_FOLDERS) {
		folders.removeLast();
	}

	QJsonArray array;
	for (const QString &f : folders) {
		array.append(f);
	}

	QByteArray json = QJsonDocument(array).toJson(QJsonDocument::Compact);
	config_set_string(App()->GetUserConfig(), SPLIT_FOLDERS_SECTION, SPLIT_FOLDERS_KEY, json.constData());
	config_save_safe(App()->GetUserConfig(), "tmp", nullptr);
}

void OBSBasic::SplitRecordingToFolder(const QString &folder)
{
	if (folder.isEmpty() || !outputHandler || !outputHandler->RecordingActive() || recordingStopping) {
		return;
	}

	if (!pendingSplitFolder.isEmpty()) {
		/* A split is already on its way; it will happen at the next
		 * keyframe. */
		return;
	}

	if (os_atomic_load_bool(&recording_paused)) {
		ShowStatusBarMessage(QTStr("Basic.Main.SplitToFolder.Paused"));
		return;
	}

	if (!RecordingCanSplit()) {
		ShowStatusBarMessage(QTStr("Basic.Main.SplitToFolder.Unsupported"));
		return;
	}

	proc_handler_t *ph = obs_output_get_proc_handler(outputHandler->fileOutput);
	uint8_t stack[128];
	calldata cd;
	calldata_init_fixed(&cd, stack, sizeof(stack));
	bool called = proc_handler_call(ph, "split_file", &cd);
	bool enabled = called && calldata_bool(&cd, "split_file_enabled");

	if (!enabled) {
		blog(LOG_WARNING, "[Split to Folder] Recording output does not have splitting enabled");
		ShowStatusBarMessage(QTStr("Basic.Main.SplitToFolder.Unsupported"));
		return;
	}

	blog(LOG_INFO, "[Split to Folder] Splitting recording, segment will be saved to '%s'", QT_TO_UTF8(folder));

	pendingSplitFolder = folder;
	AddRecentSplitFolder(folder);

	emit RecordingSplitStateChanged(SplitRecordingState::Pending, folder);
	ShowStatusBarMessage(QTStr("Basic.Main.SplitToFolder.Pending"));
}

void OBSBasic::SplitRecordingToLastFolder()
{
	if (!outputHandler || !outputHandler->RecordingActive()) {
		return;
	}

	QStringList folders = GetRecentSplitFolders();
	if (folders.isEmpty()) {
		blog(LOG_INFO, "[Split to Folder] No folder chosen yet, ignoring hotkey");
		ShowStatusBarMessage(QTStr("Basic.Main.SplitToFolder.NoFolder"));
		return;
	}

	SplitRecordingToFolder(folders.first());
}

void OBSBasic::SplitRecordingBrowseFolder()
{
	if (!outputHandler || !outputHandler->RecordingActive()) {
		return;
	}

	QStringList folders = GetRecentSplitFolders();
	QString startDir = folders.isEmpty() ? QT_UTF8(GetCurrentOutputPath()) : folders.first();

	QString folder = SelectDirectory(this, QTStr("Basic.Main.SplitToFolder.SelectFolder"), startDir);
	if (folder.isEmpty()) {
		return;
	}

	/* The recording may have stopped while the dialog was open; the
	 * function checks for that. */
	SplitRecordingToFolder(folder);
}

void OBSBasic::ShowSplitRecordingMenu(QPoint pos)
{
	QMenu menu(this);

	QAction *header = menu.addAction(QTStr("Basic.Main.SplitToFolder.MenuHeader"));
	header->setEnabled(false);

	const QStringList folders = GetRecentSplitFolders();
	for (const QString &folder : folders) {
		QAction *action = menu.addAction(QDir::toNativeSeparators(folder));
		connect(action, &QAction::triggered, this, [this, folder]() { SplitRecordingToFolder(folder); });
	}

	menu.addSeparator();
	menu.addAction(QTStr("Browse"), this, &OBSBasic::SplitRecordingBrowseFolder);

	menu.exec(pos);
}

void OBSBasic::MoveRecordingSegment(const QString &source, const QString &nextFile, const QString &folder)
{
	if (source.isEmpty()) {
		return;
	}

	if (!segmentMover) {
		segmentMover = new RecordingSegmentMover(this);
		connect(segmentMover, &RecordingSegmentMover::Finished, this, &OBSBasic::RecordingSegmentMoved);
	}

	ShowStatusBarMessage(QTStr("Basic.Main.SplitToFolder.Saving").arg(QDir::toNativeSeparators(folder)));
	emit RecordingSplitStateChanged(SplitRecordingState::Saving, folder);
	segmentMover->Move(source, nextFile, folder);
}

void OBSBasic::RecordingSegmentMoved(const QString &source, const QString &destination, bool success)
{
	/* Only report on the button if no newer split is already under way */
	if (pendingSplitFolder.isEmpty()) {
		if (success) {
			emit RecordingSplitStateChanged(SplitRecordingState::Saved,
							QFileInfo(destination).absolutePath());
		} else {
			emit RecordingSplitStateChanged(SplitRecordingState::Failed, source);
		}
	}

	if (!success) {
		ShowStatusBarMessage(QTStr("Basic.Main.SplitToFolder.Failed").arg(QDir::toNativeSeparators(source)));
		if (!isVisible()) {
			SysTrayNotify(QTStr("Basic.Main.SplitToFolder.Failed").arg(QDir::toNativeSeparators(source)),
				      QSystemTrayIcon::Warning);
		}
		/* The segment stays where it was recorded; still remux it. */
		if (outputHandler && outputHandler->fileOutput) {
			AutoRemux(source, true);
		}
		return;
	}

	ShowStatusBarMessage(QTStr("Basic.StatusBar.RecordingSavedTo").arg(QDir::toNativeSeparators(destination)));

	if (outputHandler && outputHandler->fileOutput) {
		/* Don't pop up the remux window while still recording */
		AutoRemux(destination, RecordingActive());
	}
}

void OBSBasic::RecordActionTriggered()
{
	if (outputHandler->RecordingActive()) {
		bool confirm = config_get_bool(App()->GetUserConfig(), "BasicWindow", "WarnBeforeStoppingRecord");

		if (confirm && isVisible()) {
			QMessageBox::StandardButton button = OBSMessageBox::question(
				this, QTStr("ConfirmStopRecord.Title"), QTStr("ConfirmStopRecord.Text"),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

			if (button == QMessageBox::No) {
				return;
			}
		}
		StopRecording();
	} else {
		if (!UIValidation::NoSourcesConfirmation(this)) {
			return;
		}

		StartRecording();
	}
}

bool OBSBasic::RecordingActive()
{
	if (!outputHandler) {
		return false;
	}
	return outputHandler->RecordingActive();
}

void OBSBasic::PauseRecording()
{
	if (!isRecordingPausable || !outputHandler || !outputHandler->fileOutput ||
	    os_atomic_load_bool(&recording_paused)) {
		return;
	}

	obs_output_t *output = outputHandler->fileOutput;

	if (obs_output_pause(output, true)) {
		os_atomic_set_bool(&recording_paused, true);

		emit RecordingPaused();

		ui->statusbar->RecordingPaused();

		TaskbarOverlaySetStatus(TaskbarOverlayStatusPaused);
		if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
			QIcon trayIconFile = QIcon(":/res/images/obs_paused_macos.svg");
			trayIconFile.setIsMask(true);
#else
			QIcon trayIconFile = QIcon(":/res/images/obs_paused.png");
#endif
			trayIcon->setIcon(QIcon::fromTheme("obs-tray-paused", trayIconFile));
		}

		OnEvent(OBS_FRONTEND_EVENT_RECORDING_PAUSED);

		if (os_atomic_load_bool(&replaybuf_active)) {
			ShowReplayBufferPauseWarning();
		}
	}
}

void OBSBasic::UnpauseRecording()
{
	if (!isRecordingPausable || !outputHandler || !outputHandler->fileOutput ||
	    !os_atomic_load_bool(&recording_paused)) {
		return;
	}

	obs_output_t *output = outputHandler->fileOutput;

	if (obs_output_pause(output, false)) {
		os_atomic_set_bool(&recording_paused, false);

		emit RecordingUnpaused();

		ui->statusbar->RecordingUnpaused();

		TaskbarOverlaySetStatus(TaskbarOverlayStatusActive);
		if (trayIcon && trayIcon->isVisible()) {
#ifdef __APPLE__
			QIcon trayIconFile = QIcon(":/res/images/tray_active_macos.svg");
			trayIconFile.setIsMask(true);
#else
			QIcon trayIconFile = QIcon(":/res/images/tray_active.png");
#endif
			trayIcon->setIcon(QIcon::fromTheme("obs-tray-active", trayIconFile));
		}

		OnEvent(OBS_FRONTEND_EVENT_RECORDING_UNPAUSED);
	}
}

void OBSBasic::RecordPauseToggled()
{
	if (!isRecordingPausable || !outputHandler || !outputHandler->fileOutput) {
		return;
	}

	obs_output_t *output = outputHandler->fileOutput;
	bool enable = !obs_output_paused(output);

	if (enable) {
		PauseRecording();
	} else {
		UnpauseRecording();
	}
}

void OBSBasic::UpdateIsRecordingPausable()
{
	const char *mode = config_get_string(activeConfiguration, "Output", "Mode");
	bool adv = astrcmpi(mode, "Advanced") == 0;
	bool shared = true;

	if (adv) {
		const char *recType = config_get_string(activeConfiguration, "AdvOut", "RecType");

		if (astrcmpi(recType, "FFmpeg") == 0) {
			shared = config_get_bool(activeConfiguration, "AdvOut", "FFOutputToFile");
		} else {
			const char *recordEncoder = config_get_string(activeConfiguration, "AdvOut", "RecEncoder");
			shared = astrcmpi(recordEncoder, "none") == 0;
		}
	} else {
		const char *quality = config_get_string(activeConfiguration, "SimpleOutput", "RecQuality");
		shared = strcmp(quality, "Stream") == 0;
	}

	isRecordingPausable = !shared;
}

#define MBYTE (1024ULL * 1024ULL)
#define MBYTES_LEFT_STOP_REC 50ULL
#define MAX_BYTES_LEFT (MBYTES_LEFT_STOP_REC * MBYTE)

void OBSBasic::DiskSpaceMessage()
{
	blog(LOG_ERROR, "Recording stopped because of low disk space");

	OBSMessageBox::critical(this, QTStr("Output.RecordNoSpace.Title"), QTStr("Output.RecordNoSpace.Msg"));
}

bool OBSBasic::LowDiskSpace()
{
	const char *path;

	path = GetCurrentOutputPath();
	if (!path) {
		return false;
	}

	uint64_t num_bytes = os_get_free_disk_space(path);

	if (num_bytes < (MAX_BYTES_LEFT)) {
		return true;
	} else {
		return false;
	}
}

void OBSBasic::CheckDiskSpaceRemaining()
{
	if (LowDiskSpace()) {
		StopRecording();
		StopReplayBuffer();

		DiskSpaceMessage();
	}
}
