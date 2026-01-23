#include "VolumeMeter.hpp"

#include <OBSApp.hpp>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStyleOption>
#include <QTimer>

#include "moc_VolumeMeter.cpp"

QPointer<QTimer> VolumeMeter::updateTimer = nullptr;

namespace {
constexpr int INDICATOR_THICKNESS = 3;
constexpr int CLIP_FLASH_DURATION_MS = 1000;
constexpr int TICK_SIZE = 2;
constexpr int TICK_DB_INTERVAL = 6;
} // namespace

static inline QColor color_from_int(long long val)
{
	QColor color(val & 0xff, (val >> 8) & 0xff, (val >> 16) & 0xff, (val >> 24) & 0xff);
	color.setAlpha(255);

	return color;
}

QColor VolumeMeter::getBackgroundNominalColor() const
{
	return p_backgroundNominalColor;
}

QColor VolumeMeter::getBackgroundNominalColorDisabled() const
{
	return backgroundNominalColorDisabled;
}

void VolumeMeter::setBackgroundNominalColor(QColor c)
{
	p_backgroundNominalColor = std::move(c);

	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		backgroundNominalColor =
			color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "MixerGreen"));
	} else {
		backgroundNominalColor = p_backgroundNominalColor;
	}
}

void VolumeMeter::setBackgroundNominalColorDisabled(QColor c)
{
	backgroundNominalColorDisabled = std::move(c);
}

QColor VolumeMeter::getBackgroundWarningColor() const
{
	return p_backgroundWarningColor;
}

QColor VolumeMeter::getBackgroundWarningColorDisabled() const
{
	return backgroundWarningColorDisabled;
}

void VolumeMeter::setBackgroundWarningColor(QColor c)
{
	p_backgroundWarningColor = std::move(c);

	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		backgroundWarningColor =
			color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "MixerYellow"));
	} else {
		backgroundWarningColor = p_backgroundWarningColor;
	}
}

void VolumeMeter::setBackgroundWarningColorDisabled(QColor c)
{
	backgroundWarningColorDisabled = std::move(c);
}

QColor VolumeMeter::getBackgroundErrorColor() const
{
	return p_backgroundErrorColor;
}

QColor VolumeMeter::getBackgroundErrorColorDisabled() const
{
	return backgroundErrorColorDisabled;
}

void VolumeMeter::setBackgroundErrorColor(QColor c)
{
	p_backgroundErrorColor = std::move(c);

	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		backgroundErrorColor =
			color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "MixerRed"));
	} else {
		backgroundErrorColor = p_backgroundErrorColor;
	}
}

void VolumeMeter::setBackgroundErrorColorDisabled(QColor c)
{
	backgroundErrorColorDisabled = std::move(c);
}

QColor VolumeMeter::getForegroundNominalColor() const
{
	return p_foregroundNominalColor;
}

QColor VolumeMeter::getForegroundNominalColorDisabled() const
{
	return foregroundNominalColorDisabled;
}

void VolumeMeter::setForegroundNominalColor(QColor c)
{
	p_foregroundNominalColor = std::move(c);

	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		foregroundNominalColor =
			color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "MixerGreenActive"));
	} else {
		foregroundNominalColor = p_foregroundNominalColor;
	}
}

void VolumeMeter::setForegroundNominalColorDisabled(QColor c)
{
	foregroundNominalColorDisabled = std::move(c);
}

QColor VolumeMeter::getForegroundWarningColor() const
{
	return p_foregroundWarningColor;
}

QColor VolumeMeter::getForegroundWarningColorDisabled() const
{
	return foregroundWarningColorDisabled;
}

void VolumeMeter::setForegroundWarningColor(QColor c)
{
	p_foregroundWarningColor = std::move(c);

	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		foregroundWarningColor =
			color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "MixerYellowActive"));
	} else {
		foregroundWarningColor = p_foregroundWarningColor;
	}
}

void VolumeMeter::setForegroundWarningColorDisabled(QColor c)
{
	foregroundWarningColorDisabled = std::move(c);
}

QColor VolumeMeter::getForegroundErrorColor() const
{
	return p_foregroundErrorColor;
}

QColor VolumeMeter::getForegroundErrorColorDisabled() const
{
	return foregroundErrorColorDisabled;
}

void VolumeMeter::setForegroundErrorColor(QColor c)
{
	p_foregroundErrorColor = std::move(c);

	if (config_get_bool(App()->GetUserConfig(), "Accessibility", "OverrideColors")) {
		foregroundErrorColor =
			color_from_int(config_get_int(App()->GetUserConfig(), "Accessibility", "MixerRedActive"));
	} else {
		foregroundErrorColor = p_foregroundErrorColor;
	}
}

void VolumeMeter::setForegroundErrorColorDisabled(QColor c)
{
	foregroundErrorColorDisabled = std::move(c);
}

QColor VolumeMeter::getMagnitudeColor() const
{
	return magnitudeColor;
}

void VolumeMeter::setMagnitudeColor(QColor c)
{
	magnitudeColor = std::move(c);
}

QColor VolumeMeter::getMajorTickColor() const
{
	return majorTickColor;
}

void VolumeMeter::setMajorTickColor(QColor c)
{
	majorTickColor = std::move(c);
}

QColor VolumeMeter::getMinorTickColor() const
{
	return minorTickColor;
}

void VolumeMeter::setMinorTickColor(QColor c)
{
	minorTickColor = std::move(c);
}

qreal VolumeMeter::getWarningLevel() const
{
	return warningLevel;
}

void VolumeMeter::setWarningLevel(qreal v)
{
	warningLevel = v;
}

qreal VolumeMeter::getErrorLevel() const
{
	return errorLevel;
}

void VolumeMeter::setErrorLevel(qreal v)
{
	errorLevel = v;
}

void VolumeMeter::setPeakDecayRate(qreal decayRate)
{
	peakDecayRate = decayRate;
}

void VolumeMeter::setPeakMeterType(enum obs_peak_meter_type peakMeterType)
{
	obs_volmeter_set_peak_meter_type(obsVolumeMeter, peakMeterType);
	switch (peakMeterType) {
	case TRUE_PEAK_METER:
		// For true-peak meters EBU has defined the Permitted Maximum,
		// taking into account the accuracy of the meter and further
		// processing required by lossy audio compression.
		//
		// The alignment level was not specified, but I've adjusted
		// it compared to a sample-peak meter. Incidentally Youtube
		// uses this new Alignment Level as the maximum integrated
		// loudness of a video.
		//
		//  * Permitted Maximum Level (PML) = -2.0 dBTP
		//  * Alignment Level (AL) = -13 dBTP
		setErrorLevel(-2.0);
		setWarningLevel(-13.0);
		break;

	case SAMPLE_PEAK_METER:
	default:
		// For a sample Peak Meter EBU has the following level
		// definitions, taking into account inaccuracies of this meter:
		//
		//  * Permitted Maximum Level (PML) = -9.0 dBFS
		//  * Alignment Level (AL) = -20.0 dBFS
		setErrorLevel(-9.0);
		setWarningLevel(-20.0);
		break;
	}

	bool forceUpdate = true;
	updateBackgroundCache(forceUpdate);
}

VolumeMeter::VolumeMeter(QWidget *parent, obs_source_t *source)
	: QWidget(parent),
	  weakSource(OBSGetWeakRef(source)),
	  obsVolumeMeter(obs_volmeter_create(OBS_FADER_LOG))
{
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setFocusPolicy(Qt::NoFocus);

	// Default meter settings, they only show if
	// there is no stylesheet, do not remove.
	backgroundNominalColor.setRgb(0x26, 0x7f, 0x26); // Dark green
	backgroundWarningColor.setRgb(0x7f, 0x7f, 0x26); // Dark yellow
	backgroundErrorColor.setRgb(0x7f, 0x26, 0x26);   // Dark red
	foregroundNominalColor.setRgb(0x4c, 0xff, 0x4c); // Bright green
	foregroundWarningColor.setRgb(0xff, 0xff, 0x4c); // Bright yellow
	foregroundErrorColor.setRgb(0xff, 0x4c, 0x4c);   // Bright red

	backgroundNominalColorDisabled.setRgb(90, 90, 90);
	backgroundWarningColorDisabled.setRgb(117, 117, 117);
	backgroundErrorColorDisabled.setRgb(65, 65, 65);
	foregroundNominalColorDisabled.setRgb(163, 163, 163);
	foregroundWarningColorDisabled.setRgb(217, 217, 217);
	foregroundErrorColorDisabled.setRgb(113, 113, 113);

	clipColor.setRgb(0xff, 0xff, 0xff);      // Bright white
	magnitudeColor.setRgb(0x00, 0x00, 0x00); // Black
	majorTickColor.setRgb(0x00, 0x00, 0x00); // Black
	minorTickColor.setRgb(0x32, 0x32, 0x32); // Dark gray
	minimumLevel = -60.0;                    // -60 dB
	warningLevel = -20.0;                    // -20 dB
	errorLevel = -9.0;                       //  -9 dB
	clipLevel = 0.0;                         //  0 dB
	minimumInputLevel = -50.0;               // -50 dB
	peakDecayRate = 11.76;                   //  20 dB / 1.7 sec
	magnitudeIntegrationTime = 0.3;          //  99% in 300 ms
	peakHoldDuration = 20.0;                 //  20 seconds
	inputPeakHoldDuration = 1.0;             //  1 second
	meterThickness = 3;                      // Bar thickness in pixels
	meterFontScaling = 0.8;                  // Font size for numbers is 80% of Widget's font size
	channels = (int)audio_output_get_channels(obs_get_audio());

	obs_volmeter_add_callback(obsVolumeMeter, obsVolMeterChanged, this);
	obs_volmeter_attach_source(obsVolumeMeter, source);

	destroyedSignal =
		OBSSignal(obs_source_get_signal_handler(source), "destroy", &VolumeMeter::obsSourceDestroyed, this);

	doLayout();

	if (!updateTimer) {
		updateTimer = new QTimer(qApp);
		updateTimer->setTimerType(Qt::PreciseTimer);
		updateTimer->start(16);
	}

	connect(updateTimer, &QTimer::timeout, this, [this]() {
		if (needLayoutChange()) {
			doLayout();
		}
		repaint();
	});

	connect(App(), &OBSApp::StyleChanged, this, &VolumeMeter::doLayout);
}

VolumeMeter::~VolumeMeter()
{
	obs_volmeter_remove_callback(obsVolumeMeter, obsVolMeterChanged, this);
	obs_volmeter_detach_source(obsVolumeMeter);
}

void VolumeMeter::obsSourceDestroyed(void *data, calldata_t *)
{
	VolumeMeter *self = static_cast<VolumeMeter *>(data);
	QMetaObject::invokeMethod(self, "handleSourceDestroyed", Qt::QueuedConnection);
}

void VolumeMeter::setLevels(const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS],
			    const float inputPeak[MAX_AUDIO_CHANNELS])
{
	uint64_t ts = os_gettime_ns();
	QMutexLocker locker(&dataMutex);

	currentLastUpdateTime = ts;
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		currentMagnitude[channelNr] = magnitude[channelNr];
		currentPeak[channelNr] = peak[channelNr];
		currentInputPeak[channelNr] = inputPeak[channelNr];
	}

	// In case there are more updates then redraws we must make sure
	// that the ballistics of peak and hold are recalculated.
	locker.unlock();
	calculateBallistics(ts);
}

void VolumeMeter::obsVolMeterChanged(void *data, const float magnitude[MAX_AUDIO_CHANNELS],
				     const float peak[MAX_AUDIO_CHANNELS], const float inputPeak[MAX_AUDIO_CHANNELS])
{
	VolumeMeter *meter = static_cast<VolumeMeter *>(data);

	meter->setLevels(magnitude, peak, inputPeak);
}

inline void VolumeMeter::resetLevels()
{
	currentLastUpdateTime = 0;
	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		currentMagnitude[channelNr] = -M_INFINITE;
		currentPeak[channelNr] = -M_INFINITE;
		currentInputPeak[channelNr] = -M_INFINITE;

		displayMagnitude[channelNr] = -M_INFINITE;
		displayPeak[channelNr] = -M_INFINITE;
		displayPeakHold[channelNr] = -M_INFINITE;
		displayPeakHoldLastUpdateTime[channelNr] = 0;
		displayInputPeakHold[channelNr] = -M_INFINITE;
		displayInputPeakHoldLastUpdateTime[channelNr] = 0;
	}
}

bool VolumeMeter::needLayoutChange()
{
	int currentNrAudioChannels = obs_volmeter_get_nr_channels(obsVolumeMeter);

	if (!currentNrAudioChannels) {
		struct obs_audio_info oai;
		obs_get_audio_info(&oai);
		currentNrAudioChannels = (oai.speakers == SPEAKERS_MONO) ? 1 : 2;
	}

	if (displayNrAudioChannels != currentNrAudioChannels) {
		displayNrAudioChannels = currentNrAudioChannels;
		return true;
	}

	return false;
}

void VolumeMeter::setVertical(bool vertical_)
{
	if (vertical == vertical_) {
		return;
	}

	vertical = vertical_;
	doLayout();
}

void VolumeMeter::setUseDisabledColors(bool enable)
{
	if (useDisabledColors == enable) {
		return;
	}

	useDisabledColors = enable;
}

void VolumeMeter::setMuted(bool mute)
{
	if (muted == mute) {
		return;
	}

	muted = mute;
}

void VolumeMeter::refreshColors()
{
	setBackgroundNominalColor(getBackgroundNominalColor());
	setBackgroundWarningColor(getBackgroundWarningColor());
	setBackgroundErrorColor(getBackgroundErrorColor());
	setForegroundNominalColor(getForegroundNominalColor());
	setForegroundWarningColor(getForegroundWarningColor());
	setForegroundErrorColor(getForegroundErrorColor());

	bool forceUpdate = true;
	updateBackgroundCache(forceUpdate);
}

QRect VolumeMeter::getBarRect() const
{
	QRect barRect = rect();
	if (vertical) {
		barRect.setWidth(displayNrAudioChannels * (meterThickness + 1) - 1);
	} else {
		barRect.setHeight(displayNrAudioChannels * (meterThickness + 1) - 1);
	}

	return barRect;
}

// When this is called from the constructor, obs_volmeter_get_nr_channels has not
// yet been called and Q_PROPERTY settings have not yet been read from the
// stylesheet.
inline void VolumeMeter::doLayout()
{
	QMutexLocker locker(&dataMutex);

	if (displayNrAudioChannels) {
		int meterSize = std::floor(22 / displayNrAudioChannels);
		meterThickness = std::clamp(meterSize, 3, 6);
	}

	tickFont = font();
	QFontInfo info(tickFont);
	tickFont.setPointSizeF(info.pointSizeF() * meterFontScaling);

	QFontMetrics metrics(tickFont);
	// This is a quick and naive assumption for widest potential tick label.
	tickTextTokenRect = metrics.boundingRect(" -88 ");

	updateBackgroundCache();
	resetLevels();

	updateGeometry();
}

inline bool VolumeMeter::detectIdle(uint64_t ts)
{
	double secondsSinceLastUpdate = (ts - currentLastUpdateTime) * 0.000000001;
	if (secondsSinceLastUpdate > 0.5) {
		resetLevels();
		return true;
	} else {
		return false;
	}
}

inline void VolumeMeter::calculateBallisticsForChannel(int channelNr, uint64_t ts, qreal timeSinceLastRedraw)
{
	if (currentPeak[channelNr] >= displayPeak[channelNr] || isnan(displayPeak[channelNr])) {
		// Attack of peak is immediate.
		displayPeak[channelNr] = currentPeak[channelNr];
	} else {
		// Decay of peak is 40 dB / 1.7 seconds for Fast Profile
		// 20 dB / 1.7 seconds for Medium Profile (Type I PPM)
		// 24 dB / 2.8 seconds for Slow Profile (Type II PPM)
		float decay = float(peakDecayRate * timeSinceLastRedraw);
		displayPeak[channelNr] =
			std::clamp(displayPeak[channelNr] - decay, std::min(currentPeak[channelNr], 0.f), 0.f);
	}

	if (currentPeak[channelNr] >= displayPeakHold[channelNr] || !isfinite(displayPeakHold[channelNr])) {
		// Attack of peak-hold is immediate, but keep track
		// when it was last updated.
		displayPeakHold[channelNr] = currentPeak[channelNr];
		displayPeakHoldLastUpdateTime[channelNr] = ts;
	} else {
		// The peak and hold falls back to peak
		// after 20 seconds.
		qreal timeSinceLastPeak = (uint64_t)(ts - displayPeakHoldLastUpdateTime[channelNr]) * 0.000000001;
		if (timeSinceLastPeak > peakHoldDuration) {
			displayPeakHold[channelNr] = currentPeak[channelNr];
			displayPeakHoldLastUpdateTime[channelNr] = ts;
		}
	}

	if (currentInputPeak[channelNr] >= displayInputPeakHold[channelNr] ||
	    !isfinite(displayInputPeakHold[channelNr])) {
		// Attack of peak-hold is immediate, but keep track
		// when it was last updated.
		displayInputPeakHold[channelNr] = currentInputPeak[channelNr];
		displayInputPeakHoldLastUpdateTime[channelNr] = ts;
	} else {
		// The peak and hold falls back to peak after 1 second.
		qreal timeSinceLastPeak = (uint64_t)(ts - displayInputPeakHoldLastUpdateTime[channelNr]) * 0.000000001;
		if (timeSinceLastPeak > inputPeakHoldDuration) {
			displayInputPeakHold[channelNr] = currentInputPeak[channelNr];
			displayInputPeakHoldLastUpdateTime[channelNr] = ts;
		}
	}

	if (!isfinite(displayMagnitude[channelNr])) {
		// The statements in the else-leg do not work with
		// NaN and infinite displayMagnitude.
		displayMagnitude[channelNr] = currentMagnitude[channelNr];
	} else {
		// A VU meter will integrate to the new value to 99% in 300 ms.
		// The calculation here is very simplified and is more accurate
		// with higher frame-rate.
		float attack = float((currentMagnitude[channelNr] - displayMagnitude[channelNr]) *
				     (timeSinceLastRedraw / magnitudeIntegrationTime) * 0.99);
		displayMagnitude[channelNr] =
			std::clamp(displayMagnitude[channelNr] + attack, (float)minimumLevel, 0.f);
	}
}

inline void VolumeMeter::calculateBallistics(uint64_t ts, qreal timeSinceLastRedraw)
{
	QMutexLocker locker(&dataMutex);

	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		calculateBallisticsForChannel(channelNr, ts, timeSinceLastRedraw);
	}
}

QColor VolumeMeter::getPeakColor(float peakHold)
{
	QColor color;

	if (peakHold < minimumInputLevel) {
		color = backgroundNominalColor;
	} else if (peakHold < warningLevel) {
		color = foregroundNominalColor;
	} else if (peakHold < errorLevel) {
		color = foregroundWarningColor;
	} else if (peakHold < clipLevel) {
		color = foregroundErrorColor;
	} else {
		color = clipColor;
	}

	return color;
}

void VolumeMeter::paintHTicks(QPainter &painter, int x, int y, int width)
{
	qreal scale = width / minimumLevel;

	painter.setFont(tickFont);
	QFontMetrics metrics(tickFont);
	painter.setPen(majorTickColor);

	// Draw major tick lines and numeric indicators.
	for (int i = 0; i >= minimumLevel; i -= TICK_DB_INTERVAL) {
		int position = int(x + width - (i * scale) - 1);
		QString str = QString::number(i);

		// Center the number on the tick, but don't overflow
		QRect textBounds = metrics.boundingRect(str);
		int pos;
		if (i == 0) {
			pos = position - textBounds.width();
		} else {
			pos = position - (textBounds.width() / 2);
			if (pos < 0) {
				pos = 0;
			}
		}
		painter.drawText(pos, y + 4 + metrics.capHeight(), str);

		painter.drawLine(position, y, position, y + TICK_SIZE);
	}
}

void VolumeMeter::paintVTicks(QPainter &painter, int x, int y, int height)
{
	qreal scale = height / minimumLevel;

	painter.setFont(tickFont);
	QFontMetrics metrics(tickFont);
	painter.setPen(majorTickColor);

	// Draw major tick lines and numeric indicators.
	for (int i = 0; i >= minimumLevel; i -= TICK_DB_INTERVAL) {
		int position = y + int(i * scale);
		QString str = QString::number(i);

		// Center the number on the tick, but don't overflow
		if (i == 0) {
			painter.drawText(x + 10, position + metrics.capHeight(), str);
		} else {
			painter.drawText(x + 8, position + (metrics.capHeight() / 2), str);
		}

		painter.drawLine(x, position, x + TICK_SIZE, position);
	}
}

void VolumeMeter::updateBackgroundCache(bool force)
{
	if (!force && !size().isValid()) {
		return;
	}

	if (!force && backgroundCache.size() == size() && !backgroundCache.isNull()) {
		return;
	}

	if (!force && displayNrAudioChannels <= 0) {
		return;
	}

	QColor backgroundColor = palette().color(QPalette::Window);

	backgroundCache = QPixmap(size() * devicePixelRatioF());
	backgroundCache.setDevicePixelRatio(devicePixelRatioF());
	backgroundCache.fill(backgroundColor);

	QPainter bg{&backgroundCache};
	QRect widgetRect = rect();

	// Draw ticks
	if (vertical) {
		paintVTicks(bg, displayNrAudioChannels * (meterThickness + 1) - 1, 0,
			    widgetRect.height() - (INDICATOR_THICKNESS + 3));
	} else {
		paintHTicks(bg, INDICATOR_THICKNESS + 3, displayNrAudioChannels * (meterThickness + 1) - 1,
			    widgetRect.width() - (INDICATOR_THICKNESS + 3));
	}

	// Draw meter backgrounds
	bool disabledColors = muted || useDisabledColors;
	QColor nominal = disabledColors ? backgroundNominalColorDisabled : backgroundNominalColor;
	QColor warning = disabledColors ? backgroundWarningColorDisabled : backgroundWarningColor;
	QColor error = disabledColors ? backgroundErrorColorDisabled : backgroundErrorColor;

	int meterStart = INDICATOR_THICKNESS + 2;
	int meterLength = vertical ? rect().height() - (INDICATOR_THICKNESS + 2)
				   : rect().width() - (INDICATOR_THICKNESS + 2);

	qreal scale = meterLength / minimumLevel;

	int warningPosition = meterLength - convertToInt(warningLevel * scale);
	int errorPosition = meterLength - convertToInt(errorLevel * scale);

	int nominalLength = warningPosition;
	int warningLength = nominalLength + (errorPosition - warningPosition);

	for (int channelNr = 0; channelNr < displayNrAudioChannels; channelNr++) {
		int channelOffset = channelNr * (meterThickness + 1);

		if (vertical) {
			bg.fillRect(channelOffset, meterLength, meterThickness, -meterLength, error);
			bg.fillRect(channelOffset, meterLength, meterThickness, -warningLength, warning);
			bg.fillRect(channelOffset, meterLength, meterThickness, -nominalLength, nominal);
		} else {
			bg.fillRect(meterStart, channelOffset, meterLength, meterThickness, error);
			bg.fillRect(meterStart, channelOffset, warningLength, meterThickness, warning);
			bg.fillRect(meterStart, channelOffset, nominalLength, meterThickness, nominal);
		}
	}
}

inline int VolumeMeter::convertToInt(float number)
{
	constexpr int min = std::numeric_limits<int>::min();
	constexpr int max = std::numeric_limits<int>::max();

	// NOTE: Conversion from 'const int' to 'float' changes max value from 2147483647 to 2147483648
	if (number >= (float)max) {
		return max;
	} else if (number < min) {
		return min;
	} else {
		return int(number);
	}
}

void VolumeMeter::paintEvent(QPaintEvent *)
{
	uint64_t ts = os_gettime_ns();
	qreal timeSinceLastRedraw = (ts - lastRedrawTime) * 0.000000001;
	calculateBallistics(ts, timeSinceLastRedraw);
	bool idle = detectIdle(ts);

	QPainter painter(this);

	bool disabledColors = muted || useDisabledColors;
	QColor nominal = disabledColors ? foregroundNominalColorDisabled : foregroundNominalColor;
	QColor warning = disabledColors ? foregroundWarningColorDisabled : foregroundWarningColor;
	QColor error = disabledColors ? foregroundErrorColorDisabled : foregroundErrorColor;

	int meterStart = INDICATOR_THICKNESS + 2;
	int meterLength = vertical ? rect().height() - (INDICATOR_THICKNESS + 2)
				   : rect().width() - (INDICATOR_THICKNESS + 2);

	const qreal scale = meterLength / minimumLevel;

	// Paint cached background pixmap
	painter.drawPixmap(0, 0, backgroundCache);

	// Draw dynamic audio meter bars
	int warningPosition = meterLength - convertToInt(warningLevel * scale);
	int errorPosition = meterLength - convertToInt(errorLevel * scale);
	int clipPosition = meterLength - convertToInt(clipLevel * scale);

	int nominalLength = warningPosition;
	int warningLength = nominalLength + (errorPosition - warningPosition);

	for (int channelNr = 0; channelNr < displayNrAudioChannels; channelNr++) {
		int channelNrFixed = (displayNrAudioChannels == 1 && channels > 2) ? 2 : channelNr;

		QMutexLocker locker(&dataMutex);
		float peak = displayPeak[channelNrFixed];
		float peakHold = displayPeakHold[channelNrFixed];
		float magnitude = displayMagnitude[channelNrFixed];

		int peakPosition = meterLength - convertToInt(peak * scale);
		int peakHoldPosition = meterLength - convertToInt(peakHold * scale);
		int magnitudePosition = meterLength - convertToInt(magnitude * scale);
		locker.unlock();

		if (clipping) {
			peakPosition = meterLength;
		}

		auto fill = [&](int pos, int length, const QColor &color) {
			if (vertical) {
				painter.fillRect(pos, meterLength, meterThickness, -length, color);
			} else {
				painter.fillRect(meterStart, pos, length, meterThickness, color);
			}
		};

		int channelOffset = channelNr * (meterThickness + 1);

		// Draw audio meter peak bars
		if (peakPosition >= clipPosition) {
			if (!clipping) {
				QTimer::singleShot(CLIP_FLASH_DURATION_MS, this, [&]() { clipping = false; });
				clipping = true;
			}

			fill(channelOffset, meterLength, error);
		} else {
			if (peakPosition > errorPosition) {
				fill(channelOffset, std::min(peakPosition, meterLength), error);
			}
			if (peakPosition > warningPosition) {
				fill(channelOffset, std::min(peakPosition, warningLength), warning);
			}
			if (peakPosition > meterStart) {
				fill(channelOffset, std::min(peakPosition, nominalLength), nominal);
			}
		}

		// Draw peak hold indicators
		QColor peakHoldColor = nominal;
		if (peakHoldPosition >= errorPosition) {
			peakHoldColor = error;
		} else if (peakHoldPosition >= warningPosition) {
			peakHoldColor = warning;
		}

		if (peakHoldPosition - 3 > 0) {
			if (vertical) {
				painter.fillRect(channelOffset, meterLength - peakHoldPosition - 3, meterThickness, 3,
						 peakHoldColor);
			} else {
				painter.fillRect(meterStart + peakHoldPosition - 3, channelOffset, 3, meterThickness,
						 peakHoldColor);
			}
		}

		// Draw magnitude indicator
		if (magnitudePosition - 3 >= 0) {
			if (vertical) {
				painter.fillRect(channelOffset, meterLength - magnitudePosition - 3, meterThickness, 3,
						 magnitudeColor);
			} else {
				painter.fillRect(meterStart + magnitudePosition - 3, channelOffset, 3, meterThickness,
						 magnitudeColor);
			}
		}

		if (idle) {
			continue;
		}

		// Draw audio input indicator
		if (vertical) {
			painter.fillRect(channelOffset, rect().height(), meterThickness, -INDICATOR_THICKNESS,
					 getPeakColor(displayInputPeakHold[channelNrFixed]));
		} else {
			painter.fillRect(0, channelOffset, INDICATOR_THICKNESS, meterThickness,
					 getPeakColor(displayInputPeakHold[channelNrFixed]));
		}
	}

	lastRedrawTime = ts;
}

void VolumeMeter::resizeEvent(QResizeEvent *event)
{
	updateBackgroundCache();
	return QWidget::resizeEvent(event);
}

void VolumeMeter::mousePressEvent(QMouseEvent *event)
{
	setFocus(Qt::MouseFocusReason);
	event->accept();
}

void VolumeMeter::wheelEvent(QWheelEvent *event)
{
	QApplication::sendEvent(focusProxy(), event);
}

QSize VolumeMeter::minimumSizeHint() const
{
	return sizeHint();
}

QSize VolumeMeter::sizeHint() const
{
	QRect meterRect = getBarRect();
	int labelTotal = std::abs(minimumLevel / TICK_DB_INTERVAL) + 1;

	if (vertical) {
		int width = meterRect.width() + tickTextTokenRect.width() + TICK_SIZE + 10;
		int height = (labelTotal * tickTextTokenRect.height()) + INDICATOR_THICKNESS;

		return QSize(width, height * 1.1);
	} else {
		int width = (labelTotal * tickTextTokenRect.width()) + INDICATOR_THICKNESS;
		int height = meterRect.height() + tickTextTokenRect.height();

		return QSize(width * 1.1, height);
	}
}
