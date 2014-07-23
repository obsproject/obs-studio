#pragma once

#include <obs.hpp>
#include <QWidget>

class VolumeMeter : public QWidget
{
	Q_OBJECT
private:
	float mag, peak, peakHold;
	QColor bkColor, magColor, peakColor, peakHoldColor;
	QTimer *resetTimer;
	
public:
	explicit VolumeMeter(QWidget *parent = 0);
	void setLevels(float nmag, float npeak, float npeakHold);
protected:
	void paintEvent(QPaintEvent *event);
private slots:
	void resetState();
};

class QLabel;
class QSlider;

class VolControl : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	QLabel          *nameLabel;
	QLabel          *volLabel;
	VolumeMeter     *volMeter;
	QSlider         *slider;
	bool            signalChanged;
	uint64_t        lastMeterTime;
	float           levelTotal;
	float           levelCount;

	static void OBSVolumeChanged(void *param, calldata_t calldata);
	static void OBSVolumeLevel(void *data, calldata_t calldata);

private slots:
	void VolumeChanged(int vol);
	void VolumeLevel(float mag, float peak, float peakHold);
	void SliderChanged(int vol);

public:
	VolControl(OBSSource source);
	~VolControl();

	inline obs_source_t GetSource() const {return source;}

	QString GetName() const;
	void SetName(const QString &newName);
};
