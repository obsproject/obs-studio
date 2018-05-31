#include <QToolTip>
#include "window-basic-media-controls.hpp"
#include "window-basic-main.hpp"

OBSBasicMediaControls::OBSBasicMediaControls(QWidget *parent,
		OBSSource source_)
	: QWidget (parent),
	  source  (source_)
{
	main = reinterpret_cast<OBSBasic*>(App()->GetMainWindow());
	isPlaying = false;

	QString sourceName = QString::fromUtf8(obs_source_get_name(source));
	setWindowTitle(QTStr("MediaControls") + " - " + sourceName);

	QVBoxLayout *vLayout = new QVBoxLayout();
	QHBoxLayout *layout = new QHBoxLayout();
	QHBoxLayout *topLayout = new QHBoxLayout();

	timerLabel = new QLabel();
	durationLabel = new QLabel();

	playPauseButton = new QPushButton();
	QPushButton *stopButton = new QPushButton();
	stopButton->setProperty("themeID", "stopIcon");
	QPushButton *nextButton = new QPushButton();
	nextButton->setProperty("themeID", "nextIcon");
	QPushButton *previousButton = new QPushButton();
	previousButton->setProperty("themeID", "previousIcon");

	playPauseButton->setMaximumSize(40, 40);
	stopButton->setMaximumSize(40, 40);
	nextButton->setMaximumSize(40, 40);
	previousButton->setMaximumSize(40, 40);

	slider = new MediaSlider();
	slider->setOrientation(Qt::Horizontal);
	slider->setMouseTracking(true);
	slider->setMinimum(0);
	slider->setMaximum(100);

	timer = new QTimer();

	topLayout->addWidget(timerLabel);
	topLayout->addWidget(slider);
	topLayout->addWidget(durationLabel);

	layout->addStretch();
	layout->addWidget(previousButton);
	layout->addWidget(playPauseButton);
	layout->addWidget(stopButton);
	layout->addWidget(nextButton);
	layout->addStretch();

	vLayout->addLayout(topLayout);
	vLayout->addLayout(layout);
	vLayout->addStretch();

	setLayout(vLayout);

	connect(playPauseButton, SIGNAL(clicked()), this,
			SLOT(MediaPlayPause()));
	connect(stopButton, SIGNAL(clicked()), this,
			SLOT(MediaStop()));
	connect(nextButton, SIGNAL(clicked()), this,
			SLOT(MediaNext()));
	connect(previousButton, SIGNAL(clicked()), this,
			SLOT(MediaPrevious()));
	connect(slider, SIGNAL(sliderMoved(int)), this,
			SLOT(MediaSliderChanged(int)));
	connect(slider, SIGNAL(sliderReleased()), this,
			SLOT(MediaSliderReleased()));
	connect(slider, SIGNAL(mediaSliderClicked()), this,
			SLOT(MediaSliderClicked()));
	connect(slider, SIGNAL(mediaSliderHovered()), this,
			SLOT(MediaSliderHovered()));
	connect(timer, SIGNAL(timeout()), this,
			SLOT(Update()));

	SetSliderPosition();
	SetPlayPauseIcon();
	SetDurationLabel();
	SetTimerLabel();

	channelChangedSignal.Connect(obs_get_signal_handler(), "channel_change",
			OBSChannelChanged, this);

	SetScene(main->GetCurrentScene());

	resize(450, 50);
	StartTimer();
}

OBSBasicMediaControls::~OBSBasicMediaControls()
{
	selectSignal.Disconnect();
	removeSignal.Disconnect();

	obs_source_dec_showing(source);
	deleteLater();
}

uint32_t OBSBasicMediaControls::GetMediaTime()
{
	return obs_source_media_get_time(source) / 1000;
}

uint32_t OBSBasicMediaControls::GetMediaDuration()
{
	return obs_source_media_get_duration(source) / 1000;
}

void OBSBasicMediaControls::SetSliderPosition()
{
	float sliderPosition = ((float)GetMediaTime() /
			(float)GetMediaDuration()) * 100.0f;

	slider->setValue((int)sliderPosition);
	SetTimerLabel();
}

void OBSBasicMediaControls::SetTimerLabel()
{
	timerLabel->setText(FormatSeconds(GetMediaTime()));
}

void OBSBasicMediaControls::SetDurationLabel()
{
	durationLabel->setText(FormatSeconds(GetMediaDuration()));
}

void OBSBasicMediaControls::MediaSeekTo(int value)
{
	float seconds = ((float)value / 100.0f) *
			obs_source_media_get_duration(source);

	obs_source_media_set_time(source, (uint32_t)seconds);
	SetTimerLabel();
}

void OBSBasicMediaControls::StartTimer()
{
	timer->start(100);
}

void OBSBasicMediaControls::StopTimer()
{
	timer->stop();
}

void OBSBasicMediaControls::SetPlayPauseIcon()
{
	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_PLAYING:
		playPauseButton->setProperty("themeID", "pauseIcon");
		slider->setEnabled(true);
		break;
	case OBS_MEDIA_STATE_STOPPED:
		ResetControls();
		break;
	case OBS_MEDIA_STATE_ENDED:
		playPauseButton->setProperty("themeID", "restartIcon");
		slider->setEnabled(false);
		break;
	default:
		playPauseButton->setProperty("themeID", "playIcon");
		slider->setEnabled(true);
		break;
	}

	playPauseButton->style()->unpolish(playPauseButton);
	playPauseButton->style()->polish(playPauseButton);
}

void OBSBasicMediaControls::Update()
{
	SetTimerLabel();
	SetDurationLabel();
	SetSliderPosition();
	SetPlayPauseIcon();
}

void OBSBasicMediaControls::MediaSliderChanged(int value)
{
	if (obs_source_media_get_state(source) == OBS_MEDIA_STATE_PLAYING) {
		isPlaying = true;
		MediaPlayPause();
	}

	StopTimer();

	MediaSeekTo(value);
}

void OBSBasicMediaControls::MediaSliderReleased()
{
	if (isPlaying) {
		obs_source_media_play_pause(source);
		isPlaying = false;
	}

	StartTimer();
}

void OBSBasicMediaControls::MediaSliderClicked()
{
	int value = slider->minimum() + ((slider->maximum()-slider->minimum())
			* slider->mousePos.x()) / slider->width();

	MediaSliderChanged(value);
	MediaSliderReleased();
}

void OBSBasicMediaControls::MediaSliderHovered()
{
	int value = slider->minimum() + ((slider->maximum()-slider->minimum())
			* slider->mousePos.x()) / slider->width();
	float seconds = ((float)value / 100.0f) * GetMediaDuration();

	QToolTip::showText(QCursor::pos(), FormatSeconds((int)seconds), slider);
}

QString OBSBasicMediaControls::FormatSeconds(int totalSeconds)
{
	int seconds      = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes      = totalMinutes % 60;
	int hours        = totalMinutes / 60;

	QString text;
	text.sprintf("%02d:%02d:%02d", hours, minutes, seconds);

	return text;
}

void OBSBasicMediaControls::MediaPlayPause()
{
	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_STOPPED || state == OBS_MEDIA_STATE_ENDED)
		MediaRestart();
	else
		obs_source_media_play_pause(source);

	SetPlayPauseIcon();
}

void OBSBasicMediaControls::MediaRestart()
{
	obs_source_media_restart(source);
}

void OBSBasicMediaControls::MediaStop()
{
	obs_source_media_stop(source);
	SetPlayPauseIcon();
}

void OBSBasicMediaControls::MediaNext()
{
	obs_source_media_next(source);
}

void OBSBasicMediaControls::MediaPrevious()
{
	obs_source_media_previous(source);
}

void OBSBasicMediaControls::ResetControls()
{
	playPauseButton->setProperty("themeID", "restartIcon");;
	slider->setValue(0);
	slider->setEnabled(false);
	timerLabel->setText(FormatSeconds(0));
	durationLabel->setText(FormatSeconds(0));
}

void OBSBasicMediaControls::SetScene(OBSScene scene)
{
	if (!scene)
		return;

	selectSignal.Disconnect();
	removeSignal.Disconnect();

	OBSSource sceneSource = obs_scene_get_source(scene);

	signal_handler_t *signal = obs_source_get_signal_handler(sceneSource);
	selectSignal.Connect(signal, "item_select", OBSSceneItemSelect, this);
	removeSignal.Connect(signal, "item_remove", OBSSceneItemRemoved, this);
}

void OBSBasicMediaControls::SetSource(OBSSource newSource)
{
	if (!newSource)
		return;

	source = newSource;

	QString sourceName = QString::fromUtf8(obs_source_get_name(source));
	setWindowTitle(QTStr("MediaControls") + " - " + sourceName);

	setEnabled(true);
}

void OBSBasicMediaControls::OBSChannelChanged(void *param, calldata_t *data)
{
	OBSBasicMediaControls *window =
			reinterpret_cast<OBSBasicMediaControls*>(param);
	uint32_t channel = (uint32_t)calldata_int(data, "channel");
	OBSSource source = (obs_source_t*)calldata_ptr(data, "source");

	if (channel == 0) {
		OBSScene scene = obs_scene_from_source(source);
		window->SetScene(scene);
	}
}

void OBSBasicMediaControls::OBSSceneItemSelect(void *param, calldata_t *data)
{
	OBSBasicMediaControls *window =
			reinterpret_cast<OBSBasicMediaControls*>(param);
	OBSSceneItem item = (obs_sceneitem_t*)calldata_ptr(data, "item");
	OBSSource source = obs_sceneitem_get_source(item);

	const char *id = obs_source_get_id(source);

	if (strcmp(id, "vlc_source") != 0 || source == window->source)
		return;

	window->SetSource(source);
}

void OBSBasicMediaControls::OBSSceneItemRemoved(void *param, calldata_t *data)
{
	OBSBasicMediaControls *window =
			reinterpret_cast<OBSBasicMediaControls*>(param);
	OBSSceneItem item = (obs_sceneitem_t*)calldata_ptr(data, "item");
	OBSSource source = obs_sceneitem_get_source(item);

	if (source == window->source) {
		window->setEnabled(false);
		window->MediaStop();
		window->ResetControls();
	}
}
