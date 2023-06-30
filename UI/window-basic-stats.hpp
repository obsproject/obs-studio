#pragma once

#include <obs.hpp>
#include <util/platform.h>
#include <obs-frontend-api.h>
#include <QPointer>
#include <QWidget>
#include <QTimer>
#include <QLabel>
#include <QList>

#include <QChart>
#include <QChartView>
#include <QSplineSeries>
#include <QValueAxis>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QDateTimeAxis>
#include <QTimeZone>
#include <QComboBox>
#include <QGroupBox>

class QGridLayout;
class QCloseEvent;

enum OBSCHART_TIME_SPAN {
	OBSCHART_TIME_MN = 1,
	OBSCHART_TIME_3MN = 3,
	OBSCHART_TIME_5MN = 5,
	OBSCHART_TIME_10MN = 10,
	OBSCHART_TIME_HR = 60,
	OBSCHART_TIME_2HR = 120,
	OBSCHART_TIME_4HR = 240
};

class OBSStatsChart : public QChart {
	Q_OBJECT

public:
	OBSStatsChart(QGraphicsItem *parent = nullptr,
		      Qt::WindowFlags wFlags = {});
	virtual ~OBSStatsChart();
	QValueAxis *m_axisX;    // dummy time axis used for scrolling
	QValueAxis *m_axisY;    // bitrate
	QValueAxis *m_axisZ;    // dropped frames
	QDateTimeAxis *m_axisT; // time axis in QDateTime format
	QSplineSeries *bitrate_series = nullptr;
	QSplineSeries *dropped_series = nullptr;
	QLineSeries *dummy_series = nullptr;
	obs_output_t *output;
	uint time_span; // horizontal axis width in minutes
	uint64_t lastBytesSent = 0;
	uint64_t lastBytesSentTime = 0;
	int first_total = 0;
	int first_dropped = 0;
	long double kbps = 0.0l;

public slots:
	void handleTimeout();
	void connect();
	void disconnect();
	void changeTimeSpan(int time);

private:
	QTimer m_timer;
	qreal m_x;
	qreal m_y;
	qreal m_t;
};

class OBSBasicStats : public QFrame {
	Q_OBJECT

	QLabel *fps = nullptr;
	QLabel *cpuUsage = nullptr;
	QLabel *hddSpace = nullptr;
	QLabel *recordTimeLeft = nullptr;
	QLabel *memUsage = nullptr;

	QLabel *renderTime = nullptr;
	QLabel *skippedFrames = nullptr;
	QLabel *missedFrames = nullptr;

	QGridLayout *outputLayout = nullptr;
	QGroupBox *timeGroupBox = nullptr;

	os_cpu_usage_info_t *cpu_info = nullptr;

	QTimer timer;
	QTimer recTimeLeft;
	uint64_t num_bytes = 0;
	std::vector<long double> bitrates;

	OBSStatsChart *chart = nullptr;
	QChartView *chartView = nullptr;
	QComboBox *timeBox = nullptr;

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

		void Update(obs_output_t *output, bool rec);
		void Reset(obs_output_t *output);

		long double kbps = 0.0l;
	};

	QList<OutputLabels> outputLabels;

	void AddOutputLabels(QString name);
	void Update();

	virtual void closeEvent(QCloseEvent *event) override;

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

public:
	OBSBasicStats(QWidget *parent = nullptr, bool closable = true);
	~OBSBasicStats();

	static void InitializeValues();

	void StartRecTimeLeft();
	void ResetRecTimeLeft();

private:
	QPointer<QObject> shortcutFilter;

private slots:
	void RecordingTimeLeft();
	void timeBoxItemChanged(int index);

public slots:
	void Reset();
	void ToggleGraphs();

protected:
	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;
};
