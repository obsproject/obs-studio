#pragma once
#pragma once

#include <Idian/Utils.hpp>

#include <obs.hpp>
#include <widgets/VolumeName.hpp>
#include <components/VolumeSlider.hpp>

#include <QFrame>
#include <QPushButton>
#include <QPushButton>

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
	Preview = 1 << 6,
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
	std::unique_ptr<idian::Utils> utils;

	OBSWeakSource weakSource_;
	const char *uuid;
	std::vector<OBSSignal> sigs;

	QBoxLayout *mainLayout;
	QLabel *categoryLabel;
	VolumeName *nameButton;
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
	static void obsSourceDestroy(void *data, calldata_t *params);

	void setLayoutVertical(bool vertical);
	void showVolumeControlMenu(QPoint pos = QPoint(0, 0));
	void updateCategoryLabel();
	void updateDecayRate();
	void updatePeakMeterType();

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

	void handleSourceDestroyed() { deleteLater(); }

signals:
	void unhideAll();

public:
	explicit VolControl(obs_source_t *source, bool vertical = false, QWidget *parent = nullptr);
	~VolControl();

	inline OBSWeakSource weakSource() const { return weakSource_; }
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

	bool isVertical() const { return vertical; }
	void setVertical(bool vertical);

	void updateTabOrder();
	QWidget *firstWidget() const { return qobject_cast<QWidget *>(nameButton); }
	QWidget *lastWidget() const
	{
		return vertical ? qobject_cast<QWidget *>(monitorButton) : qobject_cast<QWidget *>(slider);
	}

	void updateName();
	void refreshColors();
	void debugHideControls();
	void setLevels(const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS],
		       const float inputPeak[MAX_AUDIO_CHANNELS]);
};
