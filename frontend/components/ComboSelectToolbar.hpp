#pragma once

#include "SourceToolbar.hpp"

class Ui_DeviceSelectToolbar;

class ComboSelectToolbar : public SourceToolbar {
	Q_OBJECT

protected:
	std::unique_ptr<Ui_DeviceSelectToolbar> ui;
	const char *prop_name;
	bool is_int = false;

public:
	ComboSelectToolbar(QWidget *parent, OBSSource source);
	~ComboSelectToolbar();
	virtual void Init();

public slots:
	void on_device_currentIndexChanged(int idx);
};
