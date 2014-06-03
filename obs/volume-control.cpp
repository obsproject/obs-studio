#include "volume-control.hpp"
#include "qt-wrappers.hpp"
#include <util/platform.h>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>
#include <string>
#include <math.h>

using namespace std;

#define VOL_MIN -96.0f
#define VOL_MAX  0.0f

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
	float level = calldata_float(calldata, "level");
	float mag   = calldata_float(calldata, "magnitude");

	/*
	 * TODO: an actual volume control that can process level, mag, peak.
	 *
	 * for the time being, just average level and magnitude.
	 */
	float result = (level + mag) * 0.5f;

	QMetaObject::invokeMethod(volControl, "VolumeLevel",
		Q_ARG(float, result));
}

void VolControl::VolumeChanged(int vol)
{
	signalChanged = false;
	slider->setValue(vol);
	signalChanged = true;
}

void VolControl::VolumeLevel(float level)
{
	uint64_t curMeterTime = os_gettime_ns() / 1000000;

	levelTotal += level;
	levelCount += 1.0f;

	/* only update after a certain amount of time */
	if ((curMeterTime - lastMeterTime) > UPDATE_INTERVAL_MS) {
		lastMeterTime = curMeterTime;

		float finalLevel = levelTotal / levelCount;
		volMeter->setValue(int(DBToLinear(finalLevel) * 10000.0f));

		levelTotal = 0.0f;
		levelCount = 0.0f;
	}
}

void VolControl::SliderChanged(int vol)
{
	if (signalChanged) {
		signal_handler_disconnect(obs_source_signalhandler(source),
				"volume", OBSVolumeChanged, this);

		obs_source_setvolume(source, float(vol)*0.01f);

		signal_handler_connect(obs_source_signalhandler(source),
				"volume", OBSVolumeChanged, this);
	}

	volLabel->setText(QString::number(vol));
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
	int         vol         = int(obs_source_getvolume(source) * 100.0f);

	nameLabel = new QLabel();
	volLabel  = new QLabel();
	volMeter  = new QProgressBar();
	slider    = new QSlider(Qt::Horizontal);

	QFont font = nameLabel->font();
	font.setPointSize(font.pointSize()-1);

	nameLabel->setText(obs_source_getname(source));
	nameLabel->setFont(font);
	volLabel->setText(QString::number(vol));
	volLabel->setFont(font);
	slider->setMinimum(0);
	slider->setMaximum(100);
	slider->setValue(vol);
//	slider->setMaximumHeight(13);

	volMeter->setMaximumHeight(1);
	volMeter->setMinimum(0);
	volMeter->setMaximum(10000);
	volMeter->setTextVisible(false);

	/* [Danni] Temporary color. */
	QString testColor = "QProgressBar "
	                    "{border: 0px} "
	                    "QProgressBar::chunk "
	                    "{width: 1px; background-color: #AA0000;}";
	volMeter->setStyleSheet(testColor);

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

	signal_handler_connect(obs_source_signalhandler(source),
			"volume", OBSVolumeChanged, this);

	signal_handler_connect(obs_source_signalhandler(source),
		"volume_level", OBSVolumeLevel, this);

	QWidget::connect(slider, SIGNAL(valueChanged(int)),
			this, SLOT(SliderChanged(int)));
}

VolControl::~VolControl()
{
	signal_handler_disconnect(obs_source_signalhandler(source),
			"volume", OBSVolumeChanged, this);

	signal_handler_disconnect(obs_source_signalhandler(source),
		"volume_level", OBSVolumeLevel, this);
}
