#include "AudioMixer.h"

AudioMixer::AudioMixer(QWidget *parent) : QWidget(parent)
{
	m_layout = new QVBoxLayout(this);
	setLayout(m_layout);

	// Add some dummy channels for now
	addChannel("Desktop Audio", 80);
	addChannel("Mic / Aux", 100);
	addChannel("Music", 40);

	m_layout->addStretch();
}

void AudioMixer::addChannel(const QString &name, int volume)
{
	AudioChannelWidget *ch = new AudioChannelWidget(name, volume, this);
	m_layout->addWidget(ch);
}

// --- Channel Widget ---

AudioChannelWidget::AudioChannelWidget(const QString &name, int volume, QWidget *parent) : QWidget(parent)
{
	QHBoxLayout *layout = new QHBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);

	m_label = new QLabel(name, this);
	m_label->setFixedWidth(100);

	m_slider = new QSlider(Qt::Horizontal, this);
	m_slider->setRange(0, 100);
	m_slider->setValue(volume);

	m_valLabel = new QLabel(QString::number(volume) + "%", this);
	m_valLabel->setFixedWidth(40);

	layout->addWidget(m_label);
	layout->addWidget(m_slider);
	layout->addWidget(m_valLabel);

	connect(m_slider, &QSlider::valueChanged, this, [this](int val) {
		m_valLabel->setText(QString::number(val) + "%");
		// TODO: IPC send volume
	});
}
