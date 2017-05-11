#pragma once

#include <obs.hpp>
#include <util/platform.h>
#include <QPointer>
#include <QDialog>
#include <QTimer>
#include <QLabel>
#include <QList>

class QGridLayout;

class OBSBasicStats : public QDialog {
	Q_OBJECT

	QLabel *fps = nullptr;
	QLabel *cpuUsage = nullptr;
	QLabel *hddSpace = nullptr;
	QLabel *memUsage = nullptr;

	QLabel *renderTime = nullptr;
	QLabel *skippedFrames = nullptr;
	QLabel *missedFrames = nullptr;

	QGridLayout *outputLayout = nullptr;

	os_cpu_usage_info_t *cpu_info = nullptr;

	QTimer timer;

	struct OutputLabels {
		QPointer<QLabel> name;
		QPointer<QLabel> status;
		QPointer<QLabel> droppedFrames;
		QPointer<QLabel> megabytesSent;
		QPointer<QLabel> bitrate;

		uint64_t lastBytesSent = 0;
		uint64_t lastBytesSentTime = 0;

		int first_total = 0;
		int first_dropped = 0;

		void Update(obs_output_t *output);
		void Reset(obs_output_t *output);
	};

	QList<OutputLabels> outputLabels;

	void AddOutputLabels(QString name);
	void Update();
	void Reset();

public:
	OBSBasicStats(QWidget *parent = nullptr);
	~OBSBasicStats();
};
