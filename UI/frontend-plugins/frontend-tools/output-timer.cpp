#include <obs-frontend-api.h>
#include <obs-module.h>
#include <obs.hpp>
#include <util/util.hpp>
#include <QAction>
#include <QMainWindow>
#include <QTimer>
#include <QObject>
#include "output-timer.hpp"

using namespace std;

OutputTimer *ot;

OutputTimer::OutputTimer(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui_OutputTimer)
{
	ui->setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QObject::connect(ui->outputTimerStream, &QPushButton::clicked, this,
			 &OutputTimer::StreamingTimerButton);
	QObject::connect(ui->outputTimerStartRecord, &QPushButton::clicked,
			 this, &OutputTimer::StartRecordingTimerButton);
	QObject::connect(ui->outputTimerRecord, &QPushButton::clicked, this,
			 &OutputTimer::RecordingTimerButton);
	QObject::connect(ui->buttonBox->button(QDialogButtonBox::Close),
			 &QPushButton::clicked, this, &OutputTimer::hide);

	streamingTimer = new QTimer(this);
	streamingTimerDisplay = new QTimer(this);

	startRecordingTimer = new QTimer(this);
	startRecordingTimerDisplay = new QTimer(this);

	recordingTimer = new QTimer(this);
	recordingTimerDisplay = new QTimer(this);

	QObject::connect(streamingTimer, &QTimer::timeout, this,
			 &OutputTimer::EventStopStreaming);
	QObject::connect(streamingTimerDisplay, &QTimer::timeout, this,
			 &OutputTimer::UpdateStreamTimerDisplay);
	QObject::connect(startRecordingTimer, &QTimer::timeout, this,
			 &OutputTimer::EventStartRecording);
	QObject::connect(startRecordingTimerDisplay, &QTimer::timeout, this,
			 &OutputTimer::UpdateStartRecordTimerDisplay);
	QObject::connect(recordingTimer, &QTimer::timeout, this,
			 &OutputTimer::EventStopRecording);
	QObject::connect(recordingTimerDisplay, &QTimer::timeout, this,
			 &OutputTimer::UpdateRecordTimerDisplay);
}

void OutputTimer::closeEvent(QCloseEvent *)
{
	obs_frontend_save();
}

void OutputTimer::StreamingTimerButton()
{
	if (!obs_frontend_streaming_active()) {
		blog(LOG_INFO, "Starting stream due to OutputTimer");
		obs_frontend_streaming_start();
	} else if (streamingAlreadyActive ||
		   !this->streamingTimer->isActive()) {
		StreamTimerStart();
		streamingAlreadyActive = false;
	} else if (obs_frontend_streaming_active()) {
		stoppingStreamTimer = true;
		StreamTimerStop();
		stoppingStreamTimer = false;
	}
}

void OutputTimer::StartRecordingTimerButton()
{
	if (!recordingAlreadyActive && !this->startRecordingTimer->isActive()) {
		StartRecordTimerStart();
	} else {
		stoppingStartRecordingTimer = true;
		StartRecordTimerStop();
		stoppingStartRecordingTimer = false;
	}
}

void OutputTimer::RecordingTimerButton()
{
	if (!obs_frontend_recording_active()) {
		blog(LOG_INFO, "Starting recording due to OutputTimer");
		obs_frontend_recording_start();
	} else if (recordingAlreadyActive ||
		   !this->recordingTimer->isActive()) {
		RecordTimerStart();
		recordingAlreadyActive = false;
	} else if (obs_frontend_recording_active()) {
		stoppingRecordingTimer = true;
		RecordTimerStop();
		stoppingRecordingTimer = false;
	}
}

void OutputTimer::StreamTimerStart()
{
	if (!isVisible() && ui->autoStartStreamTimer->isChecked() == false) {
		streamingAlreadyActive = true;
		return;
	}

	int hours = ui->streamingTimerHours->value();
	int minutes = ui->streamingTimerMinutes->value();
	int seconds = ui->streamingTimerSeconds->value();

	int total = (((hours * 3600) + (minutes * 60)) + seconds) * 1000;

	if (total == 0)
		total = 1000;

	streamingTimer->setInterval(total);
	streamingTimer->setSingleShot(true);

	streamingTimer->start();
	streamingTimerDisplay->start(1000);
	ui->outputTimerStream->setText(obs_module_text("Stop"));

	UpdateStreamTimerDisplay();

	ui->outputTimerStream->setChecked(true);
}

void OutputTimer::StartRecordTimerStart()
{
	if (!isVisible())
		return;

	int hours = ui->startRecordingTimerHours->value();
	int minutes = ui->startRecordingTimerMinutes->value();
	int seconds = ui->startRecordingTimerSeconds->value();

	int total = (((hours * 3600) + (minutes * 60)) + seconds) * 1000;

	if (total == 0)
		total = 1000;

	startRecordingTimer->setInterval(total);
	startRecordingTimer->setSingleShot(true);

	startRecordingTimer->start();
	startRecordingTimerDisplay->start(1000);
	ui->outputTimerStartRecord->setText(obs_module_text("Stop"));

	UpdateStartRecordTimerDisplay();

	ui->outputTimerStartRecord->setChecked(true);
}

void OutputTimer::RecordTimerStart()
{
	if (!isVisible() && ui->autoStartRecordTimer->isChecked() == false) {
		recordingAlreadyActive = true;
		return;
	}

	int hours = ui->recordingTimerHours->value();
	int minutes = ui->recordingTimerMinutes->value();
	int seconds = ui->recordingTimerSeconds->value();

	int total = (((hours * 3600) + (minutes * 60)) + seconds) * 1000;

	if (total == 0)
		total = 1000;

	recordingTimer->setInterval(total);
	recordingTimer->setSingleShot(true);

	recordingTimer->start();
	recordingTimerDisplay->start(1000);
	ui->outputTimerRecord->setText(obs_module_text("Stop"));

	UpdateRecordTimerDisplay();

	ui->outputTimerRecord->setChecked(true);
}

void OutputTimer::StreamTimerStop()
{
	streamingAlreadyActive = false;

	if (!isVisible() && streamingTimer->isActive() == false)
		return;

	if (streamingTimer->isActive())
		streamingTimer->stop();

	ui->outputTimerStream->setText(obs_module_text("Start"));

	if (streamingTimerDisplay->isActive())
		streamingTimerDisplay->stop();

	ui->streamTime->setText("00:00:00");
	ui->outputTimerStream->setChecked(false);
}

void OutputTimer::StartRecordTimerStop()
{
	if (!isVisible())
		return;

	if (startRecordingTimer->isActive())
		startRecordingTimer->stop();

	ui->outputTimerStartRecord->setText(obs_module_text("Start"));

	if (startRecordingTimerDisplay->isActive())
		startRecordingTimerDisplay->stop();

	ui->startRecordTime->setText("00:00:00");
	ui->outputTimerStartRecord->setChecked(false);
}

void OutputTimer::RecordTimerStop()
{
	recordingAlreadyActive = false;

	if (!isVisible() && recordingTimer->isActive() == false)
		return;

	if (recordingTimer->isActive())
		recordingTimer->stop();

	ui->outputTimerRecord->setText(obs_module_text("Start"));

	if (recordingTimerDisplay->isActive())
		recordingTimerDisplay->stop();

	ui->recordTime->setText("00:00:00");
	ui->outputTimerRecord->setChecked(false);
}

void OutputTimer::UpdateStreamTimerDisplay()
{
	int remainingTime = streamingTimer->remainingTime() / 1000;

	int seconds = remainingTime % 60;
	int minutes = (remainingTime % 3600) / 60;
	int hours = remainingTime / 3600;

	QString text =
		QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
	ui->streamTime->setText(text);
}

void OutputTimer::UpdateStartRecordTimerDisplay()
{
	int remainingTime = 0;

	remainingTime = startRecordingTimer->remainingTime() / 1000;

	int seconds = remainingTime % 60;
	int minutes = (remainingTime % 3600) / 60;
	int hours = remainingTime / 3600;

	QString text =
		QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
	ui->startRecordTime->setText(text);
}

void OutputTimer::UpdateRecordTimerDisplay()
{
	int remainingTime = 0;

	if (obs_frontend_recording_paused() &&
	    ui->pauseRecordTimer->isChecked())
		remainingTime = recordingTimeLeft / 1000;
	else
		remainingTime = recordingTimer->remainingTime() / 1000;

	int seconds = remainingTime % 60;
	int minutes = (remainingTime % 3600) / 60;
	int hours = remainingTime / 3600;

	QString text =
		QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
	ui->recordTime->setText(text);
}

void OutputTimer::PauseRecordingTimer()
{
	if (!ui->pauseRecordTimer->isChecked())
		return;

	if (recordingTimer->isActive()) {
		recordingTimeLeft = recordingTimer->remainingTime();
		recordingTimer->stop();
	}
}

void OutputTimer::UnpauseRecordingTimer()
{
	if (!ui->pauseRecordTimer->isChecked())
		return;

	if (!recordingTimer->isActive())
		recordingTimer->start(recordingTimeLeft);
}

void OutputTimer::ShowHideDialog()
{
	if (!isVisible()) {
		setVisible(true);
		QTimer::singleShot(250, this, &OutputTimer::show);
	} else {
		setVisible(false);
		QTimer::singleShot(250, this, &OutputTimer::hide);
	}
}

void OutputTimer::EventStopStreaming()
{
	if (!stoppingStreamTimer) {
		blog(LOG_INFO, "Stopping stream due to OutputTimer timeout");
		obs_frontend_streaming_stop();
	}
}

void OutputTimer::EventStartRecording()
{
	if (!stoppingStartRecordingTimer) {
		blog(LOG_INFO, "Starting recording due to OutputTimer timeout");
		obs_frontend_recording_start();
	}
}

void OutputTimer::EventStopRecording()
{
	if (!stoppingRecordingTimer) {
		blog(LOG_INFO, "Stopping recording due to OutputTimer timeout");
		obs_frontend_recording_stop();
	}
}

static void SaveOutputTimer(obs_data_t *save_data, bool saving, void *)
{
	if (saving) {
		OBSDataAutoRelease obj = obs_data_create();

		obs_data_set_int(obj, "streamTimerHours",
				 ot->ui->streamingTimerHours->value());
		obs_data_set_int(obj, "streamTimerMinutes",
				 ot->ui->streamingTimerMinutes->value());
		obs_data_set_int(obj, "streamTimerSeconds",
				 ot->ui->streamingTimerSeconds->value());

		obs_data_set_int(obj, "startRecordTimerHours",
				 ot->ui->startRecordingTimerHours->value());
		obs_data_set_int(obj, "startRecordTimerMinutes",
				 ot->ui->startRecordingTimerMinutes->value());
		obs_data_set_int(obj, "startRecordTimerSeconds",
				 ot->ui->startRecordingTimerSeconds->value());

		obs_data_set_int(obj, "recordTimerHours",
				 ot->ui->recordingTimerHours->value());
		obs_data_set_int(obj, "recordTimerMinutes",
				 ot->ui->recordingTimerMinutes->value());
		obs_data_set_int(obj, "recordTimerSeconds",
				 ot->ui->recordingTimerSeconds->value());

		obs_data_set_bool(obj, "autoStartStreamTimer",
				  ot->ui->autoStartStreamTimer->isChecked());
		obs_data_set_bool(obj, "autoStartRecordTimer",
				  ot->ui->autoStartRecordTimer->isChecked());

		obs_data_set_bool(obj, "pauseRecordTimer",
				  ot->ui->pauseRecordTimer->isChecked());

		obs_data_set_obj(save_data, "output-timer", obj);
	} else {
		OBSDataAutoRelease obj =
			obs_data_get_obj(save_data, "output-timer");

		if (!obj)
			obj = obs_data_create();

		ot->ui->streamingTimerHours->setValue(
			obs_data_get_int(obj, "streamTimerHours"));
		ot->ui->streamingTimerMinutes->setValue(
			obs_data_get_int(obj, "streamTimerMinutes"));
		ot->ui->streamingTimerSeconds->setValue(
			obs_data_get_int(obj, "streamTimerSeconds"));

		ot->ui->startRecordingTimerHours->setValue(
			obs_data_get_int(obj, "startRecordTimerHours"));
		ot->ui->startRecordingTimerMinutes->setValue(
			obs_data_get_int(obj, "startRecordTimerMinutes"));
		ot->ui->startRecordingTimerSeconds->setValue(
			obs_data_get_int(obj, "startRecordTimerSeconds"));

		ot->ui->recordingTimerHours->setValue(
			obs_data_get_int(obj, "recordTimerHours"));
		ot->ui->recordingTimerMinutes->setValue(
			obs_data_get_int(obj, "recordTimerMinutes"));
		ot->ui->recordingTimerSeconds->setValue(
			obs_data_get_int(obj, "recordTimerSeconds"));

		ot->ui->autoStartStreamTimer->setChecked(
			obs_data_get_bool(obj, "autoStartStreamTimer"));
		ot->ui->autoStartRecordTimer->setChecked(
			obs_data_get_bool(obj, "autoStartRecordTimer"));

		ot->ui->pauseRecordTimer->setChecked(
			obs_data_get_bool(obj, "pauseRecordTimer"));
	}
}

extern "C" void FreeOutputTimer() {}

static void OBSEvent(enum obs_frontend_event event, void *)
{
	if (event == OBS_FRONTEND_EVENT_EXIT) {
		obs_frontend_save();
		FreeOutputTimer();
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
		ot->StreamTimerStart();
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPING) {
		ot->StreamTimerStop();
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_STARTED) {
		ot->RecordTimerStart();
		ot->StartRecordTimerStop();
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPING) {
		ot->RecordTimerStop();
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_PAUSED) {
		ot->PauseRecordingTimer();
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_UNPAUSED) {
		ot->UnpauseRecordingTimer();
	}
}

extern "C" void InitOutputTimer()
{
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(
		obs_module_text("OutputTimer"));

	obs_frontend_push_ui_translation(obs_module_get_string);

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();

	ot = new OutputTimer(window);

	auto cb = []() {
		ot->ShowHideDialog();
	};

	obs_frontend_pop_ui_translation();

	obs_frontend_add_save_callback(SaveOutputTimer, nullptr);
	obs_frontend_add_event_callback(OBSEvent, nullptr);

	action->connect(action, &QAction::triggered, cb);
}
