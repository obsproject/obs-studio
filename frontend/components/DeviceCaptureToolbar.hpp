#pragma once

#include <obs.hpp>

#include <QWidget>

class Ui_DeviceSelectToolbar;

class DeviceCaptureToolbar : public QWidget {
	Q_OBJECT

	OBSWeakSource weakSource;

	std::unique_ptr<Ui_DeviceSelectToolbar> ui;
	const char *activateText;
	const char *deactivateText;
	bool active;

public:
	DeviceCaptureToolbar(QWidget *parent, OBSSource source);
	~DeviceCaptureToolbar();

public slots:
	void on_activateButton_clicked();
};
