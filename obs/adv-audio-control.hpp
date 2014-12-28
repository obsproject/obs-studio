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
	QCheckBox              *mediaChannel1          = nullptr;
	QCheckBox              *mediaChannel2          = nullptr;
	QCheckBox              *mediaChannel3          = nullptr;
	QCheckBox              *mediaChannel4          = nullptr;

	OBSSignal              volChangedSignal;
	OBSSignal              syncOffsetSignal;
	OBSSignal              flagsSignal;

	static void OBSSourceFlagsChanged(void *param, calldata_t *calldata);
	static void OBSSourceVolumeChanged(void *param, calldata_t *calldata);
	static void OBSSourceSyncChanged(void *param, calldata_t *calldata);

public:
	OBSAdvAudioCtrl(obs_source_t *source_);

	inline obs_source_t *GetSource() const {return source;}

public slots:
	void SourceFlagsChanged(uint32_t flags);
	void SourceVolumeChanged(float volume);
	void SourceSyncChanged(int64_t offset);

	void volumeChanged(int percentage);
	void downmixMonoChanged(bool checked);
	void panningChanged(int val);
	void syncOffsetChanged(int milliseconds);
	void mediaChannel1Changed(bool checked);
	void mediaChannel2Changed(bool checked);
	void mediaChannel3Changed(bool checked);
	void mediaChannel4Changed(bool checked);
};
