#include "VideoModule.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "VRCommon.h"

VideoModule::VideoModule(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);

	// Header
	QLabel *title = new QLabel("Video Source", this);
	title->setStyleSheet("font-weight: bold; color: " + VRTheme::ACCENT_COLOR + ";");
	layout->addWidget(title);

	// File Selection
	m_filePicker = new VRFilePicker("Select Video File...", this);
	layout->addWidget(m_filePicker);

	// Preview Placeholder
	QLabel *preview = new QLabel("[Video Preview Frame]", this);
	preview->setAlignment(Qt::AlignCenter);
	preview->setStyleSheet("background: black; colow: #555; border: 1px solid #333;");
	preview->setMinimumHeight(200);
	layout->addWidget(preview);

	// Controls
	QHBoxLayout *controls = new QHBoxLayout();
	m_playBtn = new VRButton("Play", this);
	m_stopBtn = new VRButton("Stop", this);
	controls->addWidget(m_playBtn);
	controls->addWidget(m_stopBtn);
	layout->addLayout(controls);

	// Seeker
	m_seekSlider = new VRSlider(Qt::Horizontal, this);
	layout->addWidget(m_seekSlider);

	layout->addStretch();
}
