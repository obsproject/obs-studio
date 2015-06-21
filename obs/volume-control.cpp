#include "volume-control.hpp"
#include "qt-wrappers.hpp"
#include "mute-checkbox.hpp"
#include "slider-absoluteset-style.hpp"
#include <util/platform.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QVariant>
#include <QSlider>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <string>
#include <math.h>

using namespace std;

void VolControl::OBSVolumeChanged(void *data, calldata_t *calldata)
{
	Q_UNUSED(calldata);
	VolControl *volControl = static_cast<VolControl*>(data);

	QMetaObject::invokeMethod(volControl, "VolumeChanged");
}

void VolControl::OBSVolumeLevel(void *data, calldata_t *calldata)
{
	VolControl *volControl = static_cast<VolControl*>(data);
	float peak      = calldata_float(calldata, "level");
	float mag       = calldata_float(calldata, "magnitude");
	float peakHold  = calldata_float(calldata, "peak");
	bool  muted     = calldata_bool (calldata, "muted");

	QMetaObject::invokeMethod(volControl, "VolumeLevel",
		Q_ARG(float, mag),
		Q_ARG(float, peak),
		Q_ARG(float, peakHold),
		Q_ARG(bool,  muted));
}

void VolControl::OBSVolumeMuted(void *data, calldata_t *calldata)
{
	VolControl *volControl = static_cast<VolControl*>(data);
	bool muted = calldata_bool(calldata, "muted");

	QMetaObject::invokeMethod(volControl, "VolumeMuted",
			Q_ARG(bool, muted));
}

void VolControl::VolumeChanged()
{
	slider->blockSignals(true);
	slider->setValue((int) (obs_fader_get_deflection(obs_fader) * 100.0f));
	slider->blockSignals(false);
	
	updateText();
}

void VolControl::VolumeLevel(float mag, float peak, float peakHold, bool muted)
{
	if (muted) {
		mag = 0.0f;
		peak = 0.0f;
		peakHold = 0.0f;
	}

	volMeter->setLevels(mag, peak, peakHold);
}

void VolControl::VolumeMuted(bool muted)
{
	if (mute->isChecked() != muted)
		mute->setChecked(muted);
}

void VolControl::SetMuted(bool checked)
{
	obs_source_set_muted(source, checked);
}

void VolControl::SliderChanged(int vol)
{
	obs_fader_set_deflection(obs_fader, float(vol) * 0.01f);
	updateText();
}

void VolControl::updateText()
{
	volLabel->setText(QString::number(obs_fader_get_db(obs_fader), 'f', 1)
			.append(" dB"));
}

QString VolControl::GetName() const
{
	return nameLabel->text();
}

void VolControl::SetName(const QString &newName)
{
	nameLabel->setText(newName);
}

void VolControl::EmitConfigClicked()
{
	emit ConfigClicked();
}

VolControl::VolControl(OBSSource source_, bool showConfig)
	: source        (source_),
	  levelTotal    (0.0f),
	  levelCount    (0.0f),
	  obs_fader     (obs_fader_create(OBS_FADER_CUBIC)),
	  obs_volmeter  (obs_volmeter_create(OBS_FADER_LOG))
{
	QHBoxLayout *volLayout  = new QHBoxLayout();
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QHBoxLayout *textLayout = new QHBoxLayout();
	QHBoxLayout *botLayout  = new QHBoxLayout();

	nameLabel = new QLabel();
	volLabel  = new QLabel();
	volMeter  = new VolumeMeter();
	mute      = new MuteCheckBox();
	slider    = new QSlider(Qt::Horizontal);

	QFont font = nameLabel->font();
	font.setPointSize(font.pointSize()-1);

	nameLabel->setText(obs_source_get_name(source));
	nameLabel->setFont(font);
	volLabel->setFont(font);
	slider->setMinimum(0);
	slider->setMaximum(100);

//	slider->setMaximumHeight(13);

	textLayout->setContentsMargins(0, 0, 0, 0);
	textLayout->addWidget(nameLabel);
	textLayout->addWidget(volLabel);
	textLayout->setAlignment(nameLabel, Qt::AlignLeft);
	textLayout->setAlignment(volLabel,  Qt::AlignRight);

	mute->setChecked(obs_source_muted(source));

	volLayout->addWidget(slider);
	volLayout->addWidget(mute);
	volLayout->setSpacing(5);

	botLayout->setContentsMargins(0, 0, 0, 0);
	botLayout->setSpacing(0);
	botLayout->addLayout(volLayout);

	if (showConfig) {
		config = new QPushButton(this);
		config->setProperty("themeID", "configIconSmall");
		config->setFlat(true);
		config->setSizePolicy(QSizePolicy::Maximum,
				QSizePolicy::Maximum);
		config->setMaximumSize(22, 22);
		config->setAutoDefault(false);

		connect(config, &QAbstractButton::clicked,
				this, &VolControl::EmitConfigClicked);

		botLayout->addWidget(config);
	}

	mainLayout->setContentsMargins(4, 4, 4, 4);
	mainLayout->setSpacing(2);
	mainLayout->addItem(textLayout);
	mainLayout->addWidget(volMeter);
	mainLayout->addItem(botLayout);

	setLayout(mainLayout);

	signal_handler_connect(obs_fader_get_signal_handler(obs_fader),
			"volume_changed", OBSVolumeChanged, this);

	signal_handler_connect(obs_volmeter_get_signal_handler(obs_volmeter),
			"levels_updated", OBSVolumeLevel, this);

	signal_handler_connect(obs_source_get_signal_handler(source),
			"mute", OBSVolumeMuted, this);

	QWidget::connect(slider, SIGNAL(valueChanged(int)),
			this, SLOT(SliderChanged(int)));
	QWidget::connect(mute, SIGNAL(clicked(bool)),
			this, SLOT(SetMuted(bool)));

	obs_fader_attach_source(obs_fader, source);
	obs_volmeter_attach_source(obs_volmeter, source);

	slider->setStyle(new SliderAbsoluteSetStyle(slider->style()));

	/* Call volume changed once to init the slider position and label */
	VolumeChanged();
}

VolControl::~VolControl()
{
	signal_handler_disconnect(obs_fader_get_signal_handler(obs_fader),
			"volume_changed", OBSVolumeChanged, this);

	signal_handler_disconnect(obs_volmeter_get_signal_handler(obs_volmeter),
			"levels_updated", OBSVolumeLevel, this);

	signal_handler_disconnect(obs_source_get_signal_handler(source),
			"mute", OBSVolumeMuted, this);

	obs_fader_destroy(obs_fader);
	obs_volmeter_destroy(obs_volmeter);
}

QColor VolumeMeter::getBkColor() const
{
	return bkColor;
}

void VolumeMeter::setBkColor(QColor c)
{
	bkColor = c;
}

QColor VolumeMeter::getMagColor() const
{
	return magColor;
}

void VolumeMeter::setMagColor(QColor c)
{
	magColor = c;
}

QColor VolumeMeter::getPeakColor() const
{
	return peakColor;
}

void VolumeMeter::setPeakColor(QColor c)
{
	peakColor = c;
}

QColor VolumeMeter::getPeakHoldColor() const
{
	return peakHoldColor;
}

void VolumeMeter::setPeakHoldColor(QColor c)
{
	peakHoldColor = c;
}


VolumeMeter::VolumeMeter(QWidget *parent)
			: QWidget(parent)
{
	setMinimumSize(1, 3);

	//Default meter color settings, they only show if there is no stylesheet, do not remove.
	bkColor.setRgb(0xDD, 0xDD, 0xDD);
	magColor.setRgb(0x20, 0x7D, 0x17);
	peakColor.setRgb(0x3E, 0xF1, 0x2B);
	peakHoldColor.setRgb(0x00, 0x00, 0x00);
	
	resetTimer = new QTimer(this);
	connect(resetTimer, SIGNAL(timeout()), this, SLOT(resetState()));

	resetState();
}

void VolumeMeter::resetState(void)
{
	setLevels(0.0f, 0.0f, 0.0f);
	if (resetTimer->isActive())
		resetTimer->stop();
}

void VolumeMeter::setLevels(float nmag, float npeak, float npeakHold)
{
	mag      = nmag;
	peak     = npeak;
	peakHold = npeakHold;

	update();

	if (resetTimer->isActive())
		resetTimer->stop();
	resetTimer->start(1000);
}

void VolumeMeter::paintEvent(QPaintEvent *event)
{
	UNUSED_PARAMETER(event);

	QPainter painter(this);
	QLinearGradient gradient;

	int width  = size().width();
	int height = size().height();

	int scaledMag      = int((float)width * mag);
	int scaledPeak     = int((float)width * peak);
	int scaledPeakHold = int((float)width * peakHold);

	gradient.setStart(qreal(scaledMag), 0);
	gradient.setFinalStop(qreal(scaledPeak), 0);
	gradient.setColorAt(0, magColor);
	gradient.setColorAt(1, peakColor);

	// RMS
	painter.fillRect(0, 0, 
			scaledMag, height,
			magColor);

	// RMS - Peak gradient
	painter.fillRect(scaledMag, 0,
			scaledPeak - scaledMag + 1, height,
			QBrush(gradient));

	// Background
	painter.fillRect(scaledPeak, 0,
			width - scaledPeak, height,
			bkColor);

	// Peak hold
	if (peakHold == 1.0f)
		scaledPeakHold--;

	painter.setPen(peakHoldColor);
	painter.drawLine(scaledPeakHold, 0,
		scaledPeakHold, height);

}
