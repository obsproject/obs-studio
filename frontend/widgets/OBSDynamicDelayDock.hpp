#pragma once

#include <docks/OBSDock.hpp>
#include <obs.hpp>
#include <QPointer>
#include <QTimer>

class QLabel;
class QLineEdit;
class QPushButton;
class QSlider;
class QSpinBox;
class QProgressBar;

class OBSDynamicDelayDock : public OBSDock {
	Q_OBJECT

	QLineEdit *mediaPathEdit = nullptr;
	QPushButton *browseBtn = nullptr;
	QSlider *targetSlider = nullptr;
	QSpinBox *targetSpinBox = nullptr;
	QPushButton *toggleBtn = nullptr;
	QLabel *statusLabel = nullptr;
	QLabel *bufferedLabel = nullptr;
	QLabel *memoryLabel = nullptr;
	QProgressBar *progressBar = nullptr;

	QTimer updateTimer;

public:
	OBSDynamicDelayDock(QWidget *parent = nullptr);
	~OBSDynamicDelayDock();

private slots:
	void OnBrowseMedia();
	void OnTargetDelayChanged(int value);
	void OnToggleDelay();
	void UpdateStats();
};
