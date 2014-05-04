#pragma once

#include <obs.hpp>
#include <QWidget>

/* TODO: Make a real volume control that isn't terrible */

class QLabel;
class QSlider;

class VolControl : public QWidget {
	Q_OBJECT

private:
	OBSSource source;
	QLabel    *nameLabel;
	QLabel    *volLabel;
	QSlider   *slider;
	bool      signalChanged;

	static void OBSVolumeChanged(void *param, calldata_t calldata);

private slots:
	void VolumeChanged(int vol);
	void SliderChanged(int vol);

public:
	VolControl(OBSSource source);
	~VolControl();

	inline obs_source_t GetSource() const {return source;}
};
