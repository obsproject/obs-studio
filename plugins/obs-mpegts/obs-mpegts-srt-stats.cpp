/******************************************************************************
    Copyright (C) 2023 by pkv <pkv@obsproject.com>

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
#include "obs-mpegts-srt-stats.hpp"

#define TIMER_INTERVAL 1000
#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()

SRTStats *_statsFrame = nullptr;

bool WindowPositionValid(QRect rect)
{
	for (QScreen *screen : QGuiApplication::screens()) {
		if (screen->availableGeometry().intersects(rect))
			return true;
	}
	return false;
}

void setThemeID(QWidget *widget, const QString &themeID)
{
	if (widget->property("themeID").toString() != themeID) {
		widget->setProperty("themeID", themeID);

		/* force style sheet recalculation */
		QString qss = widget->styleSheet();
		widget->setStyleSheet("/* */");
		widget->setStyleSheet(qss);
	}
}

extern "C" void load_srt_stats()
{
	// initialize stats
	obs_frontend_push_ui_translation(obs_module_get_string);
	_statsFrame = new SRTStats();
	obs_frontend_pop_ui_translation();

	// add the stats dock to the docks menu
	if (!obs_frontend_add_dock_by_id(obs_module_text("SRT.Stats"),
					 obs_module_text("SRT.Stats"),
					 _statsFrame))
		blog(LOG_ERROR, "SRT Stats dock could not be created");
}

void SRTStats::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	SRTStats *stats = reinterpret_cast<SRTStats *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		stats->chart->connect();
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		stats->chart->disconnect();
		break;
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
		stats->Update();
	default:
		break;
	}
}

SRTStats::SRTStats() : timer(this)
{
	QVBoxLayout *mainLayout = new QVBoxLayout();
	outputLayout = new QGridLayout();

	/* --------------------------------------------- */
	QPushButton *resetButton =
		new QPushButton(obs_module_text("SRT.Stats.Reset"));
	QHBoxLayout *buttonLayout = new QHBoxLayout;
	buttonLayout->addStretch();
	buttonLayout->addWidget(resetButton);

	/* --------------------------------------------- */
	int col = 0;
	auto addOutputCol = [&](const char *loc) {
		QLabel *label = new QLabel(QT_UTF8(loc), this);
		label->setStyleSheet("font-weight: bold");
		outputLayout->addWidget(label, 0, col++);
	};

	addOutputCol(obs_module_text("SRT.Stats.Status"));
	addOutputCol(obs_module_text("SRT.Stats.DroppedFrames"));
	addOutputCol(obs_module_text("SRT.Stats.MegabytesSent"));
	addOutputCol(obs_module_text("SRT.Stats.Bitrate"));
	addOutputCol(obs_module_text("SRT.Stats.RTT"));
	addOutputCol(obs_module_text("SRT.Stats.Bandwidth"));

	col = 0;
	auto extraAddOutputCol = [&](const char *loc) {
		QLabel *label = new QLabel(QT_UTF8(loc), this);
		label->setStyleSheet("font-weight: bold");
		outputLayout->addWidget(label, 2, col++);
	};
	extraAddOutputCol(obs_module_text("SRT.Stats.Total.Pkts"));
	extraAddOutputCol(obs_module_text("SRT.Stats.Retransmitted.Pkts"));
	extraAddOutputCol(obs_module_text("SRT.Stats.Dropped.Pkts"));
	extraAddOutputCol(obs_module_text("SRT.Stats.Peer.Latency"));
	extraAddOutputCol(obs_module_text("SRT.Stats.Latency"));

	/* --------------------------------------------- */
	AddOutputLabels();
	/* --------------------------------------------- */
	QLabel *timeLabel = new QLabel(
		QT_UTF8(obs_module_text("SRT.Stats.Time.Selector")), this);
	timeLabel->setStyleSheet("font-weight: bold");
	outputLayout->addWidget(timeLabel, 4, 0);
	timeBox = new QComboBox();
	outputLayout->addWidget(timeBox, 4, 1);
	timeBox->addItem(QT_UTF8(obs_module_text("SRT.Stats.Time.Selector.MN")),
			 (int)SRT_CHART_TIME_MN);
	timeBox->addItem(
		QT_UTF8(obs_module_text("SRT.Stats.Time.Selector.3MN")),
		(int)SRT_CHART_TIME_3MN);
	timeBox->addItem(
		QT_UTF8(obs_module_text("SRT.Stats.Time.Selector.5MN")),
		(int)SRT_CHART_TIME_5MN);
	timeBox->addItem(
		QT_UTF8(obs_module_text("SRT.Stats.Time.Selector.10MN")),
		(int)SRT_CHART_TIME_10MN);
	timeBox->addItem(QT_UTF8(obs_module_text("SRT.Stats.Time.Selector.HR")),
			 (int)SRT_CHART_TIME_HR);
	timeBox->addItem(
		QT_UTF8(obs_module_text("SRT.Stats.Time.Selector.2HR")),
		(int)SRT_CHART_TIME_2HR);
	timeBox->addItem(
		QT_UTF8(obs_module_text("SRT.Stats.Time.Selector.4HR")),
		(int)SRT_CHART_TIME_4HR);
	QObject::connect(timeBox, SIGNAL(currentIndexChanged(int)), this,
			 SLOT(timeBoxItemChanged(int)));

	chart = new Chart;
	/* --------------------------------------------- */

	QVBoxLayout *outputContainerLayout = new QVBoxLayout();
	outputContainerLayout->addLayout(outputLayout);
	outputContainerLayout->addStretch();

	QWidget *widget = new QWidget(this);
	widget->setLayout(outputContainerLayout);
	widget->setMaximumHeight(120);

	/* --------------------------------------------- */
	// add chart and chartview
	QChartView *chartView = new QChartView(chart);
	chartView->setRenderHint(QPainter::Antialiasing);
	chartView->setMinimumSize(600, 300);
	chartView->setRubberBand(QChartView::VerticalRubberBand);
	chartView->setToolTip(
		QString::fromUtf8(obs_module_text("SRT.Stats.Tooltip")));
	chartView->setToolTipDuration(2000);

	/* --------------------------------------------- */
	mainLayout->addWidget(widget);
	mainLayout->addWidget(chartView);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	/* --------------------------------------------- */
	connect(resetButton, &QPushButton::clicked, [this]() { Reset(); });

	setWindowTitle(obs_module_text("SRT.Stats"));
#ifdef __APPLE__
	setWindowIcon(
		QIcon::fromTheme("obs", QIcon(":/res/images/obs_256x256.png")));
#else
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif

	setWindowModality(Qt::NonModal);
	setAttribute(Qt::WA_DeleteOnClose, true);

	QObject::connect(&timer, &QTimer::timeout, this, &SRTStats::Update);
	timer.setInterval(TIMER_INTERVAL);

	if (isVisible())
		timer.start();

	config_t *conf = obs_frontend_get_global_config();
	const char *geometry = config_get_string(conf, "SRTStats", "geometry");
	if (geometry != NULL) {
		QByteArray byteArray =
			QByteArray::fromBase64(QByteArray(geometry));
		restoreGeometry(byteArray);

		QRect windowGeometry = normalGeometry();
		if (!WindowPositionValid(windowGeometry)) {
			QRect rect =
				QGuiApplication::primaryScreen()->geometry();
			setGeometry(QStyle::alignedRect(Qt::LeftToRight,
							Qt::AlignCenter, size(),
							rect));
		}
	}

	obs_frontend_add_event_callback(OBSFrontendEvent, this);
}

void SRTStats::closeEvent(QCloseEvent *event)
{
	config_t *conf = obs_frontend_get_global_config();
	if (isVisible()) {
		config_set_string(conf, "SRTStats", "geometry",
				  saveGeometry().toBase64().constData());
		config_save_safe(conf, "tmp", nullptr);
	}
	QWidget::closeEvent(event);
}

SRTStats::~SRTStats()
{
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);
}

void SRTStats::AddOutputLabels()
{
	outputLabels.status = new QLabel(this);
	outputLabels.droppedFrames = new QLabel(this);
	outputLabels.megabytesSent = new QLabel(this);
	outputLabels.bitrate = new QLabel(this);
	outputLabels.srt_total_pkts = new QLabel(this);
	outputLabels.srt_retransmitted_pkts = new QLabel(this);
	outputLabels.srt_dropped_pkts = new QLabel(this);
	outputLabels.srt_rtt = new QLabel(this);
	outputLabels.srt_peer_latency = new QLabel(this);
	outputLabels.srt_latency = new QLabel(this);
	outputLabels.srt_bandwidth = new QLabel(this);

	int col = 0;
	outputLayout->addWidget(outputLabels.status, 1, col++);
	outputLayout->addWidget(outputLabels.droppedFrames, 1, col++);
	outputLayout->addWidget(outputLabels.megabytesSent, 1, col++);
	outputLayout->addWidget(outputLabels.bitrate, 1, col++);
	outputLayout->addWidget(outputLabels.srt_rtt, 1, col++);
	outputLayout->addWidget(outputLabels.srt_bandwidth, 1, col++);
	col = 0;
	outputLayout->addWidget(outputLabels.srt_total_pkts, 3, col++);
	outputLayout->addWidget(outputLabels.srt_retransmitted_pkts, 3, col++);
	outputLayout->addWidget(outputLabels.srt_dropped_pkts, 3, col++);
	outputLayout->addWidget(outputLabels.srt_peer_latency, 3, col++);
	outputLayout->addWidget(outputLabels.srt_latency, 3, col++);
}

void SRTStats::Update()
{
	OBSOutputAutoRelease strOutput = obs_frontend_get_streaming_output();
	if (!strOutput)
		return;
	/* update general stats */
	outputLabels.Update(strOutput);
	/* update srt specific stats */
	outputLabels.SRTUpdate(strOutput);
	chart->output = strOutput;
}

void SRTStats::Reset()
{
	timer.start();
	OBSOutputAutoRelease strOutput = obs_frontend_get_streaming_output();
	outputLabels.Reset(strOutput);
	Update();
	if (obs_frontend_streaming_active()) {
		chart->disconnect();
		chart->connect();
	}
}

void SRTStats::timeBoxItemChanged(int index)
{
	int time_mn = timeBox->itemData(index).toInt();
	chart->changeTimeSpan(time_mn);
}

void SRTStats::OutputLabels::Update(obs_output_t *output)
{
	uint64_t totalBytes = output ? obs_output_get_total_bytes(output) : 0;
	uint64_t curTime = os_gettime_ns();
	uint64_t bytesSent = totalBytes;

	if (bytesSent < lastBytesSent)
		bytesSent = 0;
	if (bytesSent == 0)
		lastBytesSent = 0;

	uint64_t bitsBetween = (bytesSent - lastBytesSent) * 8;
	long double timePassed =
		(long double)(curTime - lastBytesSentTime) / 1000000000.0l;
	kbps = (long double)bitsBetween / timePassed / 1000.0l;

	if (timePassed < 0.01l)
		kbps = 0.0l;

	QString str = obs_module_text("SRT.Stats.Status.Inactive");
	QString themeID;
	bool active = output ? obs_output_active(output) : false;

	if (active) {
		bool reconnecting = output ? obs_output_reconnecting(output)
					   : false;

		if (reconnecting) {
			str = obs_module_text("SRT.Stats.Status.Reconnecting");
			themeID = "error";
		} else {
			str = obs_module_text("SRT.Stats.Status.Live");
			themeID = "good";
		}
	}

	status->setText(str);
	setThemeID(status, themeID);

	long double num = (long double)totalBytes / (1024.0l * 1024.0l);

	megabytesSent->setText(
		QString("%1 MB").arg(QString::number(num, 'f', 1)));
	bitrate->setText(QString("%1 kb/s").arg(QString::number(kbps, 'f', 0)));

	int total = output ? obs_output_get_total_frames(output) : 0;
	int dropped = output ? obs_output_get_frames_dropped(output) : 0;

	if (total < first_total || dropped < first_dropped) {
		first_total = 0;
		first_dropped = 0;
	}

	total -= first_total;
	dropped -= first_dropped;

	num = total ? (long double)dropped / (long double)total * 100.0l : 0.0l;

	str = QString("%1 / %2 (%3%)")
		      .arg(QString::number(dropped), QString::number(total),
			   QString::number(num, 'f', 1));
	droppedFrames->setText(str);

	if (num > 5.0l)
		setThemeID(droppedFrames, "error");
	else if (num > 1.0l)
		setThemeID(droppedFrames, "warning");
	else
		setThemeID(droppedFrames, "");

	lastBytesSent = bytesSent;
	lastBytesSentTime = curTime;
}

void SRTStats::OutputLabels::SRTUpdate(obs_output_t *output)
{
	total_pkts = 0;
	int retransmitted_pkts = 0;
	int dropped_pkts = 0;
	float rtt = 0;
	float peer_latency = 0;
	float latency = 0;
	float bandwidth = 0;

	calldata_t cd = {0};
	proc_handler_t *ph = obs_output_get_proc_handler(output);
	proc_handler_call(ph, "get_srt_stats", &cd);
	total_pkts = (qreal)calldata_int(&cd, "srt_total_pkts");
	retransmitted_pkts = calldata_int(&cd, "srt_retransmitted_pkts");
	dropped_pkts = calldata_int(&cd, "srt_dropped_pkts");
	rtt = calldata_float(&cd, "srt_rtt");
	peer_latency = calldata_float(&cd, "srt_peer_latency");
	latency = calldata_float(&cd, "srt_latency");
	bandwidth = calldata_float(&cd, "srt_bandwidth");
	calldata_free(&cd);

	srt_total_pkts->setText(QString::number(total_pkts).append(" pkts"));
	float num = (float)retransmitted_pkts / (float)total_pkts;
	QString str = QString::number(retransmitted_pkts);
	str += QString(" pkts (%1%)").arg(QString::number(num, 'f', 1));
	srt_retransmitted_pkts->setText(str);
	num = (float)dropped_pkts / (float)total_pkts;
	str = QString::number(dropped_pkts);
	str += QString(" pkts (%1%)").arg(QString::number(num, 'f', 1));
	srt_dropped_pkts->setText(str);
	srt_rtt->setText(QString::number(rtt, 'f', 1).append(" ms"));
	srt_peer_latency->setText(
		QString::number(peer_latency, 'f', 1).append(" ms"));
	srt_latency->setText(QString::number(latency, 'f', 1).append(" ms"));
	srt_bandwidth->setText(
		QString::number(bandwidth, 'f', 1).append(" Mbps"));
}

void SRTStats::OutputLabels::Reset(obs_output_t *output)
{
	if (!output)
		return;

	first_total = obs_output_get_total_frames(output);
	first_dropped = obs_output_get_frames_dropped(output);
}

void SRTStats::showEvent(QShowEvent *)
{
	timer.start(TIMER_INTERVAL);
}

void SRTStats::hideEvent(QHideEvent *)
{
	timer.stop();
}

Chart::Chart(QGraphicsItem *parent, Qt::WindowFlags wFlags)
	: QChart(QChart::ChartTypeCartesian, parent, wFlags),
	  retrans_pkt_series(new QSplineSeries),
	  dropped_pkt_series(new QSplineSeries),
	  rtt_series(new QSplineSeries),
	  dummy_series(new QLineSeries),
	  m_axisX(new QValueAxis()),
	  m_axisY(new QValueAxis()),
	  m_axisZ(new QValueAxis()),
	  m_axisT(new QDateTimeAxis()),
	  m_x(0),
	  m_y(0),
	  m_t(0),
	  old_retrans(0),
	  old_dropped(0),
	  time_span(1),
	  output(nullptr)
{
	QPen red(Qt::red);
	QPen green(Qt::green);
	QPen blue(Qt::blue);
	green.setWidth(3);
	red.setWidth(3);
	blue.setWidth(3);
	retrans_pkt_series->setPen(red);
	dropped_pkt_series->setPen(green);
	rtt_series->setPen(blue);

	retrans_pkt_series->setName(
		obs_module_text("SRT.Stats.Retransmitted.Pkts.Rate"));
	dropped_pkt_series->setName(
		obs_module_text("SRT.Stats.Dropped.Pkts.Rate"));
	rtt_series->setName(obs_module_text("SRT.Stats.RTT"));

	addSeries(dropped_pkt_series);
	addSeries(retrans_pkt_series);
	addSeries(rtt_series);
	addSeries(dummy_series);

	addAxis(m_axisX, Qt::AlignTop);
	addAxis(m_axisY, Qt::AlignLeft);
	addAxis(m_axisZ, Qt::AlignRight);
	addAxis(m_axisT, Qt::AlignBottom);
	m_axisX->setVisible(false);

	retrans_pkt_series->attachAxis(m_axisX);
	retrans_pkt_series->attachAxis(m_axisY);
	dummy_series->attachAxis(m_axisT);
	dummy_series->attachAxis(m_axisY);
	dummy_series->hide();
	dropped_pkt_series->attachAxis(m_axisX);
	dropped_pkt_series->attachAxis(m_axisY);
	rtt_series->attachAxis(m_axisX);
	rtt_series->attachAxis(m_axisZ);

	m_axisX->setTickCount(5);
	m_axisX->setRange(0, 1);
	m_axisX->setTitleText(obs_module_text("SRT.Stats.Time"));
	m_axisY->setRange(0, 10);
	m_axisY->setTitleText(obs_module_text("SRT.Stats.Packets"));
	m_axisZ->setRange(0, 50);
	m_axisZ->setTitleText(obs_module_text("SRT.Stats.RTT.ms"));
	QBrush blueB(Qt::blue);
	m_axisZ->setTitleBrush(blueB);

	m_axisT->setTickCount(5);
	m_axisT->setFormat("hh:mm:ss");
	m_axisT->setTitleText(obs_module_text("SRT.Stats.Time.HMS"));

	QTime tm = QTime::currentTime();
	QTime tM = tm.addSecs(60);
	QDate date = QDate::currentDate();
	QDateTime min;
	min.setDate(date);
	min.setTime(tm);
	QDateTime max;
	max.setDate(date);
	max.setTime(tM);

	m_axisT->setRange(min, max);

	m_timer.start();
}

void Chart::connect()
{
	// Reset the chart when there's a stream starting.
	m_axisX->setTickCount(5);
	m_axisX->setRange(0, 60 * time_span);
	m_axisY->setRange(0, 10);
	m_axisZ->setRange(0, 50);
	m_x = 0;
	m_y = 0;
	m_t = 0;
	old_retrans = 0;
	old_dropped = 0;
	retrans_pkt_series->clear();
	dropped_pkt_series->clear();
	rtt_series->clear();
	QTime tm = QTime::currentTime();
	QTime tM = tm.addSecs(60);
	QDate date = QDate::currentDate();
	QDateTime min;
	min.setDate(date);
	min.setTime(tm);
	QDateTime max;
	max.setDate(date);
	max.setTime(tM);
	m_axisT->setRange(min, max);

	// Refresh the chart every second.
	QObject::connect(&m_timer, &QTimer::timeout, this,
			 &Chart::handleTimeout);
	m_timer.setInterval(1000);
}

void Chart::disconnect()
{
	QObject::disconnect(&m_timer, &QTimer::timeout, this,
			    &Chart::handleTimeout);
}

void Chart::changeTimeSpan(int time_mn)
{
	time_span = time_mn;
	m_axisX->setRange(m_axisX->max() - time_span * 60, m_axisX->max());
	QDateTime max = m_axisT->max();
	QDate date = QDate::currentDate();
	QDateTime min;
	min.setDate(date);
	QTime tm = max.time().addSecs(-60 * time_span);
	min.setTime(tm);
	m_axisT->setRange(min, max);
}

Chart::~Chart() {}

void Chart::handleTimeout()
{
	// The real width is divided in 60 slots per minute.
	qreal slide_x = plotArea().width() / (60 * time_span);
	qreal y = 1;
	// Retrieve data from SRT output.
	calldata_t cd = {0};
	proc_handler_t *ph = obs_output_get_proc_handler(output);
	proc_handler_call(ph, "get_srt_stats", &cd);
	qreal total_pkts = (qreal)calldata_int(&cd, "srt_total_pkts");
	UNUSED_PARAMETER(
		total_pkts); // keeping it for now because we might have to use it later
	qreal retransmitted_pkts = calldata_int(&cd, "srt_retransmitted_pkts");
	qreal dropped_pkts = calldata_int(&cd, "srt_dropped_pkts");
	qreal rtt = calldata_float(&cd, "srt_rtt");
	calldata_free(&cd);

	// Add retransmitted data to chart & auto-scale the axis.
	m_x += y;
	m_y = retransmitted_pkts - old_retrans;
	old_retrans = retransmitted_pkts;

	// auto-scale the Y axis (pkts)
	if (m_x == 1)
		m_axisY->setRange(0, qMax(qCeil(m_y / 10) * 10, 10));
	if (m_axisY->min() < 0)
		m_axisY->setRange(0, 10.0 * qCeil(m_y / 10));
	if (m_y >= 0.8 * m_axisY->max())
		m_axisY->setRange(0, qMax(2 * m_axisY->max(),
					  20.0 * qCeil(m_y / 10)));

	retrans_pkt_series->append(m_x, m_y);

	// Add dropped data to chart & auto-scale the axis.
	m_y = dropped_pkts - old_dropped;
	old_dropped = dropped_pkts;
	if (m_y >= 0.8 * m_axisY->max())
		m_axisY->setRange(0, qMax(2 * m_axisY->max(),
					  20.0 * qCeil(m_y / 10)));
	dropped_pkt_series->append(m_x, m_y);

	// Add RTT data to chart & auto-scale the axis.
	m_y = rtt;
	if (m_x == 1)
		m_axisZ->setRange(0, qCeil(m_y / 10) * 10);
	if (m_y >= 0.8 * m_axisZ->max())
		m_axisZ->setRange(0, qMax(2 * m_axisZ->max(),
					  20.0 * qCeil(m_y / 10)));

	rtt_series->append(m_x, m_y);

	// Scroll once the whole range has been drawn
	if (m_x > 60)
		scroll(slide_x, 0);
}
