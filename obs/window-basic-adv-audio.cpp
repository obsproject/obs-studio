#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include "window-basic-adv-audio.hpp"
#include "adv-audio-control.hpp"
#include "obs-app.hpp"
#include "qt-wrappers.hpp"

Q_DECLARE_METATYPE(OBSSource);

OBSBasicAdvAudio::OBSBasicAdvAudio(QWidget *parent)
	: QDialog(parent),
	  sourceAddedSignal(obs_get_signal_handler(), "source_activate",
			  OBSSourceAdded, this),
	  sourceRemovedSignal(obs_get_signal_handler(), "source_deactivate",
			  OBSSourceRemoved, this)
{
	QScrollArea *scrollArea;
	QVBoxLayout *vlayout;
	QHBoxLayout *hlayout;
	QWidget *widget;
	QLabel *label;

	hlayout = new QHBoxLayout;
	hlayout->setContentsMargins(0, 0, 0, 0);
	label = new QLabel(QTStr("Basic.AdvAudio.Name"));
	label->setMinimumWidth(170);
	label->setAlignment(Qt::AlignHCenter);
	hlayout->addWidget(label);
	label = new QLabel(QTStr("Basic.AdvAudio.Volume"));
	label->setMinimumWidth(130);
	label->setMaximumWidth(130);
	label->setAlignment(Qt::AlignHCenter);
	hlayout->addWidget(label);
	label = new QLabel(QTStr("Basic.AdvAudio.Mono"));
	label->setMinimumWidth(130);
	label->setMaximumWidth(130);
	label->setAlignment(Qt::AlignHCenter);
	hlayout->addWidget(label);
	label = new QLabel(QTStr("Basic.AdvAudio.Panning"));
	label->setMinimumWidth(140);
	label->setMaximumWidth(140);
	label->setAlignment(Qt::AlignHCenter);
	hlayout->addWidget(label);
	label = new QLabel(QTStr("Basic.AdvAudio.SyncOffset"));
	label->setMinimumWidth(130);
	label->setMaximumWidth(130);
	label->setAlignment(Qt::AlignHCenter);
	hlayout->addWidget(label);
	label = new QLabel(QTStr("Basic.AdvAudio.MediaChannel"));
	label->setMinimumWidth(160);
	label->setMaximumWidth(160);
	label->setAlignment(Qt::AlignHCenter);
	hlayout->addWidget(label);
	widget = new QWidget;
	widget->setLayout(hlayout);

	vlayout = new QVBoxLayout;
	vlayout->setContentsMargins(0, 0, 0, 10);
	vlayout->addWidget(widget);
	controlArea = new QWidget;
	controlArea->setLayout(vlayout);
	controlArea->setSizePolicy(QSizePolicy::Preferred,
			QSizePolicy::Preferred);

	vlayout = new QVBoxLayout;
	vlayout->addWidget(controlArea);
	//vlayout->setAlignment(controlArea, Qt::AlignTop);
	widget = new QWidget;
	widget->setLayout(vlayout);
	widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

	scrollArea = new QScrollArea;
	scrollArea->setWidget(widget);
	scrollArea->setWidgetResizable(true);

	vlayout = new QVBoxLayout;
	vlayout->setContentsMargins(11, 11, 11, 11);
	vlayout->addWidget(scrollArea);
	setLayout(vlayout);

	/* get global audio sources */
	for (uint32_t i = 1; i <= 10; i++) {
		obs_source_t *source = obs_get_output_source(i);
		if (source) {
			AddAudioSource(source);
			obs_source_release(source);
		}
	}

	/* enum user scene/sources */
	obs_enum_sources(EnumSources, this);

	resize(1000, 340);
	setWindowTitle(QTStr("Basic.AdvAudio"));
	setSizeGripEnabled(true);
	setWindowModality(Qt::NonModal);
	setAttribute(Qt::WA_DeleteOnClose, true);
}

bool OBSBasicAdvAudio::EnumSources(void *param, obs_source_t *source)
{
	OBSBasicAdvAudio *dialog = reinterpret_cast<OBSBasicAdvAudio*>(param);
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) != 0 && obs_source_active(source))
		dialog->AddAudioSource(source);

	return true;
}

void OBSBasicAdvAudio::OBSSourceAdded(void *param, calldata_t *calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	QMetaObject::invokeMethod(reinterpret_cast<OBSBasicAdvAudio*>(param),
			"SourceAdded", Q_ARG(OBSSource, source));
}

void OBSBasicAdvAudio::OBSSourceRemoved(void *param, calldata_t *calldata)
{
	OBSSource source((obs_source_t*)calldata_ptr(calldata, "source"));

	QMetaObject::invokeMethod(reinterpret_cast<OBSBasicAdvAudio*>(param),
			"SourceRemoved", Q_ARG(OBSSource, source));
}

inline void OBSBasicAdvAudio::AddAudioSource(obs_source_t *source)
{
	OBSAdvAudioCtrl *control = new OBSAdvAudioCtrl(source);
	controlArea->layout()->addWidget(control);
	controls.push_back(control);
}

void OBSBasicAdvAudio::SourceAdded(OBSSource source)
{
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) == 0)
		return;

	AddAudioSource(source);
}

void OBSBasicAdvAudio::SourceRemoved(OBSSource source)
{
	uint32_t flags = obs_source_get_output_flags(source);

	if ((flags & OBS_SOURCE_AUDIO) == 0)
		return;

	for (size_t i = 0; i < controls.size(); i++) {
		if (controls[i]->GetSource() == source) {
			delete controls[i];
			controls.erase(controls.begin() + i);
			break;
		}
	}
}
