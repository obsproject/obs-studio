#include "ThreeDModule.h"
#include <QVBoxLayout>
#include <QLabel>
#include "VRCommon.h"

ThreeDModule::ThreeDModule(QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *layout = new QVBoxLayout(this);

	QLabel *title = new QLabel("3D Mesh Source", this);
	title->setStyleSheet("font-weight: bold; color: " + VRTheme::ACCENT_COLOR + ";");
	layout->addWidget(title);

	m_meshPicker = new VRFilePicker("Select .obj Mesh...", this);
	layout->addWidget(m_meshPicker);

	// 3D Viewport Placeholder
	QLabel *viewport = new QLabel("[3D Viewport]", this);
	viewport->setAlignment(Qt::AlignCenter);
	viewport->setStyleSheet("background: #111; color: #777; border: 1px solid #444;");
	viewport->setMinimumHeight(200);
	layout->addWidget(viewport);

	// Transforms
	layout->addWidget(new QLabel("Scale:", this));
	m_scaleSlider = new VRSlider(Qt::Horizontal, this);
	layout->addWidget(m_scaleSlider);

	layout->addWidget(new QLabel("Rotation Y:", this));
	m_rotSlider = new VRSlider(Qt::Horizontal, this);
	layout->addWidget(m_rotSlider);

	layout->addStretch();
}
