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

void VolControl::VolumeChanged(int vol)
{
	signalChanged = false;
	slider->setValue(vol);
	signalChanged = true;
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
	//slider->setMaximumHeight(16);

	textLayout->setContentsMargins(0, 0, 0, 0);
	textLayout->addWidget(nameLabel);
	textLayout->addWidget(volLabel);
	textLayout->setAlignment(nameLabel, Qt::AlignLeft);
	textLayout->setAlignment(volLabel,  Qt::AlignRight);

	mainLayout->setContentsMargins(4, 4, 4, 4);
	mainLayout->setSpacing(2);
	mainLayout->addItem(textLayout);
	mainLayout->addWidget(slider);

	setLayout(mainLayout);

	signal_handler_connect(obs_source_signalhandler(source),
			"volume", OBSVolumeChanged, this);

	QWidget::connect(slider, SIGNAL(valueChanged(int)),
			this, SLOT(SliderChanged(int)));
}

VolControl::~VolControl()
{
	signal_handler_disconnect(obs_source_signalhandler(source),
			"volume", OBSVolumeChanged, this);
}
