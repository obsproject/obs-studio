#pragma once

#include <obs.hpp>

#include <QMutex>
#include <QWidget>

#define FADER_PRECISION 4096.0

class VolumeMeterTimer;

class VolumeMeter : public QWidget {
	Q_OBJECT
	Q_PROPERTY(QColor backgroundNominalColor READ getBackgroundNominalColor WRITE setBackgroundNominalColor
			   DESIGNABLE true)
	Q_PROPERTY(QColor backgroundWarningColor READ getBackgroundWarningColor WRITE setBackgroundWarningColor
			   DESIGNABLE true)
	Q_PROPERTY(
		QColor backgroundErrorColor READ getBackgroundErrorColor WRITE setBackgroundErrorColor DESIGNABLE true)
	Q_PROPERTY(QColor foregroundNominalColor READ getForegroundNominalColor WRITE setForegroundNominalColor
			   DESIGNABLE true)
	Q_PROPERTY(QColor foregroundWarningColor READ getForegroundWarningColor WRITE setForegroundWarningColor
			   DESIGNABLE true)
	Q_PROPERTY(
		QColor foregroundErrorColor READ getForegroundErrorColor WRITE setForegroundErrorColor DESIGNABLE true)

	Q_PROPERTY(QColor backgroundNominalColorDisabled READ getBackgroundNominalColorDisabled WRITE
			   setBackgroundNominalColorDisabled DESIGNABLE true)
	Q_PROPERTY(QColor backgroundWarningColorDisabled READ getBackgroundWarningColorDisabled WRITE
			   setBackgroundWarningColorDisabled DESIGNABLE true)
	Q_PROPERTY(QColor backgroundErrorColorDisabled READ getBackgroundErrorColorDisabled WRITE
			   setBackgroundErrorColorDisabled DESIGNABLE true)
	Q_PROPERTY(QColor foregroundNominalColorDisabled READ getForegroundNominalColorDisabled WRITE
			   setForegroundNominalColorDisabled DESIGNABLE true)
	Q_PROPERTY(QColor foregroundWarningColorDisabled READ getForegroundWarningColorDisabled WRITE
			   setForegroundWarningColorDisabled DESIGNABLE true)
	Q_PROPERTY(QColor foregroundErrorColorDisabled READ getForegroundErrorColorDisabled WRITE
			   setForegroundErrorColorDisabled DESIGNABLE true)

	Q_PROPERTY(QColor clipColor READ getClipColor WRITE setClipColor DESIGNABLE true)
	Q_PROPERTY(QColor magnitudeColor READ getMagnitudeColor WRITE setMagnitudeColor DESIGNABLE true)
	Q_PROPERTY(QColor majorTickColor READ getMajorTickColor WRITE setMajorTickColor DESIGNABLE true)
	Q_PROPERTY(QColor minorTickColor READ getMinorTickColor WRITE setMinorTickColor DESIGNABLE true)
	Q_PROPERTY(int meterThickness READ getMeterThickness WRITE setMeterThickness DESIGNABLE true)
	Q_PROPERTY(qreal meterFontScaling READ getMeterFontScaling WRITE setMeterFontScaling DESIGNABLE true)

	// Levels are denoted in dBFS.
	Q_PROPERTY(qreal minimumLevel READ getMinimumLevel WRITE setMinimumLevel DESIGNABLE true)
	Q_PROPERTY(qreal warningLevel READ getWarningLevel WRITE setWarningLevel DESIGNABLE true)
	Q_PROPERTY(qreal errorLevel READ getErrorLevel WRITE setErrorLevel DESIGNABLE true)
	Q_PROPERTY(qreal clipLevel READ getClipLevel WRITE setClipLevel DESIGNABLE true)
	Q_PROPERTY(qreal minimumInputLevel READ getMinimumInputLevel WRITE setMinimumInputLevel DESIGNABLE true)

	// Rates are denoted in dB/second.
	Q_PROPERTY(qreal peakDecayRate READ getPeakDecayRate WRITE setPeakDecayRate DESIGNABLE true)

	// Time in seconds for the VU meter to integrate over.
	Q_PROPERTY(qreal magnitudeIntegrationTime READ getMagnitudeIntegrationTime WRITE setMagnitudeIntegrationTime
			   DESIGNABLE true)

	// Duration is denoted in seconds.
	Q_PROPERTY(qreal peakHoldDuration READ getPeakHoldDuration WRITE setPeakHoldDuration DESIGNABLE true)
	Q_PROPERTY(qreal inputPeakHoldDuration READ getInputPeakHoldDuration WRITE setInputPeakHoldDuration
			   DESIGNABLE true)

	friend class VolControl;

private:
	obs_volmeter_t *obs_volmeter;
	static std::weak_ptr<VolumeMeterTimer> updateTimer;
	std::shared_ptr<VolumeMeterTimer> updateTimerRef;

	inline void resetLevels();
	inline void doLayout();
	inline bool detectIdle(uint64_t ts);
	inline void calculateBallistics(uint64_t ts, qreal timeSinceLastRedraw = 0.0);
	inline void calculateBallisticsForChannel(int channelNr, uint64_t ts, qreal timeSinceLastRedraw);

	inline int convertToInt(float number);
	void paintInputMeter(QPainter &painter, int x, int y, int width, int height, float peakHold);
	void paintHMeter(QPainter &painter, int x, int y, int width, int height, float magnitude, float peak,
			 float peakHold);
	void paintHTicks(QPainter &painter, int x, int y, int width);
	void paintVMeter(QPainter &painter, int x, int y, int width, int height, float magnitude, float peak,
			 float peakHold);
	void paintVTicks(QPainter &painter, int x, int y, int height);

	QMutex dataMutex;

	bool recalculateLayout = true;
	uint64_t currentLastUpdateTime = 0;
	float currentMagnitude[MAX_AUDIO_CHANNELS];
	float currentPeak[MAX_AUDIO_CHANNELS];
	float currentInputPeak[MAX_AUDIO_CHANNELS];

	int displayNrAudioChannels = 0;
	float displayMagnitude[MAX_AUDIO_CHANNELS];
	float displayPeak[MAX_AUDIO_CHANNELS];
	float displayPeakHold[MAX_AUDIO_CHANNELS];
	uint64_t displayPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];
	float displayInputPeakHold[MAX_AUDIO_CHANNELS];
	uint64_t displayInputPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];

	QFont tickFont;
	QColor backgroundNominalColor;
	QColor backgroundWarningColor;
	QColor backgroundErrorColor;
	QColor foregroundNominalColor;
	QColor foregroundWarningColor;
	QColor foregroundErrorColor;

	QColor backgroundNominalColorDisabled;
	QColor backgroundWarningColorDisabled;
	QColor backgroundErrorColorDisabled;
	QColor foregroundNominalColorDisabled;
	QColor foregroundWarningColorDisabled;
	QColor foregroundErrorColorDisabled;

	QColor clipColor;
	QColor magnitudeColor;
	QColor majorTickColor;
	QColor minorTickColor;

	int meterThickness;
	qreal meterFontScaling;

	qreal minimumLevel;
	qreal warningLevel;
	qreal errorLevel;
	qreal clipLevel;
	qreal minimumInputLevel;
	qreal peakDecayRate;
	qreal magnitudeIntegrationTime;
	qreal peakHoldDuration;
	qreal inputPeakHoldDuration;

	QColor p_backgroundNominalColor;
	QColor p_backgroundWarningColor;
	QColor p_backgroundErrorColor;
	QColor p_foregroundNominalColor;
	QColor p_foregroundWarningColor;
	QColor p_foregroundErrorColor;

	uint64_t lastRedrawTime = 0;
	int channels = 0;
	bool clipping = false;
	bool vertical;
	bool muted = false;

public:
	explicit VolumeMeter(QWidget *parent = nullptr, obs_volmeter_t *obs_volmeter = nullptr, bool vertical = false);
	~VolumeMeter();

	void setLevels(const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS],
		       const float inputPeak[MAX_AUDIO_CHANNELS]);
	QRect getBarRect() const;
	bool needLayoutChange();

	QColor getBackgroundNominalColor() const;
	void setBackgroundNominalColor(QColor c);
	QColor getBackgroundWarningColor() const;
	void setBackgroundWarningColor(QColor c);
	QColor getBackgroundErrorColor() const;
	void setBackgroundErrorColor(QColor c);
	QColor getForegroundNominalColor() const;
	void setForegroundNominalColor(QColor c);
	QColor getForegroundWarningColor() const;
	void setForegroundWarningColor(QColor c);
	QColor getForegroundErrorColor() const;
	void setForegroundErrorColor(QColor c);

	QColor getBackgroundNominalColorDisabled() const;
	void setBackgroundNominalColorDisabled(QColor c);
	QColor getBackgroundWarningColorDisabled() const;
	void setBackgroundWarningColorDisabled(QColor c);
	QColor getBackgroundErrorColorDisabled() const;
	void setBackgroundErrorColorDisabled(QColor c);
	QColor getForegroundNominalColorDisabled() const;
	void setForegroundNominalColorDisabled(QColor c);
	QColor getForegroundWarningColorDisabled() const;
	void setForegroundWarningColorDisabled(QColor c);
	QColor getForegroundErrorColorDisabled() const;
	void setForegroundErrorColorDisabled(QColor c);

	QColor getClipColor() const;
	void setClipColor(QColor c);
	QColor getMagnitudeColor() const;
	void setMagnitudeColor(QColor c);
	QColor getMajorTickColor() const;
	void setMajorTickColor(QColor c);
	QColor getMinorTickColor() const;
	void setMinorTickColor(QColor c);
	int getMeterThickness() const;
	void setMeterThickness(int v);
	qreal getMeterFontScaling() const;
	void setMeterFontScaling(qreal v);
	qreal getMinimumLevel() const;
	void setMinimumLevel(qreal v);
	qreal getWarningLevel() const;
	void setWarningLevel(qreal v);
	qreal getErrorLevel() const;
	void setErrorLevel(qreal v);
	qreal getClipLevel() const;
	void setClipLevel(qreal v);
	qreal getMinimumInputLevel() const;
	void setMinimumInputLevel(qreal v);
	qreal getPeakDecayRate() const;
	void setPeakDecayRate(qreal v);
	qreal getMagnitudeIntegrationTime() const;
	void setMagnitudeIntegrationTime(qreal v);
	qreal getPeakHoldDuration() const;
	void setPeakHoldDuration(qreal v);
	qreal getInputPeakHoldDuration() const;
	void setInputPeakHoldDuration(qreal v);
	void setPeakMeterType(enum obs_peak_meter_type peakMeterType);
	virtual void mousePressEvent(QMouseEvent *event) override;
	virtual void wheelEvent(QWheelEvent *event) override;

protected:
	void paintEvent(QPaintEvent *event) override;
	void changeEvent(QEvent *e) override;
};
