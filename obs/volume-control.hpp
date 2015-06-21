#pragma once

#include <obs.hpp>
#include <QWidget>

class QPushButton;

class VolumeMeter : public QWidget
{
	Q_OBJECT
	Q_PROPERTY(QColor bkColor READ getBkColor WRITE setBkColor DESIGNABLE true)
	Q_PROPERTY(QColor magColor READ getMagColor WRITE setMagColor DESIGNABLE true)
	Q_PROPERTY(QColor peakColor READ getPeakColor WRITE setPeakColor DESIGNABLE true)
	Q_PROPERTY(QColor peakHoldColor READ getPeakHoldColor WRITE setPeakHoldColor DESIGNABLE true)

private:
	float mag, peak, peakHold;
	QColor bkColor, magColor, peakColor, peakHoldColor;
	QTimer *resetTimer;

public:
	explicit VolumeMeter(QWidget *parent = 0);
	void setLevels(float nmag, float npeak, float npeakHold);
	QColor getBkColor() const;
	void setBkColor(QColor c);
	QColor getMagColor() const;
	void setMagColor(QColor c);
	QColor getPeakColor() const;
	void setPeakColor(QColor c);
	QColor getPeakHoldColor() const;
	void setPeakHoldColor(QColor c);

protected:
	void paintEvent(QPaintEvent *event);
private slots:
	void resetState();
};

class QLabel;
class QSlider;
class MuteCheckBox;

class VolControl : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	QLabel          *nameLabel;
	QLabel          *volLabel;
	VolumeMeter     *volMeter;
	QSlider         *slider;
	MuteCheckBox    *mute;
	QPushButton     *config = nullptr;
	float           levelTotal;
	float           levelCount;
	obs_fader_t     *obs_fader;
	obs_volmeter_t  *obs_volmeter;

	static void OBSVolumeChanged(void *param, calldata_t *calldata);
	static void OBSVolumeLevel(void *data, calldata_t *calldata);
	static void OBSVolumeMuted(void *data, calldata_t *calldata);

	void EmitConfigClicked();

private slots:
	void VolumeChanged();
	void VolumeMuted(bool muted);
	void VolumeLevel(float mag, float peak, float peakHold, bool muted);

	void SetMuted(bool checked);
	void SliderChanged(int vol);
	void updateText();

signals:
	void ConfigClicked();

public:
	VolControl(OBSSource source, bool showConfig = false);
	~VolControl();

	inline obs_source_t *GetSource() const {return source;}

	QString GetName() const;
	void SetName(const QString &newName);
};
