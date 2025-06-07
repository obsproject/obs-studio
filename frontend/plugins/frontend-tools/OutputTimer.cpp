#include "OutputTimer.hpp"

#include <obs-module.h>
#include <qt-wrappers.hpp>

#include <QMainWindow>
#include <QPushButton>
#include <QTimer>

#include "ui_OutputTimer.h"
#include "moc_OutputTimer.cpp"

namespace {
QScopedPointer<OutputTimer> dialog;

int getInterval(int hours, int minutes, int seconds)
{
	return ((hours * 3600) + (minutes * 60) + seconds) * 1000;
}

void updateButtonState(QPushButton *button, bool active)
{
	button->setText(active ? obs_module_text("Stop") : obs_module_text("Start"));
	setClasses(button, active ? "state-active" : "");
}

QString formatTime(int ms)
{
	int remainingTime = ms / 1000;

	int seconds = remainingTime % 60;
	int minutes = (remainingTime % 3600) / 60;
	int hours = remainingTime / 3600;

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}
} // namespace

OutputTimer::OutputTimer(QWidget *parent) : QDialog(parent), ui(new Ui_OutputTimer)
{
	ui->setupUi(this);

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	obs_frontend_add_save_callback(onSave, this);
	obs_frontend_add_event_callback(onEvent, this);

	ui->streamStoppingIn->hide();
	ui->streamTimeLeft->hide();
	ui->recordStoppingIn->hide();
	ui->recordTimeLeft->hide();

	connect(ui->buttonBox->button(QDialogButtonBox::Close), &QPushButton::pressed, this, &OutputTimer::hide);
}

OutputTimer::~OutputTimer()
{
	obs_frontend_save();

	obs_frontend_remove_save_callback(onSave, this);
	obs_frontend_remove_event_callback(onEvent, this);
}

void OutputTimer::updateLabels()
{
	if (streamTimer)
		ui->streamTimeLeft->setText(formatTime(streamTimer->remainingTime()));
	else
		ui->recordTimeLeft->setText(formatTime(getStreamInterval()));

	if (recordTimer)
		ui->recordTimeLeft->setText(formatTime(paused ? pausedTimeRemaining : recordTimer->remainingTime()));
	else
		ui->recordTimeLeft->setText(formatTime(getRecordInterval()));
}

void OutputTimer::onActivate()
{
	if (streamTimer || recordTimer)
		return;

	remainingTimer.reset(new QTimer(this));
	connect(remainingTimer.data(), &QTimer::timeout, this, &OutputTimer::updateLabels);
	remainingTimer->start(500);
}

void OutputTimer::onDeactivate()
{
	if (streamTimer || recordTimer)
		return;

	remainingTimer.reset();
}

// Streaming
void OutputTimer::enableStreamWidgets(bool enable)
{
	ui->streamHours->setEnabled(enable);
	ui->streamMinutes->setEnabled(enable);
	ui->streamSeconds->setEnabled(enable);
	ui->streamAutoStart->setEnabled(enable);
}

int OutputTimer::getStreamInterval()
{
	return getInterval(ui->streamHours->value(), ui->streamMinutes->value(), ui->streamSeconds->value());
}

void OutputTimer::on_streamButton_clicked()
{
	streamButtonClicked = true;

	if (streamTimer)
		stopRecordTimer();
	else if (obs_frontend_streaming_active())
		startStreamTimer();
	else
		obs_frontend_streaming_start();

	streamButtonClicked = false;
}

void OutputTimer::startStreamTimer()
{
	if (streamTimer)
		return;

	if (!streamButtonClicked && !ui->streamAutoStart->isChecked())
		return;

	enableStreamWidgets(false);

	updateLabels();
	ui->streamStoppingIn->show();
	ui->streamTimeLeft->show();

	onActivate();

	streamTimer.reset(new QTimer(this));
	streamTimer->setSingleShot(true);
	streamTimer->setTimerType(Qt::PreciseTimer);
	connect(streamTimer.data(), &QTimer::timeout, this, []() { obs_frontend_streaming_stop(); });

	updateButtonState(ui->streamButton, true);

	int interval = getInterval(ui->streamHours->value(), ui->streamMinutes->value(), ui->streamSeconds->value());
	streamTimer->start(interval);
}

void OutputTimer::stopStreamTimer()
{
	if (!streamTimer)
		return;

	streamTimer.reset();

	updateButtonState(ui->streamButton, false);

	ui->streamStoppingIn->hide();
	ui->streamTimeLeft->hide();

	onDeactivate();

	enableStreamWidgets(true);
}

// Recording
void OutputTimer::enableRecordWidgets(bool enable)
{
	ui->recordHours->setEnabled(enable);
	ui->recordMinutes->setEnabled(enable);
	ui->recordSeconds->setEnabled(enable);
	ui->recordAutoStart->setEnabled(enable);
	ui->recordPauseTimer->setEnabled(enable);
}

int OutputTimer::getRecordInterval()
{
	return getInterval(ui->recordHours->value(), ui->recordMinutes->value(), ui->recordSeconds->value());
}

void OutputTimer::on_recordButton_clicked()
{
	recordButtonClicked = true;

	if (recordTimer)
		stopRecordTimer();
	else if (obs_frontend_recording_active())
		startRecordTimer();
	else
		obs_frontend_recording_start();

	recordButtonClicked = false;
}

void OutputTimer::startRecordTimer()
{
	if (recordTimer)
		return;

	if (!recordButtonClicked && !ui->recordAutoStart->isChecked())
		return;

	enableRecordWidgets(false);

	updateLabels();
	ui->recordStoppingIn->show();
	ui->recordTimeLeft->show();

	onActivate();

	recordTimer.reset(new QTimer(this));
	recordTimer->setSingleShot(true);
	recordTimer->setTimerType(Qt::PreciseTimer);
	connect(recordTimer.data(), &QTimer::timeout, this, []() { obs_frontend_recording_stop(); });

	updateButtonState(ui->recordButton, true);

	int interval = getInterval(ui->recordHours->value(), ui->recordMinutes->value(), ui->recordSeconds->value());
	recordTimer->start(interval);
}

void OutputTimer::stopRecordTimer()
{
	if (!recordTimer)
		return;

	recordTimer.reset();

	updateButtonState(ui->recordButton, false);

	ui->recordStoppingIn->hide();
	ui->recordTimeLeft->hide();

	paused = false;

	onDeactivate();

	enableRecordWidgets(true);
}

void OutputTimer::pauseRecordTimer()
{
	if (!recordTimer || !ui->recordPauseTimer->isChecked())
		return;

	pausedTimeRemaining = recordTimer->remainingTime();
	paused = true;

	recordTimer->stop();
}

void OutputTimer::unpauseRecordTimer()
{
	if (!recordTimer || !ui->recordPauseTimer->isChecked())
		return;

	recordTimer->start(pausedTimeRemaining);
	paused = false;
}

// Saving
void OutputTimer::onSave(obs_data_t *saveData, bool saving, void *data)
{
	OutputTimer *ot = static_cast<OutputTimer *>(data);

	if (saving) {
		OBSDataAutoRelease obj = obs_data_create();

		obs_data_set_int(obj, "streamTimerHours", ot->ui->streamHours->value());
		obs_data_set_int(obj, "streamTimerMinutes", ot->ui->streamMinutes->value());
		obs_data_set_int(obj, "streamTimerSeconds", ot->ui->streamSeconds->value());

		obs_data_set_int(obj, "recordTimerHours", ot->ui->recordHours->value());
		obs_data_set_int(obj, "recordTimerMinutes", ot->ui->recordMinutes->value());
		obs_data_set_int(obj, "recordTimerSeconds", ot->ui->recordSeconds->value());

		obs_data_set_bool(obj, "autoStartStreamTimer", ot->ui->streamAutoStart->isChecked());
		obs_data_set_bool(obj, "autoStartRecordTimer", ot->ui->recordAutoStart->isChecked());

		obs_data_set_bool(obj, "pauseRecordTimer", ot->ui->recordPauseTimer->isChecked());

		obs_data_set_obj(saveData, "output-timer", obj);
	} else {
		OBSDataAutoRelease obj = obs_data_get_obj(saveData, "output-timer");

		if (!obj)
			obj = obs_data_create();

		ot->ui->streamHours->setValue(obs_data_get_int(obj, "streamTimerHours"));
		ot->ui->streamMinutes->setValue(obs_data_get_int(obj, "streamTimerMinutes"));
		ot->ui->streamSeconds->setValue(obs_data_get_int(obj, "streamTimerSeconds"));

		ot->ui->recordHours->setValue(obs_data_get_int(obj, "recordTimerHours"));
		ot->ui->recordMinutes->setValue(obs_data_get_int(obj, "recordTimerMinutes"));
		ot->ui->recordSeconds->setValue(obs_data_get_int(obj, "recordTimerSeconds"));

		ot->ui->streamAutoStart->setChecked(obs_data_get_bool(obj, "autoStartStreamTimer"));
		ot->ui->recordAutoStart->setChecked(obs_data_get_bool(obj, "autoStartRecordTimer"));

		ot->ui->recordPauseTimer->setChecked(obs_data_get_bool(obj, "pauseRecordTimer"));
	}
}

extern "C" void freeOutputTimer()
{
	dialog.reset();
}

void OutputTimer::onEvent(enum obs_frontend_event event, void *data)
{
	OutputTimer *ot = static_cast<OutputTimer *>(data);

	switch (event) {
	case OBS_FRONTEND_EVENT_EXIT:
		freeOutputTimer();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		ot->startStreamTimer();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPING:
		ot->stopStreamTimer();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		ot->startRecordTimer();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPING:
		ot->stopRecordTimer();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_PAUSED:
		ot->pauseRecordTimer();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_UNPAUSED:
		ot->unpauseRecordTimer();
		break;
	default:
		break;
	};
}

extern "C" void initOutputTimer()
{
	QAction *action = (QAction *)obs_frontend_add_tools_menu_qaction(obs_module_text("OutputTimer"));

	obs_frontend_push_ui_translation(obs_module_get_string);

	QMainWindow *window = (QMainWindow *)obs_frontend_get_main_window();
	dialog.reset(new OutputTimer(window));

	auto cb = []() {
		dialog->show();
		dialog->raise();
	};

	obs_frontend_pop_ui_translation();

	action->connect(action, &QAction::triggered, cb);
}
