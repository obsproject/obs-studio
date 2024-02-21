#pragma once
#include <obs-module.h>
#include <obs-frontend-api.h>
#include <obs.hpp>
#include <util/platform.h>
#include <util/config-file.h>
#include <string>

#include <QPointer>
#include <QWidget>
#include <QComboBox>
#include <QStyle>
#include <QTimer>
#include <QLabel>
#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QScreen>
#include <QGuiApplication>
#include <QPushButton>

#include <QChart>
#include <QChartView>
#include <QSplineSeries>
#include <QValueAxis>
#include <QDate>
#include <QDateTime>
#include <QTime>
#include <QDateTimeAxis>

enum SRT_CHART_TIME {
	SRT_CHART_TIME_MN = 1,
	SRT_CHART_TIME_3MN = 3,
	SRT_CHART_TIME_5MN = 5,
	SRT_CHART_TIME_10MN = 10,
	SRT_CHART_TIME_HR = 60,
	SRT_CHART_TIME_2HR = 120,
	SRT_CHART_TIME_4HR = 240
};

class Chart : public QChart {
	Q_OBJECT

public:
	Chart(QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = {});
	virtual ~Chart();
	QValueAxis *m_axisX;    // dummy time axis used for scrolling
	QValueAxis *m_axisY;    // packets per sec
	QValueAxis *m_axisZ;    // vertical axis for RTT
	QDateTimeAxis *m_axisT; // time axis in QDateTime format
	QSplineSeries *retrans_pkt_series = nullptr;
	QSplineSeries *dropped_pkt_series = nullptr;
	QSplineSeries *rtt_series = nullptr;
	QLineSeries *dummy_series = nullptr;
	obs_output_t *output;
	uint time_span; // horizontal axis width in minutes

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
	qreal old_retrans;
	qreal old_dropped;
};

class SRTStats : public QFrame {
	Q_OBJECT

	QGridLayout *outputLayout = nullptr;
	QWidget *srtWidget = nullptr;
	Chart *chart = nullptr;
	QChartView *chartView = nullptr;
	QComboBox *timeBox = nullptr;
	QTimer timer;
	uint64_t num_bytes = 0;
	std::vector<long double> bitrates;

	struct OutputLabels {
		QPointer<QLabel> status;
		QPointer<QLabel> droppedFrames;
		QPointer<QLabel> megabytesSent;
		QPointer<QLabel> bitrate;

		QPointer<QLabel> srt_total_pkts;
		QPointer<QLabel> srt_retransmitted_pkts;
		QPointer<QLabel> srt_dropped_pkts;
		QPointer<QLabel> srt_rtt;
		QPointer<QLabel> srt_peer_latency;
		QPointer<QLabel> srt_latency;
		QPointer<QLabel> srt_bandwidth;
		qreal total_pkts;

		/*------------------------------------------*/

		uint64_t lastBytesSent = 0;
		uint64_t lastBytesSentTime = 0;

		int first_total = 0;
		int first_dropped = 0;
		/* Update: general stats; SRTUpdate: SRT stats */
		void Update(obs_output_t *output);
		void SRTUpdate(obs_output_t *output);
		void Reset(obs_output_t *output);

		long double kbps = 0.0l;
	};

	OutputLabels outputLabels;

	void AddOutputLabels();
	void Update();

	virtual void closeEvent(QCloseEvent *event) override;

	static void OBSFrontendEvent(enum obs_frontend_event event, void *ptr);

	void Reset();

public:
	SRTStats();
	~SRTStats();

private Q_SLOTS:
	void timeBoxItemChanged(int index);

protected:
	virtual void showEvent(QShowEvent *event) override;
	virtual void hideEvent(QHideEvent *event) override;
};
