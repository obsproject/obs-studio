#include "OBSAudioMeter.hpp"
#include "qt-wrappers.hpp"
#include "obs-app.hpp"

static constexpr double lerp(double t, double a, double b)
{
	return (1 - t)*a + t * b;
}

static constexpr double clamp(double t, double min, double max)
{
	return t < min ? min : (t > max ? max : t);
}

using namespace std;

inline void OBSAudioMeter::calculateBallisticsForChannel(int channelNr,
	uint64_t ts, qreal timeSinceLastRedraw)
{
	if (currentPeak[channelNr] >= displayPeak[channelNr] ||
		isnan(displayPeak[channelNr])) {
		// Attack of peak is immediate.
		displayPeak[channelNr] = currentPeak[channelNr];
	} else {
		// Decay of peak is 40 dB / 1.7 seconds for Fast Profile
		// 20 dB / 1.7 seconds for Medium Profile (Type I PPM)
		// 24 dB / 2.8 seconds for Slow Profile (Type II PPM)
		float decay = float(peakDecayRate * timeSinceLastRedraw);
		displayPeak[channelNr] = clamp(displayPeak[channelNr] - decay,
			currentPeak[channelNr], 0.0);
	}

	if (currentInputPeak[channelNr] >= displayInputPeak[channelNr] ||
		isnan(displayInputPeak[channelNr])) {
		// Attack of peak is immediate.
		displayInputPeak[channelNr] = currentInputPeak[channelNr];
	} else {
		// Decay of peak is 40 dB / 1.7 seconds for Fast Profile
		// 20 dB / 1.7 seconds for Medium Profile (Type I PPM)
		// 24 dB / 2.8 seconds for Slow Profile (Type II PPM)
		float decay = float(peakDecayRate * timeSinceLastRedraw);
		displayInputPeak[channelNr] = clamp(displayInputPeak[channelNr] - decay,
			currentInputPeak[channelNr], 0.0);
	}

	if (currentPeak[channelNr] >= displayPeakHold[channelNr] ||
		!isfinite(displayPeakHold[channelNr])) {
		// Attack of peak-hold is immediate, but keep track
		// when it was last updated.
		displayPeakHold[channelNr] = currentPeak[channelNr];
		displayPeakHoldLastUpdateTime[channelNr] = ts;
	} else {
		// The peak and hold falls back to peak
		// after 20 seconds.
		qreal timeSinceLastPeak = (uint64_t)(ts -
			displayPeakHoldLastUpdateTime[channelNr]) * 0.000000001;
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
		qreal timeSinceLastPeak = (uint64_t)(ts -
			displayInputPeakHoldLastUpdateTime[channelNr]) *
			0.000000001;
		if (timeSinceLastPeak > inputPeakHoldDuration) {
			displayInputPeakHold[channelNr] =
				currentInputPeak[channelNr];
			displayInputPeakHoldLastUpdateTime[channelNr] =
				ts;
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
		float attack = float((currentMagnitude[channelNr] -
			displayMagnitude[channelNr]) *
			(timeSinceLastRedraw /
				magnitudeIntegrationTime) * 0.99);
		displayMagnitude[channelNr] = clamp(displayMagnitude[channelNr]
			+ attack, (float)minimumLevel, 0.0);
	}
}

void OBSAudioMeter::calculateBallistics(uint64_t ts, qreal timeSinceLastRedraw)
{
	QMutexLocker locker(&dataMutex);

	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++)
		calculateBallisticsForChannel(channelNr, ts,
			timeSinceLastRedraw);
}

void OBSAudioMeter::paintInputMeter(QPainter &painter, int x, int y, int width,
	int height, float peakHold)
{
	QMutexLocker locker(&dataMutex);
	QColor color;

	if (peakHold < minimumLevel)
		color = backgroundNominalColor;
	else if (peakHold < warningLevel)
		color = foregroundNominalColor;
	else if (peakHold < errorLevel)
		color = foregroundWarningColor;
	else if (peakHold <= 0.0f)
		color = foregroundErrorColor;
	else
		color = clipColor;

	painter.fillRect(x, y, width, height, color);
}

void OBSAudioMeter::paintHTicks(QPainter &painter, int x, int y, int width,
	int height)
{
	if (_tick_opts == none)
		return;
	qreal scale = width / minimumLevel;

	painter.setFont(tickFont);
	painter.setPen(majorTickColor);

	if (_tick_opts == bottom) {
		// Draw major tick lines and numeric indicators.
		for (int i = 0; i >= minimumLevel; i -= 5) {
			int position = int(x + width - (i * scale) - 1);
			if (_tick_db) {
				QString str = QString::number(i);

				if (i == 0 || i == -5)
					painter.drawText(position - 3, height, str);
				else
					painter.drawText(position - 5, height, str);
			}
			painter.drawLine(position, y, position, y + 2);
		}

		// Draw minor tick lines.
		painter.setPen(minorTickColor);
		for (int i = 0; i >= minimumLevel; i--) {
			int position = int(x + width - (i * scale) - 1);
			if (i % 5 != 0)
				painter.drawLine(position, y, position, y + 1);
		}
	} else {
		//painter.translate(width, 0);
		//painter.scale(-1, 1);
		// Draw major tick lines.
		for (int i = 0; i >= minimumLevel; i -= 5) {
			int position = int(x + width - (i * scale) - 1);
			painter.drawLine(position, y + height, position, y + (height - 2));
		}

		// Draw minor tick lines.
		painter.setPen(minorTickColor);
		for (int i = 0; i >= minimumLevel; i--) {
			int position = int(x + width - (i * scale) - 1);
			if (i % 5 != 0)
				painter.drawLine(position, y + height, position, y + (height - 1));
		}
		// Draw numeric indicators
		painter.setPen(majorTickColor);
		for (int i = 0; i >= minimumLevel; i -= 5) {
			int position = int(x + width - (i * scale) - 1);
			if (_tick_db) {
				QString str = QString::number(i);

				if (i == 0 || i == -5)
					painter.drawText(position - 3, y + (height - 4), str);
				else
					painter.drawText(position - 5, y + (height - 4), str);
			}
		}
	}
}

void OBSAudioMeter::paintVTicks(QPainter &painter, int x, int y, int width, int height)
{
	if (_tick_opts == none)
		return;
	qreal scale = height / minimumLevel;

	painter.setFont(tickFont);
	painter.setPen(majorTickColor);

	if (_tick_opts == right) {
		// Draw major tick lines and numeric indicators.
		for (int i = 0; i >= minimumLevel; i -= 5) {
			int position = y + int((i * scale) - 1);
			if (_tick_db) {
				QString str = QString::number(i);

				if (i == 0)
					painter.drawText(x + 5, position + 4, str);
				else if (i == -60)
					painter.drawText(x + 4, position, str);
				else
					painter.drawText(x + 4, position + 2, str);
			}
			painter.drawLine(x, position, x + 2, position);
		}

		// Draw minor tick lines.
		painter.setPen(minorTickColor);
		for (int i = 0; i >= minimumLevel; i--) {
			int position = y + int((i * scale) - 1);
			if (i % 5 != 0)
				painter.drawLine(x, position, x + 1, position);
		}
	} else {
		// Draw major tick lines
		for (int i = 0; i >= minimumLevel; i -= 5) {
			int position = y + int((i * scale) - 1);
			painter.drawLine(x + width, position, x + (width - 2), position);
		}

		// Draw minor tick lines.
		painter.setPen(minorTickColor);
		for (int i = 0; i >= minimumLevel; i--) {
			int position = y + int((i * scale) - 1);
			if (i % 5 != 0)
				painter.drawLine(x + width, position, x + (width - 1), position);
		}

		if (_tick_db) {
			painter.setPen(majorTickColor);
			int fontHeight = painter.font().pixelSize();
			for (int i = 0; i >= minimumLevel; i -= 5) {
				int position = y + int((i * scale) - 1);
				QString str = QString::number(i);

				if (i == 0)
					painter.drawText(x, position + 4 - fontHeight, (width - 3), fontHeight, Qt::AlignBottom | Qt::AlignRight, str);
				else if (i == minimumLevel)
					painter.drawText(x, position + 1 - fontHeight, (width - 3), fontHeight, Qt::AlignBottom | Qt::AlignRight, str);
				else
					painter.drawText(x, position + 2 - fontHeight, (width - 3), fontHeight, Qt::AlignBottom | Qt::AlignRight, str);
			}
		}
	}
}

void OBSAudioMeter::paintHMeter(QPainter &painter, int x, int y, int width,
	int height, float magnitude, float peak, float inputPeak, float peakHold, float inputPeakHold = -M_INFINITE)
{
	qreal scale = width / minimumLevel;

	QMutexLocker locker(&dataMutex);
	int minimumPosition = x + 0;
	int maximumPosition = x + width;
	int magnitudePosition = int(x + width - (magnitude * scale));
	int peakPosition = int(x + width - (peak * scale));
//	int inputPeakPosition = int(x + width - (inputPeak * scale));
	int peakHoldPosition = int(x + width - (peakHold * scale));
//	int inputPeakHoldPosition = int(x + width - (inputPeakHold * scale));
	int warningPosition = int(x + width - (warningLevel * scale));
	int errorPosition = int(x + width - (errorLevel * scale));

	int nominalLength = warningPosition - minimumPosition;
	int warningLength = errorPosition - warningPosition;
	int errorLength = maximumPosition - errorPosition;
	locker.unlock();

	auto drawMeter = [&painter, &minimumPosition, &warningPosition,
		&errorPosition, &maximumPosition, &nominalLength, &warningLength, &errorLength,
		&x, &y, &height](int peak, int peakHold, const QColor &foregroundNominalColor,
			const QColor &foregroundWarningColor, const QColor &foregroundErrorColor) {
		if (peak < minimumPosition) {

		} else if (peak < warningPosition) {
			painter.fillRect(minimumPosition, y, peak -
				minimumPosition, height,
				foregroundNominalColor);
		} else if (peak < errorPosition) {
			painter.fillRect(minimumPosition, y, nominalLength, height,
				foregroundNominalColor);
			painter.fillRect(warningPosition, y,
				peak - warningPosition, height,
				foregroundWarningColor);
		} else if (peak < maximumPosition) {
			painter.fillRect(minimumPosition, y, nominalLength, height,
				foregroundNominalColor);
			painter.fillRect(warningPosition, y, warningLength, height,
				foregroundWarningColor);
			painter.fillRect(errorPosition, y, peak - errorPosition,
				height, foregroundErrorColor);
		} else {
			painter.fillRect(minimumPosition, y, nominalLength, height,
				foregroundNominalColor);
			painter.fillRect(warningPosition, y, warningLength, height,
				foregroundWarningColor);
			painter.fillRect(errorPosition, y, errorLength, height,
				foregroundErrorColor);
		}
		/* Draw Peak Hold */
		if (peakHold - 3 < minimumPosition)
			;// Peak-hold below minimum, no drawing.
		else if (peakHold < warningPosition)
			painter.fillRect(peakHold - 3, y, 3, height,
				foregroundNominalColor);
		else if (peakHold < errorPosition)
			painter.fillRect(peakHold - 3, y, 3, height,
				foregroundWarningColor);
		else if (peakHold < maximumPosition)
			painter.fillRect(peakHold - 3, y, 3, height,
				foregroundErrorColor);
		else
			painter.fillRect(maximumPosition - 7, y, 7, height,
				foregroundErrorColor);
	};
	/*Draw Background*/
	painter.fillRect(minimumPosition, y, nominalLength, height,
		backgroundNominalColor);
	painter.fillRect(warningPosition, y, warningLength, height,
		backgroundWarningColor);
	painter.fillRect(errorPosition, y, errorLength, height,
		backgroundErrorColor);

	/*Draw Meter*/
	drawMeter(peakPosition, peakHoldPosition, foregroundNominalColor, foregroundWarningColor, foregroundErrorColor);

	/*Draw Magnitude*/
	if (magnitudePosition - 3 >= minimumPosition)
		painter.fillRect(magnitudePosition - 3, y, 3, height,
			magnitudeColor);
}

void OBSAudioMeter::paintVMeter(QPainter &painter, int x, int y, int width,
	int height, float magnitude, float peak, float inputPeak, float peakHold, float inputPeakHold = -M_INFINITE)
{
	qreal scale = height / minimumLevel;

	QMutexLocker locker(&dataMutex);
	int minimumPosition = y + 0;
	int maximumPosition = y + height;
	int magnitudePosition = int(y + height - (magnitude * scale));
	int peakPosition = int(y + height - (peak * scale));
	int inputPeakPosition = int(y + height - (inputPeak * scale));
	int peakHoldPosition = int(y + height - (peakHold * scale));
	int inputPeakHoldPosition = int(y + height - (inputPeakHold * scale));
	int warningPosition = int(y + height - (warningLevel * scale));
	int errorPosition = int(y + height - (errorLevel * scale));

	int nominalLength = warningPosition - minimumPosition;
	int warningLength = errorPosition - warningPosition;
	int errorLength = maximumPosition - errorPosition;
	locker.unlock();

	auto drawMeter = [&painter, &minimumPosition, &warningPosition,
		&errorPosition, &maximumPosition, &nominalLength, &warningLength, &errorLength,
		&x, &y, &width](int peak, int peakHold, const QColor &foregroundNominalColor,
			const QColor &foregroundWarningColor, const QColor &foregroundErrorColor) {
		if (peak < minimumPosition) {

		} else if (peak < warningPosition) {
			painter.fillRect(x, minimumPosition, width, peak -
				minimumPosition, foregroundNominalColor);
		} else if (peak < errorPosition) {
			painter.fillRect(x, minimumPosition, width, nominalLength,
				foregroundNominalColor);
			painter.fillRect(x, warningPosition, width, peak -
				warningPosition, foregroundWarningColor);
		} else if (peak < maximumPosition) {
			painter.fillRect(x, minimumPosition, width, nominalLength,
				foregroundNominalColor);
			painter.fillRect(x, warningPosition, width, warningLength,
				foregroundWarningColor);
			painter.fillRect(x, errorPosition, width, peak -
				errorPosition, foregroundErrorColor);
		} else {
			painter.fillRect(x, minimumPosition, width, nominalLength,
				foregroundNominalColor);
			painter.fillRect(x, warningPosition, width, warningLength,
				foregroundWarningColor);
			painter.fillRect(x, errorPosition, width, errorLength,
				foregroundErrorColor);
		}

		if (peakHold - 3 < minimumPosition)
			;// Peak-hold below minimum, no drawing.
		else if (peakHold < warningPosition)
			painter.fillRect(x, peakHold - 3, width, 3,
				foregroundNominalColor);
		else if (peakHold < errorPosition)
			painter.fillRect(x, peakHold - 3, width, 3,
				foregroundWarningColor);
		else if (peakHold < maximumPosition)
			painter.fillRect(x, peakHold - 3, width, 3,
				foregroundErrorColor);
		else
			painter.fillRect(x, maximumPosition - 3, width, 3,
				foregroundErrorColor);
	};
	/*Draw Background*/
	painter.fillRect(x, minimumPosition, width, nominalLength,
		backgroundNominalColor);
	painter.fillRect(x, warningPosition, width, warningLength,
		backgroundWarningColor);
	painter.fillRect(x, errorPosition, width, errorLength,
		backgroundErrorColor);

	/*Draw Meter*/
	drawMeter(peakPosition, peakHoldPosition, foregroundNominalColor, foregroundWarningColor, foregroundErrorColor);

	if (magnitudePosition - 3 >= minimumPosition)
		painter.fillRect(x, magnitudePosition - 3, width, 3,
			magnitudeColor);
}

void OBSAudioMeter::paintEvent(QPaintEvent *event)
{
	uint64_t ts = os_gettime_ns();
	qreal timeSinceLastRedraw = (ts - lastRedrawTime) * 0.000000001;
	calculateBallistics(ts, timeSinceLastRedraw);
	handleChannelConfigurationChange();

	bool idle = detectIdle(ts);

	QPainter painter(this);
	/*Draw cached background*/
	const QRect rect = event->region().boundingRect();
	int width = rect.width();
	int height = rect.height();
	/*
	int width = this->width();
	int height = this->height();
	*/
	tick_location ticks = _tick_opts;

	// Draw the ticks in a off-screen buffer when the widget changes size.
	QSize tickPaintCacheSize;
	if (vertical)
		tickPaintCacheSize = QSize(14, height);
	else
		tickPaintCacheSize = QSize(width, 9);

	if (tickPaintCache == nullptr ||
		tickPaintCache->size() != tickPaintCacheSize) {
		delete tickPaintCache;
		tickPaintCache = new QPixmap(tickPaintCacheSize);

		QColor clearColor(0, 0, 0, 0);
		tickPaintCache->fill(clearColor);

		QPainter tickPainter(tickPaintCache);
		if (vertical) {
			tickPainter.translate(0, height);
			tickPainter.scale(1, -1);
			paintVTicks(tickPainter, 0, 11,
				tickPaintCacheSize.width(),
				tickPaintCacheSize.height() - 11);
		} else {
			paintHTicks(tickPainter, 6, 0,
				tickPaintCacheSize.width() - 6,
				tickPaintCacheSize.height());
		}
		tickPainter.end();
	}

	int maxChannelNr = _channels;
	// Actual painting of the widget starts here.

	// Invert the Y axis to ease the math
	if (vertical) {
		painter.translate(0, height);
		painter.scale(1, -1);
	}
	if (ticks != none) {
		if (vertical) {
			if (ticks == right) {
				painter.drawPixmap(maxChannelNr * 4 - 1, 7, *tickPaintCache);
			} else {
				painter.drawPixmap(0, 7, *tickPaintCache);
			}
		} else {
			if (ticks == bottom) {
				painter.drawPixmap(0, maxChannelNr * 4 - 1, *tickPaintCache);
			} else {
				painter.drawPixmap(0, 0, *tickPaintCache);
			}
		}
	}

	float maxPeak = -M_INFINITE;
	for (int channelNr = 0; channelNr < maxChannelNr; channelNr++) {
		int offset = channelNr * 4;
		if (vertical && ticks == left)
			offset += tickPaintCacheSize.width();
		else if (!vertical && ticks == top)
			offset += tickPaintCacheSize.height();

		if (vertical)
			paintVMeter(painter, offset, 8, 3, height - 10,
				displayMagnitude[channelNr],
				displayPeak[channelNr],
				displayInputPeak[channelNr],
				displayPeakHold[channelNr],
				displayInputPeakHold[channelNr]);
		else
			paintHMeter(painter, 5, offset, width - 5, 3,
				displayMagnitude[channelNr],
				displayPeak[channelNr],
				displayInputPeak[channelNr],
				displayPeakHold[channelNr],
				displayInputPeakHold[channelNr]);

		if (displayPeak[channelNr] > maxPeak)
			maxPeak = displayPeak[channelNr];

		if (idle)
			continue;

		// By not drawing the input meter boxes the user can
		// see that the audio stream has been stopped, without
		// having too much visual impact.
		if (vertical)
			paintInputMeter(painter, offset, 3, 3, 3,
				displayInputPeakHold[channelNr]);
		else
			paintInputMeter(painter, 0, offset, 3, 3,
				displayInputPeakHold[channelNr]);
	}

	lastRedrawTime = ts;
}

void OBSAudioMeter::setLevels(const float magnitude[MAX_AUDIO_CHANNELS],
	const float peak[MAX_AUDIO_CHANNELS],
	const float inputPeak[MAX_AUDIO_CHANNELS])
{
	currentLastUpdateTime = os_gettime_ns();
	QMutexLocker locker(&dataMutex);

	for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
		currentMagnitude[channelNr] = magnitude[channelNr];
		currentPeak[channelNr] = peak[channelNr];
		currentInputPeak[channelNr] = inputPeak[channelNr];
	}

	// In case there are more updates then redraws we must make sure
	// that the ballistics of peak and hold are recalculated.
	locker.unlock();
	calculateBallistics(currentLastUpdateTime);
}

QColor OBSAudioMeter::getBackgroundNominalColor() const
{
	return backgroundNominalColor;
}

void OBSAudioMeter::setBackgroundNominalColor(QColor c)
{
	backgroundNominalColor = std::move(c);
}

QColor OBSAudioMeter::getBackgroundWarningColor() const
{
	return backgroundWarningColor;
}

void OBSAudioMeter::setBackgroundWarningColor(QColor c)
{
	backgroundWarningColor = std::move(c);
}

QColor OBSAudioMeter::getBackgroundErrorColor() const
{
	return backgroundErrorColor;
}

void OBSAudioMeter::setBackgroundErrorColor(QColor c)
{
	backgroundErrorColor = std::move(c);
}

QColor OBSAudioMeter::getForegroundNominalColor() const
{
	return foregroundNominalColor;
}

void OBSAudioMeter::setForegroundNominalColor(QColor c)
{
	foregroundNominalColor = std::move(c);
}

QColor OBSAudioMeter::getForegroundWarningColor() const
{
	return foregroundWarningColor;
}

void OBSAudioMeter::setForegroundWarningColor(QColor c)
{
	foregroundWarningColor = std::move(c);
}

QColor OBSAudioMeter::getForegroundErrorColor() const
{
	return foregroundErrorColor;
}

void OBSAudioMeter::setForegroundErrorColor(QColor c)
{
	foregroundErrorColor = std::move(c);
}

QColor OBSAudioMeter::getClipColor() const
{
	return clipColor;
}

void OBSAudioMeter::setClipColor(QColor c)
{
	clipColor = std::move(c);
}

QColor OBSAudioMeter::getMagnitudeColor() const
{
	return magnitudeColor;
}

void OBSAudioMeter::setMagnitudeColor(QColor c)
{
	magnitudeColor = std::move(c);
}

QColor OBSAudioMeter::getMajorTickColor() const
{
	return majorTickColor;
}

void OBSAudioMeter::setMajorTickColor(QColor c)
{
	majorTickColor = std::move(c);
}

QColor OBSAudioMeter::getMinorTickColor() const
{
	return minorTickColor;
}

void OBSAudioMeter::setMinorTickColor(QColor c)
{
	minorTickColor = std::move(c);
}

qreal OBSAudioMeter::getMinimumLevel() const
{
	return minimumLevel;
}

void OBSAudioMeter::setMinimumLevel(qreal v)
{
	minimumLevel = v;
}

qreal OBSAudioMeter::getWarningLevel() const
{
	return warningLevel;
}

void OBSAudioMeter::setWarningLevel(qreal v)
{
	warningLevel = v;
}

qreal OBSAudioMeter::getErrorLevel() const
{
	return errorLevel;
}

void OBSAudioMeter::setErrorLevel(qreal v)
{
	errorLevel = v;
}

qreal OBSAudioMeter::getClipLevel() const
{
	return clipLevel;
}

void OBSAudioMeter::setClipLevel(qreal v)
{
	clipLevel = v;
}

qreal OBSAudioMeter::getMinimumInputLevel() const
{
	return minimumInputLevel;
}

void OBSAudioMeter::setMinimumInputLevel(qreal v)
{
	minimumInputLevel = v;
}

qreal OBSAudioMeter::getPeakDecayRate() const
{
	return peakDecayRate;
}

void OBSAudioMeter::setPeakDecayRate(qreal v)
{
	peakDecayRate = v;
}

qreal OBSAudioMeter::getMagnitudeIntegrationTime() const
{
	return magnitudeIntegrationTime;
}

void OBSAudioMeter::setMagnitudeIntegrationTime(qreal v)
{
	magnitudeIntegrationTime = v;
}

qreal OBSAudioMeter::getPeakHoldDuration() const
{
	return peakHoldDuration;
}

void OBSAudioMeter::setPeakHoldDuration(qreal v)
{
	peakHoldDuration = v;
}

qreal OBSAudioMeter::getInputPeakHoldDuration() const
{
	return inputPeakHoldDuration;
}

void OBSAudioMeter::setInputPeakHoldDuration(qreal v)
{
	inputPeakHoldDuration = v;
}

void OBSAudioMeter::setPeakMeterType(enum obs_peak_meter_type peakMeterType)
{
	//obs_volmeter_set_peak_meter_type(obs_volmeter, peakMeterType);
	switch (peakMeterType) {
	case TRUE_PEAK_METER:
		// For true-peak meters EBU has defined the Permitted Maximum,
		// taking into account the accuracy of the meter and further
		// processing required by lossy audio compression.
		//
		// The alignment level was not specified, but I've adjusted
		// it compared to a sample-peak meter. Incidently Youtube
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
}

