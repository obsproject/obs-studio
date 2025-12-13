#pragma once

#include "ComboSelectToolbar.hpp"

class WindowCaptureToolbar : public ComboSelectToolbar {
	Q_OBJECT

public:
	WindowCaptureToolbar(QWidget *parent, OBSSource source);
	void Init() override;
};
