#include "ExtraModules.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include "VRCommon.h"
#include "VRButton.h"

// --- PhotoModule ---
PhotoModule::PhotoModule(QWidget *parent) : QWidget(parent)
{
	auto layout = new QVBoxLayout(this);

	auto title = new QLabel("Photo/Image Source", this);
	title->setStyleSheet("color: white; font-weight: bold; font-size: 14px;");
	layout->addWidget(title);

	layout->addWidget(new VRFilePicker("Select Image...", this));

	// Brightness/Contrast stubs
	layout->addWidget(new QLabel("Brightness", this));
	layout->addWidget(new VRSlider(Qt::Horizontal, this));

	layout->addStretch();
}

// --- StreamModule ---
StreamModule::StreamModule(QWidget *parent) : QWidget(parent)
{
	auto layout = new QVBoxLayout(this);

	auto title = new QLabel("Incoming Stream (SRT/RTMP)", this);
	title->setStyleSheet("color: white; font-weight: bold; font-size: 14px;");
	layout->addWidget(title);

	auto urlInput = new QLineEdit(this);
	urlInput->setPlaceholderText("srt://ip:port or rtmp://server/key");
	urlInput->setStyleSheet("background: " + VRTheme::color("bg_light") +
				"; color: white; padding: 5px; border: 1px solid #555;");
	layout->addWidget(urlInput);

	auto connectBtn = new VRButton("Connect Stream", this);
	layout->addWidget(connectBtn);

	layout->addStretch();
}
