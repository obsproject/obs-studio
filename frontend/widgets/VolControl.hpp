#pragma once

#include <obs.hpp>

#include <QFrame>

class OBSSourceLabel;
class VolumeMeter;
class VolumeSlider;
class MuteCheckBox;
class QCheckBox;
class QLabel;
class QPushButton;
class QGridLayout;
class OBSSourceLabel;

class VolControl : public QFrame {
	Q_OBJECT

private:
	QScopedPointer<QFrame> controlContainer;

	OBSSource source;
	std::vector<OBSSignal> sigs;
	QGridLayout *controlLayout;
	OBSSourceLabel *nameLabel;
	QScopedPointer<QLabel> volLabel;
	QScopedPointer<VolumeMeter> volMeter;
	QScopedPointer<VolumeSlider> slider;
	QScopedPointer<MuteCheckBox> mute;
	QScopedPointer<QPushButton> config;

	float levelTotal;
	float levelCount;
	OBSFader obs_fader;
	OBSVolMeter obs_volmeter;
	bool vertical = false;
	QMenu *contextMenu;
	enum obs_peak_meter_type peakMeterType = SAMPLE_PEAK_METER;
	qreal peakDecayRate;
	bool sliderEnabled = true;

	static void OBSVolumeChanged(void *param, float db);
	static void OBSVolumeLevel(void *data, const float magnitude[MAX_AUDIO_CHANNELS],
				   const float peak[MAX_AUDIO_CHANNELS], const float inputPeak[MAX_AUDIO_CHANNELS]);
	static void OBSAudioChanged(void *data, calldata_t *);

	void EmitConfigClicked();

private slots:
	void VolumeChanged();
	void AudioChanged();

	void SetMuted(bool checked);
	void SliderChanged(int vol);
	void updateText();
	void ResetControls(bool visible);

signals:
	void ConfigClicked();

public:
	explicit VolControl(OBSSource source, bool vertical = false, bool hidden = false);
	~VolControl();

	inline obs_source_t *GetSource() const { return source; }

	void SetMeterDecayRate(qreal q);
	void setPeakMeterType(enum obs_peak_meter_type peakMeterType);

	void EnableSlider(bool enable);
	inline void SetContextMenu(QMenu *cm) { contextMenu = cm; }

	void refreshColors();
};
