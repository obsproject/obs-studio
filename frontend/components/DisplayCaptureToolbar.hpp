#pragma once

#include "ComboSelectToolbar.hpp"

class DisplayCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	DisplayCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};
