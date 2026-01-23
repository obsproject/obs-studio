#pragma once

#include <obs.hpp>

#include <components/VolumeName.hpp>
#include <components/VolumeSlider.hpp>

#include <Idian/Utils.hpp>
#include <QFrame>
#include <QPushButton>
#include <QWidget>

class MuteCheckBox;
class QBoxLayout;
class QLabel;
class VolumeMeter;

class VolumeControl : public QFrame {
	Q_OBJECT

public:
	struct MixerStatus {
		enum Value : uint32_t {
			None = 0,
			Active = 1 << 0,
			Locked = 1 << 1,
			Global = 1 << 2,
			Pinned = 1 << 3,
			Hidden = 1 << 4,
			Unassigned = 1 << 5,
			Preview = 1 << 6,
		};

		MixerStatus() = default;
		MixerStatus(Value v) : bits(v) {}

		bool has(Value v) const { return (bits & v) != 0; }
		void set(Value v, bool enable)
		{
			if (enable) {
				bits |= v;
			} else {
				bits &= ~v;
			}
		}

		MixerStatus operator|(Value v) const { return MixerStatus(bits | v); }
		MixerStatus &operator|=(Value v)
		{
			bits |= v;
			return *this;
		}

		MixerStatus operator&(Value v) const { return MixerStatus(bits & v); }
		MixerStatus &operator&=(Value v)
		{
			bits &= v;
			return *this;
		}

		MixerStatus operator~() const { return MixerStatus(~bits); }

	private:
		uint32_t bits = None;
		explicit MixerStatus(uint32_t v) : bits(v) {}
	};

private:
	std::unique_ptr<idian::Utils> utils;

	OBSWeakSource weakSource_;
	const char *uuid;
	std::vector<OBSSignal> obsSignals;

	QBoxLayout *mainLayout;
	QLabel *categoryLabel;
	VolumeName *nameButton;
	QLabel *volumeLabel;
	VolumeMeter *volumeMeter;
	VolumeSlider *slider;
	QPushButton *muteButton;
	QPushButton *monitorButton;

	OBSFader obs_fader;

	QString sourceName;
	bool vertical;

	MixerStatus mixerStatus_;

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
	explicit VolumeControl(obs_source_t *source, QWidget *parent, bool vertical);
	explicit VolumeControl(obs_source_t *source, QWidget *parent) : VolumeControl(source, parent, false) {}
	~VolumeControl();

	inline OBSWeakSource weakSource() const { return weakSource_; }
	QString const &getCachedName() const { return sourceName; }

	void setMeterDecayRate(qreal q);
	void setPeakMeterType(enum obs_peak_meter_type peakMeterType);

	void enableSlider(bool enable);
	MixerStatus &mixerStatus() { return mixerStatus_; }
	void setGlobalInMixer(bool global);
	void setPinnedInMixer(bool pinned);
	void setHiddenInMixer(bool hidden);
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
	void setLevels(const float magnitude[MAX_AUDIO_CHANNELS], const float peak[MAX_AUDIO_CHANNELS],
		       const float inputPeak[MAX_AUDIO_CHANNELS]);
};
