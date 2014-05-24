#include "volume-control.hpp"
#include "qt-wrappers.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QLabel>
#include <string>

using namespace std;

void VolControl::OBSVolumeChanged(void *data, calldata_t calldata)
{
	VolControl *volControl = static_cast<VolControl*>(data);
	int vol = (int)(calldata_float(calldata, "volume") * 100.0f + 0.5f);

	QMetaObject::invokeMethod(volControl, "VolumeChanged",
			Q_ARG(int, vol));
}


/* [Danni] This may be a bit too resource intensive for such a simple 
		   application. */

void VolControl::OBSVolumeLevel(void *data, calldata_t calldata)
{
	VolControl *volControl = static_cast<VolControl*>(data);
	int v = calldata_int(calldata, "volumelevel");
		
	QMetaObject::invokeMethod(volControl, "VolumeLevel",
		Q_ARG(int, v));
}

void VolControl::VolumeChanged(int vol)
{
	signalChanged = false;
	slider->setValue(vol);
	signalChanged = true;
}

void VolControl::VolumeLevel(int vol)
{
	volMeter->setValue(vol); /* linear */
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
	  signalChanged (true)
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
	slider->setMaximumHeight(10);

	volMeter->setMaximumHeight(1);
	volMeter->setMinimum(0);
	volMeter->setMaximum(10000);
	volMeter->setTextVisible(false);

	// [Danni] Temporary color.
	QString testColor = "QProgressBar {border: 0px} QProgressBar::chunk {width: 1px; background-color: #AA0000;}";
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
		"volumelevel", OBSVolumeLevel, this);

	QWidget::connect(slider, SIGNAL(valueChanged(int)),
			this, SLOT(SliderChanged(int)));
}

VolControl::~VolControl()
{
	signal_handler_disconnect(obs_source_signalhandler(source),
			"volume", OBSVolumeChanged, this);

	signal_handler_disconnect(obs_source_signalhandler(source),
		"volumelevel", OBSVolumeLevel, this);
}
