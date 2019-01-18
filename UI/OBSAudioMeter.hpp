#pragma once

#include <obs.hpp>
#include <QWidget>
#include <QPainter>
#include <QPaintEvent>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>
#include <QSlider>

class OBSAudioMeter : public QWidget {
	Q_OBJECT
	Q_PROPERTY(QColor backgroundNominalColor
		READ getBackgroundNominalColor
		WRITE setBackgroundNominalColor DESIGNABLE true)
	Q_PROPERTY(QColor backgroundWarningColor
		READ getBackgroundWarningColor
		WRITE setBackgroundWarningColor DESIGNABLE true)
	Q_PROPERTY(QColor backgroundErrorColor
		READ getBackgroundErrorColor
		WRITE setBackgroundErrorColor DESIGNABLE true)

	Q_PROPERTY(QColor foregroundNominalColor
		READ getForegroundNominalColor
		WRITE setForegroundNominalColor DESIGNABLE true)
	Q_PROPERTY(QColor foregroundWarningColor
		READ getForegroundWarningColor
		WRITE setForegroundWarningColor DESIGNABLE true)
	Q_PROPERTY(QColor foregroundErrorColor
		READ getForegroundErrorColor
		WRITE setForegroundErrorColor DESIGNABLE true)

	Q_PROPERTY(QColor magnitudeColor
		READ getMagnitudeColor
		WRITE setMagnitudeColor DESIGNABLE true)

	Q_PROPERTY(QColor clipColor
		READ getClipColor
		WRITE setClipColor DESIGNABLE true)

	Q_PROPERTY(QColor majorTickColor
		READ getMajorTickColor
		WRITE setMajorTickColor DESIGNABLE true)
	Q_PROPERTY(QColor minorTickColor
		READ getMinorTickColor
		WRITE setMinorTickColor DESIGNABLE true)

	Q_PROPERTY(qreal clipLevel
		READ getClipLevel
		WRITE setClipLevel DESIGNABLE true)

	// Levels are denoted in dBFS.
	Q_PROPERTY(qreal minimumLevel
		READ getMinimumLevel
		WRITE setMinimumLevel DESIGNABLE true)
	Q_PROPERTY(qreal warningLevel
		READ getWarningLevel
		WRITE setWarningLevel DESIGNABLE true)
	Q_PROPERTY(qreal errorLevel
		READ getErrorLevel
		WRITE setErrorLevel DESIGNABLE true)
	Q_PROPERTY(qreal clipLevel
		READ getClipLevel
		WRITE setClipLevel DESIGNABLE true)

	// Rates are denoted in dB/second.
	Q_PROPERTY(qreal peakDecayRate
		READ getPeakDecayRate
		WRITE setPeakDecayRate DESIGNABLE true)

	// Time in seconds for the VU meter to integrate over.
	Q_PROPERTY(qreal magnitudeIntegrationTime
		READ getMagnitudeIntegrationTime
		WRITE setMagnitudeIntegrationTime DESIGNABLE true)

	// Duration is denoted in seconds.
	Q_PROPERTY(qreal peakHoldDuration
		READ getPeakHoldDuration
		WRITE setPeakHoldDuration DESIGNABLE true)
	Q_PROPERTY(qreal inputPeakHoldDuration
		READ getInputPeakHoldDuration
		WRITE setInputPeakHoldDuration DESIGNABLE true)
private slots:
	void ClipEnding()
	{
		clipping = false;
	}
public:
	enum tick_location {
		none,
		left,
		right,
		top = left,
		bottom = right
	};
private:
	bool vertical = false;
	bool clipping = false;
	tick_location _tick_opts;
	bool _tick_db;

	qreal minimumInputLevel;
	qreal minimumLevel;
	qreal warningLevel;
	qreal errorLevel;
	qreal clipLevel;

	QColor backgroundNominalColor;
	QColor backgroundWarningColor;
	QColor backgroundErrorColor;

	QColor foregroundNominalColor;
	QColor foregroundWarningColor;
	QColor foregroundErrorColor;

	QColor magnitudeColor;

	QColor clipColor;

	QColor majorTickColor;
	QColor minorTickColor;

	QTimer *redrawTimer;
	uint64_t lastRedrawTime = 0;

	qreal peakDecayRate;
	qreal magnitudeIntegrationTime;
	qreal peakHoldDuration;
	qreal inputPeakHoldDuration;

	QFont tickFont;

	float currentMagnitude[MAX_AUDIO_CHANNELS];
	float currentPeak[MAX_AUDIO_CHANNELS];
	float currentInputPeak[MAX_AUDIO_CHANNELS];

	float displayMagnitude[MAX_AUDIO_CHANNELS];
	float displayPeak[MAX_AUDIO_CHANNELS];
	float displayInputPeak[MAX_AUDIO_CHANNELS];

	float displayPeakHold[MAX_AUDIO_CHANNELS];
	uint64_t displayPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];
	float displayInputPeakHold[MAX_AUDIO_CHANNELS];
	uint64_t displayInputPeakHoldLastUpdateTime[MAX_AUDIO_CHANNELS];

	QPixmap *tickPaintCache = nullptr;

	int _channels = 2;

	obs_source_t *_source = nullptr;
	obs_source_t *_parent = nullptr;

	QMutex dataMutex;

	uint64_t currentLastUpdateTime = 0;

	inline bool detectIdle(uint64_t ts)
	{
		double timeSinceLastUpdate = (ts - currentLastUpdateTime) * 0.000000001;
		if (timeSinceLastUpdate > 0.5) {
			resetLevels();
			return true;
		} else {
			return false;
		}
	}

	inline void handleChannelConfigurationChange()
	{
		QMutexLocker locker(&dataMutex);

		// Make room for 3 pixels meter, with one pixel between each.
		// Then 9/13 pixels for ticks and numbers.
		if (vertical)
			setMinimumSize(_channels * 4 + 14, 130);
		else
			setMinimumSize(130, _channels * 4 + 8);
	}

	void paintHTicks(QPainter &painter, int x, int y, int width,
		int height);
	void paintVTicks(QPainter &painter, int x, int y, int width,
		int height);

	inline void calculateBallistics(uint64_t ts,
		qreal timeSinceLastRedraw = 0.0);
	inline void calculateBallisticsForChannel(int channelNr,
		uint64_t ts, qreal timeSinceLastRedraw);

	void paintInputMeter(QPainter &painter, int x, int y, int width,
		int height, float peakHold);
	void paintVMeter(QPainter &painter, int x, int y, int width,
		int height, float magnitude, float peak, float inputPeak,
		float peakHold, float inputPeakHold);
	void paintHMeter(QPainter &painter, int x, int y, int width,
		int height, float magnitude, float peak, float inputPeak,
		float peakHold, float inputPeakHold);
protected:
	void paintEvent(QPaintEvent *event) override;
	void (*getSample)(OBSAudioMeter *meter) = nullptr;
public slots:
	void sample()
	{
		if(getSample != nullptr)
			getSample(this);
		QWidget::update();
	}
public:
	void setVertical()
	{
		vertical = true;
		handleChannelConfigurationChange();
	}
	void setHorizontal()
	{
		vertical = false;
		handleChannelConfigurationChange();
	}
	void setLayout(bool verticalLayout)
	{
		vertical = verticalLayout;
		handleChannelConfigurationChange();

	}
	void setTickOptions(tick_location location, bool show_db)
	{
		_tick_opts = location;
		_tick_db = show_db;
		handleChannelConfigurationChange();
	}
	template<class _Fn>
	void setCallback(_Fn _func)
	{
		getSample = _func;
	}
	const obs_source_t *getSource()
	{
		return _source;
	}
	void setChannels(int channels)
	{
		_channels = channels;
	}
	void removeCallback()
	{
		getSample = nullptr;
	}
	void resetLevels()
	{
		QMutexLocker locker(&dataMutex);
		//currentLastUpdateTime = 0;
		//lastRedrawTime = 0;
		for (int channelNr = 0; channelNr < MAX_AUDIO_CHANNELS; channelNr++) {
			currentMagnitude[channelNr] = -M_INFINITE;
			currentPeak[channelNr] = -M_INFINITE;
			currentInputPeak[channelNr] = -M_INFINITE;

			displayMagnitude[channelNr] = -M_INFINITE;
			displayPeak[channelNr] = -M_INFINITE;
			displayInputPeak[channelNr] = -M_INFINITE;
			displayPeakHoldLastUpdateTime[channelNr] = 0;

			displayPeakHold[channelNr] = -M_INFINITE;
			displayInputPeakHold[channelNr] = -M_INFINITE;

			displayInputPeakHoldLastUpdateTime[channelNr] = 0;
		}

		locker.unlock();
	}
	void setLevels(const float magnitude[MAX_AUDIO_CHANNELS],
		const float peak[MAX_AUDIO_CHANNELS],
		const float inputPeak[MAX_AUDIO_CHANNELS]);

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
	QColor getClipColor() const;
	void setClipColor(QColor c);
	QColor getMagnitudeColor() const;
	void setMagnitudeColor(QColor c);
	QColor getMajorTickColor() const;
	void setMajorTickColor(QColor c);
	QColor getMinorTickColor() const;
	void setMinorTickColor(QColor c);
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

	OBSAudioMeter(QWidget* parent = Q_NULLPTR,
		Qt::WindowFlags f = Qt::WindowFlags(),
		obs_source_t *source = nullptr) :
		QWidget(parent, f)
	{
		_tick_db = true;
		vertical = true;
		tickFont = QFont("Arial");
		tickFont.setPixelSize(7);

		_source = source;
		_parent = obs_filter_get_parent(_source);

		backgroundNominalColor.setRgb(0x26, 0x7f, 0x26);    // Dark green
		backgroundWarningColor.setRgb(0x7f, 0x7f, 0x26);    // Dark yellow
		backgroundErrorColor.setRgb(0x7f, 0x26, 0x26);      // Dark red
		foregroundNominalColor.setRgb(0x4c, 0xff, 0x4c);    // Bright green
		foregroundWarningColor.setRgb(0xff, 0xff, 0x4c);    // Bright yellow
		foregroundErrorColor.setRgb(0xff, 0x4c, 0x4c);      // Bright red
		clipColor.setRgb(0xff, 0xff, 0xff);                 // Solid white

		majorTickColor.setRgb(0xff, 0xff, 0xff);            // Solid white
		minorTickColor.setRgb(0xcc, 0xcc, 0xcc);            // Black

		minimumInputLevel = -50.0;
		minimumLevel = -60.0;                               // -60 dB
		warningLevel = -20.0;                               // -20 dB
		errorLevel = -9.0;                                  //  -9 dB

		//peakDecayRate = 11.76;                            //  20 dB / 1.7 sec
		peakDecayRate = 25.53;
		magnitudeIntegrationTime = 0.3;                     //  99% in 300 ms
		peakHoldDuration = 5.0;                             //  20 seconds
		inputPeakHoldDuration = 5.0;                        //  20 seconds

		resetLevels();
		redrawTimer = new QTimer(this);
		QObject::connect(redrawTimer, &QTimer::timeout, this, &OBSAudioMeter::sample);
		redrawTimer->start(34);
	};
	~OBSAudioMeter()
	{
		if (redrawTimer)
			redrawTimer->deleteLater();
		removeCallback();
	};
};
