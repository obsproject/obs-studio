#pragma once

#include <QDialog>
#include <QScopedPointer>
#include <memory>

#include <obs-frontend-api.h>
#include <obs.hpp>

class Ui_OutputTimer;
class QTimer;

class OutputTimer : public QDialog {
	Q_OBJECT

public:
	OutputTimer(QWidget *parent);
	~OutputTimer();

private:
	std::unique_ptr<Ui_OutputTimer> ui;

	QScopedPointer<QTimer> streamTimer;
	QScopedPointer<QTimer> recordTimer;
	QScopedPointer<QTimer> remainingTimer;

	bool streamButtonClicked = false;
	bool recordButtonClicked = false;
	int pausedTimeRemaining = 0;
	bool paused = false;

	void onActivate();
	void onDeactivate();

	void enableStreamWidgets(bool enable);
	int getStreamInterval();
	void startStreamTimer();
	void stopStreamTimer();

	void enableRecordWidgets(bool enable);
	int getRecordInterval();
	void startRecordTimer();
	void stopRecordTimer();
	void pauseRecordTimer();
	void unpauseRecordTimer();

	void onStreamStart();
	void onRecordStart();

	static void onSave(obs_data_t *saveData, bool saving, void *data);
	static void onEvent(enum obs_frontend_event event, void *data);

private slots:
	void updateLabels();

	void on_streamButton_clicked();
	void on_recordButton_clicked();
};
