#pragma once

#include "ui_OBSBasicTransitionAudio.h"

#include "balance-slider.hpp"
#include <obs.hpp>

class OBSBasic;

class OBSBasicTransitionAudio : public QDialog {
	Q_OBJECT

private:
	OBSBasic *main;
	std::unique_ptr<Ui::OBSBasicTransitionAudio> ui;
	OBSSource source;

	OBSSignal sourceUpdateSignal;
	static void OBSSourceUpdated(void *param, calldata_t *calldata);

	enum class VolumeType {
		dB,
		Percent,
	};
	void SetVolumeType(VolumeType type);

public:
	OBSBasicTransitionAudio(QWidget *parent, OBSSource source_);
	void Init();

public slots:
	void on_usePercent_toggled(bool checked);

	void SourceUpdated();

	void volumeChanged(double db);
	void percentChanged(int percent);
	void downmixMonoChanged(bool checked);
	void balanceChanged(int val);
	void syncOffsetChanged(int milliseconds);
	void monitoringTypeChanged(int index);
	void mixer1Changed(bool checked);
	void mixer2Changed(bool checked);
	void mixer3Changed(bool checked);
	void mixer4Changed(bool checked);
	void mixer5Changed(bool checked);
	void mixer6Changed(bool checked);
	void ResetBalance();
};
