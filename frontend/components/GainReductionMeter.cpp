#include "GainReductionMeter.hpp"

#include <OBSApp.hpp>

#include <cmath>

#include <QHBoxLayout>
#include <QPainter>
#include <QTimer>
#include <QVBoxLayout>

#include <util/platform.h>

#include "moc_GainReductionMeter.cpp"

GainReductionMeter::GainReductionMeter(QWidget *parent, obs_source_t *source)
	: QWidget(parent),
	  weakSource(OBSGetWeakRef(source))
{
	setFocusPolicy(Qt::NoFocus);
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	// Header row: "Gain Reduction" on the left, live dB on the right
	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 4, 0, 8);
	layout->setSpacing(4);

	auto *header = new QHBoxLayout();
	header->setContentsMargins(0, 0, 0, 0);

	auto *titleLabel = new QLabel(QTStr("Basic.Filters.GainReduction"), this);
	valueLabel = new QLabel(QStringLiteral("0.0 dB"), this);
	valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

	header->addWidget(titleLabel);
	header->addStretch();
	header->addWidget(valueLabel);
	layout->addLayout(header);
	layout->addSpacing(14);

	// Stop polling cleanly if the filter is deleted while the dialog is open
	if (source) {
		destroyedSignal = OBSSignal(obs_source_get_signal_handler(source), "destroy",
					    &GainReductionMeter::obsSourceDestroyed, this);
	}

	// ~30 Hz UI update; audio thread writes GR much faster via atomics
	auto *timer = new QTimer(this);
	timer->setTimerType(Qt::PreciseTimer);
	connect(timer, &QTimer::timeout, this, &GainReductionMeter::tick);
	timer->start(33);

	tick();
}

GainReductionMeter::~GainReductionMeter() = default;

void GainReductionMeter::obsSourceDestroyed(void *data, calldata_t *)
{
	auto *self = static_cast<GainReductionMeter *>(data);
	// Bounce to the UI thread before touching Qt widgets
	QMetaObject::invokeMethod(self, "onSourceDestroyed", Qt::QueuedConnection);
}

void GainReductionMeter::onSourceDestroyed()
{
	weakSource = nullptr;
	destroyedSignal.Disconnect();
	currentGainReductionDb = 0.0f;
	peakHoldDb = 0.0f;
	valueLabel->setText(QStringLiteral("0.0 dB"));
	update();
}

QSize GainReductionMeter::minimumSizeHint() const
{
	return QSize(120, 40);
}

QSize GainReductionMeter::sizeHint() const
{
	return QSize(200, 40);
}

float GainReductionMeter::dbToBarWidth(float gainReductionDb, int width) const
{
	if (width <= 0) {
		return 0.0f;
	}

	// 0 dB -> empty; kMinimumDb (-60) -> full width. gainReductionDb is <= 0.
	float amount = qBound(0.0f, gainReductionDb / kMinimumDb, 1.0f);
	return amount * (float)width;
}

void GainReductionMeter::pollGainReduction()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		currentGainReductionDb = 0.0f;
		return;
	}

	// Compressor registers this in compressor_create
	proc_handler_t *ph = obs_source_get_proc_handler(source);
	if (!ph) {
		currentGainReductionDb = 0.0f;
		return;
	}

	calldata_t cd = {};
	if (!proc_handler_call(ph, "get_gain_reduction", &cd)) {
		calldata_free(&cd);
		currentGainReductionDb = 0.0f;
		return;
	}

	// Negative = reduction applied; 0 = idle / below threshold
	currentGainReductionDb = (float)calldata_float(&cd, "db");
	if (!std::isfinite(currentGainReductionDb) || currentGainReductionDb > 0.0f) {
		currentGainReductionDb = 0.0f;
	} else if (currentGainReductionDb < kMinimumDb) {
		currentGainReductionDb = kMinimumDb;
	}

	calldata_free(&cd);
}

void GainReductionMeter::updatePeakHold(float gainReductionDb, uint64_t ts)
{
	const uint64_t holdNs = (uint64_t)(kPeakHoldDurationSec * 1000000000.0);

	// More reduction (more negative) always refreshes the peak
	if (gainReductionDb < peakHoldDb) {
		peakHoldDb = gainReductionDb;
		peakHoldTimeNs = ts;
		return;
	}

	// After the hold window, let the peak fall back to the live value
	if (peakHoldTimeNs == 0 || (ts - peakHoldTimeNs) >= holdNs) {
		peakHoldDb = gainReductionDb;
		peakHoldTimeNs = ts;
	}
}

void GainReductionMeter::tick()
{
	pollGainReduction();
	updatePeakHold(currentGainReductionDb, os_gettime_ns());

	// Label shows live GR only; peak is visual-only on the bar
	valueLabel->setText(QStringLiteral("%1 dB").arg(currentGainReductionDb, 0, 'f', 1));
	update();
}

void GainReductionMeter::paintEvent(QPaintEvent *)
{
	QPainter painter(this);

	const int barHeight = 10;
	const int barY = height() - barHeight - 4;
	const int barWidth = width();

	if (barWidth <= 0 || barY < 0) {
		return;
	}

	const QColor background(0x2a, 0x2a, 0x2a);
	const QColor fill(0xe6, 0xb8, 0x00);     // amber fill = live GR
	const QColor peakTick(0xff, 0xf0, 0xa0); // lighter tick = peak hold

	painter.fillRect(0, barY, barWidth, barHeight, background);

	// Live fill grows left -> right as reduction increases
	const int fillWidth = (int)dbToBarWidth(currentGainReductionDb, barWidth);
	if (fillWidth > 0) {
		painter.fillRect(0, barY, fillWidth, barHeight, fill);
	}

	// Peak tick sits at the recent maximum reduction
	const int peakX = (int)dbToBarWidth(peakHoldDb, barWidth);
	if (peakHoldDb < -0.05f && peakX > 0) {
		const int tickWidth = qMax(2, barWidth / 120);
		painter.fillRect(qMin(peakX, barWidth - tickWidth), barY, tickWidth, barHeight, peakTick);
	}
}
