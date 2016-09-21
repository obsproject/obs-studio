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

struct TimerData {
	int streamTimerHours = 0;
	int streamTimerMinutes = 0;
	int streamTimerSeconds = 0;
	int streamTimerTotal = 0;

	int recordTimerHours = 0;
	int recordTimerMinutes = 0;
	int recordTimerSeconds = 0;
	int recordTimerTotal = 0;

	int streamTimerDisplay = 0;
	int recordTimerDisplay = 0;

	int secondsStream = 0;
	int totalMinutesStream = 0;
	int minutesStream = 0;
	int hoursStream = 0;

	int secondsRecord = 0;
	int totalMinutesRecord = 0;
	int minutesRecord = 0;
	int hoursRecord = 0;

	bool streamButtonClicked = false;
	bool recordButtonClicked = false;

	QTimer *streamingTimer;
	QTimer *recordingTimer;
	QTimer *streamingTimerDisplay;
	QTimer *recordingTimerDisplay;

	QString textStream;
	QString textRecord;
};

QMainWindow *window;
OutputTimer *ot;

static TimerData *timer = nullptr;

OutputTimer::OutputTimer(QWidget *parent)
	: QDialog(parent),
	ui(new Ui_OutputTimer)
{
	ui->setupUi(this);

	QObject::connect(ui->outputTimerStream, SIGNAL(clicked()), this,
		SLOT(StreamingTimerButton()));
	QObject::connect(ui->outputTimerRecord, SIGNAL(clicked()), this,
		SLOT(RecordingTimerButton()));
}

void OutputTimer::closeEvent(QCloseEvent*)
{
	obs_frontend_save();
}

void OutputTimer::StreamingTimerButton()
{
	if (obs_frontend_streaming_active())
		obs_frontend_streaming_stop();
	else
		obs_frontend_streaming_start();
}

void OutputTimer::RecordingTimerButton()
{
	if (obs_frontend_recording_active())
		obs_frontend_recording_stop();
	else
		obs_frontend_recording_start();
}

void OutputTimer::StreamTimerStart()
{
	timer->streamingTimer = new QTimer(this);
	timer->streamingTimerDisplay = new QTimer(this);

	if (!isVisible()) {
		ui->outputTimerStream->setEnabled(false);
		return;
	}

	timer->streamTimerHours = ui->streamingTimerHours->value();
	timer->streamTimerMinutes = ui->streamingTimerMinutes->value();
	timer->streamTimerSeconds = ui->streamingTimerSeconds->value();

	timer->streamTimerTotal = (((timer->streamTimerHours * 3600) +
		(timer->streamTimerMinutes * 60)) +
		timer->streamTimerSeconds) * 1000;

	if (timer->streamTimerTotal == 0)
		timer->streamTimerTotal = 1000;

	timer->streamTimerDisplay = timer->streamTimerTotal / 1000;

	timer->streamingTimer->setInterval(timer->streamTimerTotal);
	timer->streamingTimer->setSingleShot(true);

	QObject::connect(timer->streamingTimer, SIGNAL(timeout()),
		SLOT(EventStopStreaming()));

	QObject::connect(timer->streamingTimerDisplay, SIGNAL(timeout()), this,
		SLOT(UpdateStreamTimerDisplay()));

	timer->streamingTimer->start();
	timer->streamingTimerDisplay->start(1000);
	ui->outputTimerStream->setText(tr("Stop"));
}

void OutputTimer::RecordTimerStart()
{
	timer->recordingTimer = new QTimer(this);
	timer->recordingTimerDisplay = new QTimer(this);

	if (!isVisible()) {
		ui->outputTimerRecord->setEnabled(false);
		return;
	}

	timer->recordTimerHours = ui->recordingTimerHours->value();
	timer->recordTimerMinutes = ui->recordingTimerMinutes->value();
	timer->recordTimerSeconds = ui->recordingTimerSeconds->value();

	timer->recordTimerTotal = (((timer->recordTimerHours * 3600) +
			(timer->recordTimerMinutes * 60)) +
			timer->recordTimerSeconds) * 1000;

	if (timer->recordTimerTotal == 0)
		timer->recordTimerTotal = 1000;

	timer->recordTimerDisplay = timer->recordTimerTotal / 1000;

	timer->recordingTimer->setInterval(timer->recordTimerTotal);
	timer->recordingTimer->setSingleShot(true);

	QObject::connect(timer->recordingTimer, SIGNAL(timeout()),
		SLOT(EventStopRecording()));

	QObject::connect(timer->recordingTimerDisplay, SIGNAL(timeout()), this,
		SLOT(UpdateRecordTimerDisplay()));

	timer->recordingTimer->start();
	timer->recordingTimerDisplay->start(1000);
	ui->outputTimerRecord->setText(tr("Stop"));
}

void OutputTimer::StreamTimerStop()
{
	ui->outputTimerStream->setEnabled(true);

	if (!isVisible() && timer->streamingTimer->isActive() == false)
		return;

	if (timer->streamingTimer->isActive())
		timer->streamingTimer->stop();

	ui->outputTimerStream->setText(tr("Start"));

	if (timer->streamingTimerDisplay->isActive())
		timer->streamingTimerDisplay->stop();

	ui->streamTime->setText("00:00:00");
}

void OutputTimer::RecordTimerStop()
{
	ui->outputTimerRecord->setEnabled(true);

	if (!isVisible() && timer->recordingTimer->isActive() == false)
		return;

	if (timer->recordingTimer->isActive())
		timer->recordingTimer->stop();

	ui->outputTimerRecord->setText(tr("Start"));

	if (timer->recordingTimerDisplay->isActive())
		timer->recordingTimerDisplay->stop();

	ui->recordTime->setText("00:00:00");
}

void OutputTimer::UpdateStreamTimerDisplay()
{
	timer->streamTimerDisplay--;

	timer->secondsStream      = timer->streamTimerDisplay % 60;
	timer->totalMinutesStream = timer->streamTimerDisplay / 60;
	timer->minutesStream      = timer->totalMinutesStream % 60;
	timer->hoursStream        = timer->totalMinutesStream / 60;

	timer->textStream.sprintf("%02d:%02d:%02d",
		timer->hoursStream, timer->minutesStream, timer->secondsStream);
	ui->streamTime->setText(timer->textStream);
}

void OutputTimer::UpdateRecordTimerDisplay()
{
	timer->recordTimerDisplay--;

	timer->secondsRecord      = timer->recordTimerDisplay % 60;
	timer->totalMinutesRecord = timer->recordTimerDisplay / 60;
	timer->minutesRecord      = timer->totalMinutesRecord % 60;
	timer->hoursRecord        = timer->totalMinutesRecord / 60;

	timer->textRecord.sprintf("%02d:%02d:%02d",
		timer->hoursRecord, timer->minutesRecord, timer->secondsRecord);
	ui->recordTime->setText(timer->textRecord);
}

void OutputTimer::ShowHideDialog()
{
	if (!isVisible()) {
		setVisible(true);
		QTimer::singleShot(250, this, SLOT(show()));
	} else {
		setVisible(false);
		QTimer::singleShot(250, this, SLOT(hide()));
	}
}

void OutputTimer::EventStopStreaming()
{
	obs_frontend_streaming_stop();
}

void OutputTimer::EventStopRecording()
{
	obs_frontend_recording_stop();
}

static void SaveOutputTimer(obs_data_t *save_data, bool saving, void *)
{
	if (saving) {
		obs_data_t *obj = obs_data_create();

		obs_data_set_int(obj, "streamTimerHours",
				ot->ui->streamingTimerHours->value());
		obs_data_set_int(obj, "streamTimerMinutes",
				ot->ui->streamingTimerMinutes->value());
		obs_data_set_int(obj, "streamTimerSeconds",
				ot->ui->streamingTimerSeconds->value());

		obs_data_set_int(obj, "recordTimerHours",
				ot->ui->recordingTimerHours->value());
		obs_data_set_int(obj, "recordTimerMinutes",
				ot->ui->recordingTimerMinutes->value());
		obs_data_set_int(obj, "recordTimerSeconds",
				ot->ui->recordingTimerSeconds->value());

		obs_data_set_obj(save_data, "output-timer", obj);

		obs_data_release(obj);
	} else {
		obs_data_t *obj = obs_data_get_obj(save_data,
				"output-timer");

		if (!obj)
			obj = obs_data_create();

		ot->ui->streamingTimerHours->setValue(
				obs_data_get_int(obj, "streamTimerHours"));
		ot->ui->streamingTimerMinutes->setValue(
				obs_data_get_int(obj, "streamTimerMinutes"));
		ot->ui->streamingTimerSeconds->setValue(
				obs_data_get_int(obj, "streamTimerSeconds"));

		ot->ui->recordingTimerHours->setValue(
				obs_data_get_int(obj, "recordTimerHours"));
		ot->ui->recordingTimerMinutes->setValue(
				obs_data_get_int(obj, "recordTimerMinutes"));
		ot->ui->recordingTimerSeconds->setValue(
				obs_data_get_int(obj, "recordTimerSeconds"));

		obs_data_release(obj);
	}
}

extern "C" void FreeOutputTimer()
{
	delete timer;
	timer = nullptr;
}

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
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPING) {
		ot->RecordTimerStop();
	}
}

extern "C" void InitOutputTimer()
{
	QAction *action = (QAction*)obs_frontend_add_tools_menu_qaction(
			obs_module_text("OutputTimer"));

	timer = new TimerData;

	obs_frontend_push_ui_translation(obs_module_get_string);

	window = (QMainWindow*)obs_frontend_get_main_window();

	ot = new OutputTimer(window);

	auto cb = [] ()
	{
		ot->ShowHideDialog();
	};

	obs_frontend_pop_ui_translation();

	obs_frontend_add_save_callback(SaveOutputTimer, nullptr);
	obs_frontend_add_event_callback(OBSEvent, nullptr);

	action->connect(action, &QAction::triggered, cb);
}
