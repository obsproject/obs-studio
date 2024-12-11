#pragma once

#include <obs.hpp>

#include <QFrame>

class OBSSourceLabel;
class VolumeMeter;
class VolumeSlider;
class MuteCheckBox;
class QLabel;
class QPushButton;

class VolControl : public QFrame {
	Q_OBJECT

private:
	OBSSource source;
	std::vector<OBSSignal> sigs;
	OBSSourceLabel *nameLabel;
	QLabel *volLabel;
	VolumeMeter *volMeter;
	VolumeSlider *slider;
	MuteCheckBox *mute;
	QPushButton *config = nullptr;
	float levelTotal;
	float levelCount;
	OBSFader obs_fader;
	OBSVolMeter obs_volmeter;
	bool vertical;
	QMenu *contextMenu;

	static void OBSVolumeChanged(void *param, float db);
	static void OBSVolumeLevel(void *data, const float magnitude[MAX_AUDIO_CHANNELS],
				   const float peak[MAX_AUDIO_CHANNELS], const float inputPeak[MAX_AUDIO_CHANNELS]);
	static void OBSVolumeMuted(void *data, calldata_t *calldata);
	static void OBSMixersOrMonitoringChanged(void *data, calldata_t *);

	void EmitConfigClicked();

private slots:
	void VolumeChanged();
	void VolumeMuted(bool muted);
	void MixersOrMonitoringChanged();

	void SetMuted(bool checked);
	void SliderChanged(int vol);
	void updateText();

signals:
	void ConfigClicked();

public:
	explicit VolControl(OBSSource source, bool showConfig = false, bool vertical = false);
	~VolControl();

	inline obs_source_t *GetSource() const { return source; }

	void SetMeterDecayRate(qreal q);
	void setPeakMeterType(enum obs_peak_meter_type peakMeterType);

	void EnableSlider(bool enable);
	inline void SetContextMenu(QMenu *cm) { contextMenu = cm; }

	void refreshColors();
};
