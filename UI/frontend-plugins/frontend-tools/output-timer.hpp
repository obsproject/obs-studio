#pragma once

#include <QDialog>
#include <memory>

#include "ui_output-timer.h"

class QCloseEvent;

class OutputTimer : public QDialog {
	Q_OBJECT

public:
	std::unique_ptr<Ui_OutputTimer> ui;
	OutputTimer(QWidget *parent);

	void closeEvent(QCloseEvent *event) override;
	void PauseRecordingTimer();
	void UnpauseRecordingTimer();

public slots:
	void StreamingTimerButton();
	void StartRecordingTimerButton();
	void RecordingTimerButton();
	void StreamTimerStart();
	void StartRecordTimerStart();
	void RecordTimerStart();
	void StreamTimerStop();
	void StartRecordTimerStop();
	void RecordTimerStop();
	void UpdateStreamTimerDisplay();
	void UpdateStartRecordTimerDisplay();
	void UpdateRecordTimerDisplay();
	void ShowHideDialog();
	void EventStopStreaming();
	void EventStartRecording();
	void EventStopRecording();

private:
	bool streamingAlreadyActive = false;
	bool recordingAlreadyActive = false;
	bool stoppingStreamTimer = false;
	bool stoppingStartRecordingTimer = false;
	bool stoppingRecordingTimer = false;

	QTimer *streamingTimer;
	QTimer *startRecordingTimer;
	QTimer *recordingTimer;
	QTimer *streamingTimerDisplay;
	QTimer *startRecordingTimerDisplay;
	QTimer *recordingTimerDisplay;

	int recordingTimeLeft;
};
