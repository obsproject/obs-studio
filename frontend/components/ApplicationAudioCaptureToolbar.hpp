#pragma once

#include "ComboSelectToolbar.hpp"

class ApplicationAudioCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	ApplicationAudioCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};
