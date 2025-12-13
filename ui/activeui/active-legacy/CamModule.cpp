#include "CamModule.h"
#include <QVBoxLayout>
#include <QLabel>
#include "VRCommon.h"

CamModule::CamModule(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);

	QLabel *title = new QLabel("Camera Source", this);
	title->setStyleSheet("font-weight: bold; color: " + VRTheme::ACCENT_COLOR + ";");
	layout->addWidget(title);

	// Device Selection
	m_deviceList = new QComboBox(this);
	m_deviceList->addItem("Webcam 1 (/dev/video0)");
	m_deviceList->addItem("Capture Card (/dev/video2)");
	layout->addWidget(m_deviceList);

	// Preview
	QLabel *preview = new QLabel("[Camera Feed]", this);
	preview->setAlignment(Qt::AlignCenter);
	preview->setStyleSheet("background: black; color: #555; border: 1px solid #333;");
	preview->setMinimumHeight(200);
	layout->addWidget(preview);

	m_activateBtn = new VRButton("Activate", this);
	layout->addWidget(m_activateBtn);

	layout->addStretch();
}
