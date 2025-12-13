#pragma once

#include "ComboSelectToolbar.hpp"

class AudioCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	AudioCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};
