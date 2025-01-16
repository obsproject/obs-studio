#include "OBSBasicStats.hpp"

#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include "moc_OBSBasicStats.cpp"

#define TIMER_INTERVAL 2000
#define REC_TIME_LEFT_INTERVAL 30000

void OBSBasicStats::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	OBSBasicStats *stats = reinterpret_cast<OBSBasicStats *>(ptr);

	switch (event) {
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		stats->StartRecTimeLeft();
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		stats->ResetRecTimeLeft();
		break;
	case OBS_FRONTEND_EVENT_EXIT:
		// This is only reached when the non-closable (dock) stats
		// window is being cleaned up. The closable stats window is
		// already gone by this point as it's deleted on close.
		obs_frontend_remove_event_callback(OBSFrontendEvent, stats);
		break;
	default:
		break;
	}
}

static QString MakeTimeLeftText(int hours, int minutes)
{
	return QString::asprintf("%d %s, %d %s", hours, Str("Hours"), minutes, Str("Minutes"));
}

static QString MakeMissedFramesText(uint32_t total_lagged, uint32_t total_rendered, long double num)
{
	return QString("%1 / %2 (%3%)")
		.arg(QString::number(total_lagged), QString::number(total_rendered), QString::number(num, 'f', 1));
}

OBSBasicStats::OBSBasicStats(QWidget *parent, bool closable)
	: QFrame(parent),
	  cpu_info(os_cpu_usage_info_start()),
	  timer(this),
	  recTimeLeft(this)
{
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QGridLayout *topLayout = new QGridLayout();
	outputLayout = new QGridLayout();

	bitrates.reserve(REC_TIME_LEFT_INTERVAL / TIMER_INTERVAL);

	int row = 0;

	auto newStatBare = [&](QString name, QWidget *label, int col) {
		QLabel *typeLabel = new QLabel(name, this);
		topLayout->addWidget(typeLabel, row, col);
		topLayout->addWidget(label, row++, col + 1);
	};

	auto newStat = [&](const char *strLoc, QWidget *label, int col) {
		std::string str = "Basic.Stats.";
		str += strLoc;
		newStatBare(QTStr(str.c_str()), label, col);
	};

	/* --------------------------------------------- */

	cpuUsage = new QLabel(this);
	hddSpace = new QLabel(this);
	recordTimeLeft = new QLabel(this);
	memUsage = new QLabel(this);

	QString str = MakeTimeLeftText(99999, 59);
	int textWidth = recordTimeLeft->fontMetrics().boundingRect(str).width();
	recordTimeLeft->setMinimumWidth(textWidth);

	newStat("CPUUsage", cpuUsage, 0);
	newStat("HDDSpaceAvailable", hddSpace, 0);
	newStat("DiskFullIn", recordTimeLeft, 0);
	newStat("MemoryUsage", memUsage, 0);

	fps = new QLabel(this);
	renderTime = new QLabel(this);
	skippedFrames = new QLabel(this);
	missedFrames = new QLabel(this);

	str = MakeMissedFramesText(999999, 999999, 99.99);
	textWidth = missedFrames->fontMetrics().boundingRect(str).width();
	missedFrames->setMinimumWidth(textWidth);

	row = 0;

	newStatBare("FPS", fps, 2);
	newStat("AverageTimeToRender", renderTime, 2);
	newStat("MissedFrames", missedFrames, 2);
	newStat("SkippedFrames", skippedFrames, 2);

	/* --------------------------------------------- */
	QPushButton *closeButton = nullptr;
	if (closable)
		closeButton = new QPushButton(QTStr("Close"));
	QPushButton *resetButton = new QPushButton(QTStr("Reset"));
	QHBoxLayout *buttonLayout = new QHBoxLayout;
	buttonLayout->addStretch();
	buttonLayout->addWidget(resetButton);
	if (closable)
		buttonLayout->addWidget(closeButton);

	/* --------------------------------------------- */

	int col = 0;
	auto addOutputCol = [&](const char *loc) {
		QLabel *label = new QLabel(QTStr(loc), this);
		label->setStyleSheet("font-weight: bold");
		outputLayout->addWidget(label, 0, col++);
	};

	addOutputCol("Basic.Settings.Output");
	addOutputCol("Basic.Stats.Status");
	addOutputCol("Basic.Stats.DroppedFrames");
	addOutputCol("Basic.Stats.MegabytesSent");
	addOutputCol("Basic.Stats.Bitrate");

	/* --------------------------------------------- */

	AddOutputLabels(QTStr("Basic.Stats.Output.Stream"));
	AddOutputLabels(QTStr("Basic.Stats.Output.Recording"));

	/* --------------------------------------------- */

	QVBoxLayout *outputContainerLayout = new QVBoxLayout();
	outputContainerLayout->addLayout(outputLayout);
	outputContainerLayout->addStretch();

	QWidget *widget = new QWidget(this);
	widget->setLayout(outputContainerLayout);

	QScrollArea *scrollArea = new QScrollArea(this);
	scrollArea->setWidget(widget);
	scrollArea->setWidgetResizable(true);

	/* --------------------------------------------- */

	mainLayout->addLayout(topLayout);
	mainLayout->addWidget(scrollArea);
	mainLayout->addLayout(buttonLayout);
	setLayout(mainLayout);

	/* --------------------------------------------- */
	if (closable)
		connect(closeButton, &QPushButton::clicked, [this]() { close(); });
	connect(resetButton, &QPushButton::clicked, [this]() { Reset(); });

	delete shortcutFilter;
	shortcutFilter = CreateShortcutFilter();
	installEventFilter(shortcutFilter);

	resize(800, 280);

	setWindowTitle(QTStr("Basic.Stats"));
#ifdef __APPLE__
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs_256x256.png")));
#else
	setWindowIcon(QIcon::fromTheme("obs", QIcon(":/res/images/obs.png")));
#endif

	setWindowModality(Qt::NonModal);
	setAttribute(Qt::WA_DeleteOnClose, true);

	QObject::connect(&timer, &QTimer::timeout, this, &OBSBasicStats::Update);
	timer.setInterval(TIMER_INTERVAL);

	if (isVisible())
		timer.start();

	Update();

	QObject::connect(&recTimeLeft, &QTimer::timeout, this, &OBSBasicStats::RecordingTimeLeft);
	recTimeLeft.setInterval(REC_TIME_LEFT_INTERVAL);

	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	const char *geometry = config_get_string(main->Config(), "Stats", "geometry");
	if (geometry != NULL) {
		QByteArray byteArray = QByteArray::fromBase64(QByteArray(geometry));
		restoreGeometry(byteArray);

		QRect windowGeometry = normalGeometry();
		if (!WindowPositionValid(windowGeometry)) {
			QRect rect = QGuiApplication::primaryScreen()->geometry();
			setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), rect));
		}
	}

	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	if (obs_frontend_recording_active())
		StartRecTimeLeft();
}

void OBSBasicStats::closeEvent(QCloseEvent *event)
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());
	if (isVisible()) {
		config_set_string(main->Config(), "Stats", "geometry", saveGeometry().toBase64().constData());
		config_save_safe(main->Config(), "tmp", nullptr);
	}

	// This code is only reached when the non-dockable stats window is
	// manually closed or OBS is exiting.
	obs_frontend_remove_event_callback(OBSFrontendEvent, this);

	QWidget::closeEvent(event);
}

OBSBasicStats::~OBSBasicStats()
{
	delete shortcutFilter;
	os_cpu_usage_info_destroy(cpu_info);
}

void OBSBasicStats::AddOutputLabels(QString name)
{
	OutputLabels ol;
	ol.name = new QLabel(name, this);
	ol.status = new QLabel(this);
	ol.droppedFrames = new QLabel(this);
	ol.megabytesSent = new QLabel(this);
	ol.bitrate = new QLabel(this);

	int col = 0;
	int row = outputLabels.size() + 1;
	outputLayout->addWidget(ol.name, row, col++);
	outputLayout->addWidget(ol.status, row, col++);
	outputLayout->addWidget(ol.droppedFrames, row, col++);
	outputLayout->addWidget(ol.megabytesSent, row, col++);
	outputLayout->addWidget(ol.bitrate, row, col++);
	outputLabels.push_back(ol);
}

static uint32_t first_encoded = 0xFFFFFFFF;
static uint32_t first_skipped = 0xFFFFFFFF;
static uint32_t first_rendered = 0xFFFFFFFF;
static uint32_t first_lagged = 0xFFFFFFFF;

void OBSBasicStats::InitializeValues()
{
	video_t *video = obs_get_video();
	first_encoded = video_output_get_total_frames(video);
	first_skipped = video_output_get_skipped_frames(video);
	first_rendered = obs_get_total_frames();
	first_lagged = obs_get_lagged_frames();
}

void OBSBasicStats::Update()
{
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	/* TODO: Un-hardcode */

	struct obs_video_info ovi = {};
	obs_get_video_info(&ovi);

	OBSOutputAutoRelease strOutput = obs_frontend_get_streaming_output();
	OBSOutputAutoRelease recOutput = obs_frontend_get_recording_output();

	if (!strOutput && !recOutput)
		return;

	/* ------------------------------------------- */
	/* general usage                               */

	double curFPS = obs_get_active_fps();
	double obsFPS = (double)ovi.fps_num / (double)ovi.fps_den;

	QString str = QString::number(curFPS, 'f', 2);
	fps->setText(str);

	if (curFPS < (obsFPS * 0.8))
		setClasses(fps, "text-danger");
	else if (curFPS < (obsFPS * 0.95))
		setClasses(fps, "text-warning");
	else
		setClasses(fps, "");

	/* ------------------ */

	double usage = os_cpu_usage_info_query(cpu_info);
	str = QString::number(usage, 'g', 2) + QStringLiteral("%");
	cpuUsage->setText(str);

	/* ------------------ */

	const char *path = main->GetCurrentOutputPath();

#define MBYTE (1024ULL * 1024ULL)
#define GBYTE (1024ULL * 1024ULL * 1024ULL)
#define TBYTE (1024ULL * 1024ULL * 1024ULL * 1024ULL)
	num_bytes = os_get_free_disk_space(path);
	QString abrv = QStringLiteral(" MB");
	long double num;

	num = (long double)num_bytes / (1024.0l * 1024.0l);
	if (num_bytes > TBYTE) {
		num /= 1024.0l * 1024.0l;
		abrv = QStringLiteral(" TB");
	} else if (num_bytes > GBYTE) {
		num /= 1024.0l;
		abrv = QStringLiteral(" GB");
	}

	str = QString::number(num, 'f', 1) + abrv;
	hddSpace->setText(str);

	if (num_bytes < GBYTE)
		setClasses(hddSpace, "text-danger");
	else if (num_bytes < (5 * GBYTE))
		setClasses(hddSpace, "text-warning");
	else
		setClasses(hddSpace, "");

	/* ------------------ */

	num = (long double)os_get_proc_resident_size() / (1024.0l * 1024.0l);

	str = QString::number(num, 'f', 1) + QStringLiteral(" MB");
	memUsage->setText(str);

	/* ------------------ */

	num = (long double)obs_get_average_frame_time_ns() / 1000000.0l;

	str = QString::number(num, 'f', 1) + QStringLiteral(" ms");
	renderTime->setText(str);

	long double fpsFrameTime = (long double)ovi.fps_den * 1000.0l / (long double)ovi.fps_num;

	if (num > fpsFrameTime)
		setClasses(renderTime, "text-danger");
	else if (num > fpsFrameTime * 0.75l)
		setClasses(renderTime, "text-warning");
	else
		setClasses(renderTime, "");

	/* ------------------ */

	video_t *video = obs_get_video();
	uint32_t total_encoded = video_output_get_total_frames(video);
	uint32_t total_skipped = video_output_get_skipped_frames(video);

	if (total_encoded < first_encoded || total_skipped < first_skipped) {
		first_encoded = total_encoded;
		first_skipped = total_skipped;
	}
	total_encoded -= first_encoded;
	total_skipped -= first_skipped;

	num = total_encoded ? (long double)total_skipped / (long double)total_encoded : 0.0l;
	num *= 100.0l;

	str = QString("%1 / %2 (%3%)")
		      .arg(QString::number(total_skipped), QString::number(total_encoded),
			   QString::number(num, 'f', 1));
	skippedFrames->setText(str);

	if (num > 5.0l)
		setClasses(skippedFrames, "text-danger");
	else if (num > 1.0l)
		setClasses(skippedFrames, "text-warning");
	else
		setClasses(skippedFrames, "");

	/* ------------------ */

	uint32_t total_rendered = obs_get_total_frames();
	uint32_t total_lagged = obs_get_lagged_frames();

	if (total_rendered < first_rendered || total_lagged < first_lagged) {
		first_rendered = total_rendered;
		first_lagged = total_lagged;
	}
	total_rendered -= first_rendered;
	total_lagged -= first_lagged;

	num = total_rendered ? (long double)total_lagged / (long double)total_rendered : 0.0l;
	num *= 100.0l;

	str = MakeMissedFramesText(total_lagged, total_rendered, num);
	missedFrames->setText(str);

	if (num > 5.0l)
		setClasses(missedFrames, "text-danger");
	else if (num > 1.0l)
		setClasses(missedFrames, "text-warning");
	else
		setClasses(missedFrames, "");

	/* ------------------------------------------- */
	/* recording/streaming stats                   */

	outputLabels[0].Update(strOutput, false);
	outputLabels[1].Update(recOutput, true);

	if (obs_output_active(recOutput)) {
		long double kbps = outputLabels[1].kbps;
		bitrates.push_back(kbps);
	}
}

void OBSBasicStats::StartRecTimeLeft()
{
	if (recTimeLeft.isActive())
		ResetRecTimeLeft();

	recordTimeLeft->setText(QTStr("Calculating"));
	recTimeLeft.start();
}

void OBSBasicStats::ResetRecTimeLeft()
{
	if (recTimeLeft.isActive()) {
		bitrates.clear();
		recTimeLeft.stop();
		recordTimeLeft->setText(QTStr(""));
	}
}

void OBSBasicStats::RecordingTimeLeft()
{
	if (bitrates.empty())
		return;

	long double averageBitrate = accumulate(bitrates.begin(), bitrates.end(), 0.0) / (long double)bitrates.size();
	if (averageBitrate == 0)
		return;

	long double bytesPerSec = (averageBitrate / 8.0l) * 1000.0l;
	long double secondsUntilFull = (long double)num_bytes / bytesPerSec;

	bitrates.clear();

	int totalMinutes = (int)secondsUntilFull / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text = MakeTimeLeftText(hours, minutes);
	recordTimeLeft->setText(text);
	recordTimeLeft->setMinimumWidth(recordTimeLeft->width());
}

void OBSBasicStats::Reset()
{
	timer.start();

	first_encoded = 0xFFFFFFFF;
	first_skipped = 0xFFFFFFFF;
	first_rendered = 0xFFFFFFFF;
	first_lagged = 0xFFFFFFFF;

	OBSOutputAutoRelease strOutput = obs_frontend_get_streaming_output();
	OBSOutputAutoRelease recOutput = obs_frontend_get_recording_output();

	outputLabels[0].Reset(strOutput);
	outputLabels[1].Reset(recOutput);
	Update();
}

void OBSBasicStats::OutputLabels::Update(obs_output_t *output, bool rec)
{
	uint64_t totalBytes = output ? obs_output_get_total_bytes(output) : 0;
	uint64_t curTime = os_gettime_ns();
	uint64_t bytesSent = totalBytes;

	if (bytesSent < lastBytesSent)
		bytesSent = 0;
	if (bytesSent == 0)
		lastBytesSent = 0;

	uint64_t bitsBetween = (bytesSent - lastBytesSent) * 8;
	long double timePassed = (long double)(curTime - lastBytesSentTime) / 1000000000.0l;
	kbps = (long double)bitsBetween / timePassed / 1000.0l;

	if (timePassed < 0.01l)
		kbps = 0.0l;

	QString str = QTStr("Basic.Stats.Status.Inactive");
	QString styling;
	bool active = output ? obs_output_active(output) : false;
	if (rec) {
		if (active)
			str = QTStr("Basic.Stats.Status.Recording");
	} else {
		if (active) {
			bool reconnecting = output ? obs_output_reconnecting(output) : false;

			if (reconnecting) {
				str = QTStr("Basic.Stats.Status.Reconnecting");
				styling = "text-danger";
			} else {
				str = QTStr("Basic.Stats.Status.Live");
				styling = "text-success";
			}
		}
	}

	status->setText(str);
	setClasses(status, styling);

	long double num = (long double)totalBytes / (1024.0l * 1024.0l);
	const char *unit = "MiB";
	if (num > 1024) {
		num /= 1024;
		unit = "GiB";
	}
	megabytesSent->setText(QString("%1 %2").arg(num, 0, 'f', 1).arg(unit));

	num = kbps;
	unit = "kb/s";
	if (num >= 10'000) {
		num /= 1000;
		unit = "Mb/s";
	}
	bitrate->setText(QString("%1 %2").arg(num, 0, 'f', 0).arg(unit));

	if (!rec) {
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
			      .arg(QString::number(dropped), QString::number(total), QString::number(num, 'f', 1));
		droppedFrames->setText(str);

		if (num > 5.0l)
			setClasses(droppedFrames, "text-danger");
		else if (num > 1.0l)
			setClasses(droppedFrames, "text-warning");
		else
			setClasses(droppedFrames, "");
	}

	lastBytesSent = bytesSent;
	lastBytesSentTime = curTime;
}

void OBSBasicStats::OutputLabels::Reset(obs_output_t *output)
{
	if (!output)
		return;

	first_total = obs_output_get_total_frames(output);
	first_dropped = obs_output_get_frames_dropped(output);
}

void OBSBasicStats::showEvent(QShowEvent *)
{
	timer.start(TIMER_INTERVAL);
}

void OBSBasicStats::hideEvent(QHideEvent *)
{
	timer.stop();
}
