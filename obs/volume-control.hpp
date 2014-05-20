#pragma once

#include <obs.hpp>
#include <QWidget>
#include <QProgressBar>

/* TODO: Make a real volume control that isn't terrible */

class QLabel;
class QSlider;

class VolControl : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	QLabel          *nameLabel;
	QLabel          *volLabel;
	QProgressBar    *volMeter;
	QSlider         *slider;
	bool            signalChanged;

	static void OBSVolumeChanged(void *param, calldata_t calldata);
	static void OBSVolumeLevel(void *data, calldata_t calldata);

private slots:
	void VolumeChanged(int vol);
	void VolumeLevel(int vol);
	void SliderChanged(int vol);

public:
	VolControl(OBSSource source);
	~VolControl();

	inline obs_source_t GetSource() const {return source;}
};
