#pragma once

#include <obs.hpp>
#include <QWidget>

class QLabel;
class QSpinBox;
class QCheckBox;
class QSlider;

class OBSAdvAudioCtrl : public QWidget {
	Q_OBJECT

private:
	OBSSource              source;
	QLabel                 *nameLabel              = nullptr;
	QSpinBox               *volume                 = nullptr;
	QCheckBox              *forceMono              = nullptr;
	QSlider                *panning                = nullptr;
	QSpinBox               *syncOffset             = nullptr;
	QCheckBox              *mixer1                 = nullptr;
	QCheckBox              *mixer2                 = nullptr;
	QCheckBox              *mixer3                 = nullptr;
	QCheckBox              *mixer4                 = nullptr;

	OBSSignal              volChangedSignal;
	OBSSignal              syncOffsetSignal;
	OBSSignal              flagsSignal;
	OBSSignal              mixersSignal;

	static void OBSSourceFlagsChanged(void *param, calldata_t *calldata);
	static void OBSSourceVolumeChanged(void *param, calldata_t *calldata);
	static void OBSSourceSyncChanged(void *param, calldata_t *calldata);
	static void OBSSourceMixersChanged(void *param, calldata_t *calldata);

public:
	OBSAdvAudioCtrl(obs_source_t *source_);

	inline obs_source_t *GetSource() const {return source;}

public slots:
	void SourceFlagsChanged(uint32_t flags);
	void SourceVolumeChanged(float volume);
	void SourceSyncChanged(int64_t offset);
	void SourceMixersChanged(uint32_t mixers);

	void volumeChanged(int percentage);
	void downmixMonoChanged(bool checked);
	void panningChanged(int val);
	void syncOffsetChanged(int milliseconds);
	void mixer1Changed(bool checked);
	void mixer2Changed(bool checked);
	void mixer3Changed(bool checked);
	void mixer4Changed(bool checked);
};
