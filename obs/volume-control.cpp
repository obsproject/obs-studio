#include "volume-control.hpp"
#include "qt-wrappers.hpp"
#include <util/platform.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <string>
#include <math.h>

using namespace std;

#define VOL_MIN -96.0f
#define VOL_MAX  0.0f

/* 
	VOL_MIN_LOG = DBToLog(VOL_MIN)
	VOL_MAX_LOG = DBToLog(VOL_MAX)
	... just in case someone wants to use a smaller scale
 */

#define VOL_MIN_LOG -2.0086001717619175
#define VOL_MAX_LOG -0.77815125038364363

#define UPDATE_INTERVAL_MS 50

static inline float DBToLog(float db)
{
	return -log10f(0.0f - (db - 6.0f));
}

static inline float DBToLinear(float db_full)
{
	float db = fmaxf(fminf(db_full, VOL_MAX), VOL_MIN);
	return (DBToLog(db) - VOL_MIN_LOG) / (VOL_MAX_LOG - VOL_MIN_LOG);
}

void VolControl::OBSVolumeChanged(void *data, calldata_t calldata)
{
	VolControl *volControl = static_cast<VolControl*>(data);
	int vol = (int)(calldata_float(calldata, "volume") * 100.0f + 0.5f);

	QMetaObject::invokeMethod(volControl, "VolumeChanged", Q_ARG(int, vol));
}

void VolControl::OBSVolumeLevel(void *data, calldata_t calldata)
{
	VolControl *volControl = static_cast<VolControl*>(data);
	float peak      = calldata_float(calldata, "level");
	float mag       = calldata_float(calldata, "magnitude");
	float peakHold  = calldata_float(calldata, "peak");

	QMetaObject::invokeMethod(volControl, "VolumeLevel",
		Q_ARG(float, mag),
		Q_ARG(float, peak),
		Q_ARG(float, peakHold));
}

void VolControl::VolumeChanged(int vol)
{
	signalChanged = false;
	slider->setValue(vol);
	signalChanged = true;
}

void VolControl::VolumeLevel(float mag, float peak, float peakHold)
{
	uint64_t curMeterTime = os_gettime_ns() / 1000000;

	/*
	   Add again peak averaging?
	*/

	/* only update after a certain amount of time */
	if ((curMeterTime - lastMeterTime) > UPDATE_INTERVAL_MS) {
		float vol = (float)slider->value() * 0.01f;
		lastMeterTime = curMeterTime;
		volMeter->setLevels(DBToLinear(mag) * vol,
				    DBToLinear(peak) * vol,
				    DBToLinear(peakHold) * vol);
	}
}

void VolControl::SliderChanged(int vol)
{
	if (signalChanged) {
		signal_handler_disconnect(obs_source_get_signal_handler(source),
				"volume", OBSVolumeChanged, this);

		obs_source_set_volume(source, float(vol)*0.01f);

		signal_handler_connect(obs_source_get_signal_handler(source),
				"volume", OBSVolumeChanged, this);
	}

	volLabel->setText(QString::number(vol));
}

QString VolControl::GetName() const
{
	return nameLabel->text();
}

void VolControl::SetName(const QString &newName)
{
	nameLabel->setText(newName);
}

VolControl::VolControl(OBSSource source_)
	: source        (source_),
	  signalChanged (true),
	  lastMeterTime (0),
	  levelTotal    (0.0f),
	  levelCount    (0.0f)
{
	QVBoxLayout *mainLayout = new QVBoxLayout();
	QHBoxLayout *textLayout = new QHBoxLayout();
	int         vol         = int(obs_source_get_volume(source) * 100.0f);

	nameLabel = new QLabel();
	volLabel  = new QLabel();
	volMeter  = new VolumeMeter();
	slider    = new QSlider(Qt::Horizontal);

	QFont font = nameLabel->font();
	font.setPointSize(font.pointSize()-1);

	nameLabel->setText(obs_source_get_name(source));
	nameLabel->setFont(font);
	volLabel->setText(QString::number(vol));
	volLabel->setFont(font);
	slider->setMinimum(0);
	slider->setMaximum(100);
	slider->setValue(vol);
//	slider->setMaximumHeight(13);

	textLayout->setContentsMargins(0, 0, 0, 0);
	textLayout->addWidget(nameLabel);
	textLayout->addWidget(volLabel);
	textLayout->setAlignment(nameLabel, Qt::AlignLeft);
	textLayout->setAlignment(volLabel,  Qt::AlignRight);

	mainLayout->setContentsMargins(4, 4, 4, 4);
	mainLayout->setSpacing(2);
	mainLayout->addItem(textLayout);
	mainLayout->addWidget(volMeter);
	mainLayout->addWidget(slider);

	setLayout(mainLayout);

	signal_handler_connect(obs_source_get_signal_handler(source),
			"volume", OBSVolumeChanged, this);

	signal_handler_connect(obs_source_get_signal_handler(source),
		"volume_level", OBSVolumeLevel, this);

	QWidget::connect(slider, SIGNAL(valueChanged(int)),
			this, SLOT(SliderChanged(int)));
}

VolControl::~VolControl()
{
	signal_handler_disconnect(obs_source_get_signal_handler(source),
			"volume", OBSVolumeChanged, this);

	signal_handler_disconnect(obs_source_get_signal_handler(source),
		"volume_level", OBSVolumeLevel, this);
}

VolumeMeter::VolumeMeter(QWidget *parent)
			: QWidget(parent)
{
	setMinimumSize(1, 3);

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
	resetTimer->start(250);

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
