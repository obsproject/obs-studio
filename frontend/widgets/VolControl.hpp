#pragma once

#include <Idian/Utils.hpp>

#include <obs.hpp>

#include <QFrame>

class VolumeName;
class VolumeMeter;
class VolumeSlider;
class MuteCheckBox;
class QLabel;
class QPushButton;

namespace OBS {
enum class MixerStatus : uint32_t {
	None = 0,
	Active = 1 << 0,
	Locked = 1 << 1,
	Global = 1 << 2,
	Pinned = 1 << 3,
	Hidden = 1 << 4,
	Unassigned = 1 << 5,
};

inline MixerStatus operator|(MixerStatus a, MixerStatus b)
{
	return static_cast<MixerStatus>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline MixerStatus operator&(MixerStatus a, MixerStatus b)
{
	return static_cast<MixerStatus>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

inline MixerStatus operator~(MixerStatus a)
{
	return static_cast<MixerStatus>(~static_cast<uint32_t>(a));
}
} // namespace OBS

class VolControl : public QFrame {
	Q_OBJECT

private:
	idian::Utils *utils;

	OBSSource source;
	QString uuid;
	std::vector<OBSSignal> sigs;
	QLabel *categoryLabel;
	VolumeName *nameLabel;
	QLabel *volLabel;
	VolumeMeter *volMeter;
	VolumeSlider *slider;
	QPushButton *muteButton;
	QPushButton *monitorButton;

	float levelTotal;
	float levelCount;
	OBSFader obs_fader;

	QString sourceName;
	bool vertical;

	OBS::MixerStatus statusCategory;

	QMenu *contextMenu;

	static void obsVolumeChanged(void *param, float db);
	static void obsVolumeMuted(void *data, calldata_t *calldata);
	static void obsMixersOrMonitoringChanged(void *data, calldata_t *);
	static void obsSourceActivated(void *data, calldata_t *params);
	static void obsSourceDeactivated(void *data, calldata_t *params);

	void showVolumeControlMenu();
	void updateCategoryLabel();

	void setMuted(bool mute);
	void setMonitoring(obs_monitoring_type type);

public slots:
	void sourceActiveChanged(bool active);
	void setUseDisabledColors(bool greyscale);
	void setLocked(bool locked);
	void updateMixerState();

private slots:
	void renameSource();
	void changeVolume();

	void handleMuteButton(bool checked);
	void handleMonitorButton(bool checked);
	void sliderChanged(int vol);
	void updateText();
	void setName(QString name);

signals:
	void unhideAll();

public:
	explicit VolControl(OBSSource source, bool showConfig = false, bool vertical = false);
	~VolControl();

	inline obs_source_t *getSource() const { return source; }
	QString const &getCachedName() const { return sourceName; }

	void setMeterDecayRate(qreal q);
	void setPeakMeterType(enum obs_peak_meter_type peakMeterType);

	void enableSlider(bool enable);
	void setMixerFlag(OBS::MixerStatus category, bool enable);
	bool hasMixerFlag(OBS::MixerStatus category);
	void setGlobalInMixer(bool global);
	void setPinnedInMixer(bool pinned);
	void setHideInMixer(bool hidden);
	void setContextMenu(QMenu *cm) { contextMenu = cm; }

	void updateName();
	void refreshColors();
	void debugHideControls();
	void setLevels(const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS],
		       const float inputPeak[MAX_AUDIO_CHANNELS]);
};
