#include "media-controls.hpp"
#include <QToolTip>

void MediaControls::OBSFrontendEvent(enum obs_frontend_event event, void *ptr)
{
	MediaControls *controls = reinterpret_cast<MediaControls *>(ptr);
	OBSBasic *main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	switch (event) {
	case OBS_FRONTEND_EVENT_SCENE_CHANGED:
		controls->SetScene(main->GetCurrentScene());
		break;
	default:
		break;
	}
}

void MediaControls::OBSMediaStopped(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetRestartState");
}

void MediaControls::OBSMediaPlay(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void MediaControls::OBSMediaPause(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPausedState");
}

void MediaControls::OBSMediaStarted(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void MediaControls::OBSSceneItemRemoved(void *param, calldata_t *data)
{
	MediaControls *controls = reinterpret_cast<MediaControls *>(param);
	OBSSceneItem item = (obs_sceneitem_t *)calldata_ptr(data, "item");
	OBSSource source = obs_sceneitem_get_source(item);

	if (source == controls->GetSource())
		controls->SetSource(nullptr);
}

void MediaControls::OBSSceneItemSelect(void *param, calldata_t *data)
{
	MediaControls *controls = reinterpret_cast<MediaControls *>(param);
	OBSSceneItem item = (obs_sceneitem_t *)calldata_ptr(data, "item");
	OBSSource source = obs_sceneitem_get_source(item);

	uint32_t flags = obs_source_get_output_flags(source);

	if (flags & OBS_SOURCE_CONTROLLABLE_MEDIA)
		controls->SetSource(source);
	else
		controls->SetSource(nullptr);
}

void MediaControls::SetScene(OBSScene scene)
{
	selectSignal.Disconnect();
	removeSignal.Disconnect();

	if (scene) {
		OBSSource source = obs_scene_get_source(scene);
		signal_handler_t *signal =
			obs_source_get_signal_handler(source);

		removeSignal.Connect(signal, "item_remove", OBSSceneItemRemoved,
				     this);
		selectSignal.Connect(signal, "item_select", OBSSceneItemSelect,
				     this);
	}
}

MediaControls::MediaControls(QWidget *parent)
	: QWidget(parent), ui(new Ui::MediaControls)
{
	ui->setupUi(this);
	ui->playPauseButton->setProperty("themeID", "playIcon");
	ui->previousButton->setProperty("themeID", "previousIcon");
	ui->nextButton->setProperty("themeID", "nextIcon");
	ui->stopButton->setProperty("themeID", "stopIcon");

	main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(SetSliderPosition()));
	connect(ui->slider, SIGNAL(mediaSliderClicked()), this,
		SLOT(SliderClicked()));
	connect(ui->slider, SIGNAL(mediaSliderHovered(int)), this,
		SLOT(SliderHovered(int)));
	connect(ui->slider, SIGNAL(mediaSliderReleased(int)), this,
		SLOT(SliderReleased(int)));

	obs_frontend_add_event_callback(OBSFrontendEvent, this);

	SetScene(main->GetCurrentScene());
	hide();
}

MediaControls::~MediaControls()
{
	deleteLater();
}

void MediaControls::SliderClicked()
{
	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_PAUSED:
		prevPaused = true;
		break;
	case OBS_MEDIA_STATE_PLAYING:
		prevPaused = false;
		obs_source_media_play_pause(source, true);
	default:
		break;
	}
}

void MediaControls::SliderReleased(int val)
{
	ui->slider->setValue(val);

	float percent = (float)val / float(ui->slider->maximum());
	int64_t seekTo =
		(int64_t)(percent * (obs_source_media_get_duration(source)));
	obs_source_media_set_time(source, seekTo);

	ui->timerLabel->setText(FormatSeconds((int)((float)seekTo / 1000.0f)));

	if (!prevPaused)
		obs_source_media_play_pause(source, false);
}

void MediaControls::SliderHovered(int val)
{
	float percent = (float)val / float(ui->slider->maximum());

	float seconds =
		percent * (obs_source_media_get_duration(source) / 1000.0f);

	QToolTip::showText(QCursor::pos(), FormatSeconds((int)seconds), this);
}

void MediaControls::StartTimer()
{
	if (!timer->isActive())
		timer->start(100);
}

void MediaControls::StopTimer()
{
	if (timer->isActive())
		timer->stop();
}

void MediaControls::SetPlayingState()
{
	ui->slider->setEnabled(true);
	ui->playPauseButton->setProperty("themeID", "pauseIcon");
	ui->playPauseButton->style()->unpolish(ui->playPauseButton);
	ui->playPauseButton->style()->polish(ui->playPauseButton);

	prevPaused = false;

	StartTimer();
}

void MediaControls::SetPausedState()
{
	ui->playPauseButton->setProperty("themeID", "playIcon");
	ui->playPauseButton->style()->unpolish(ui->playPauseButton);
	ui->playPauseButton->style()->polish(ui->playPauseButton);

	StopTimer();
}

void MediaControls::SetRestartState()
{
	ui->playPauseButton->setProperty("themeID", "restartIcon");
	ui->playPauseButton->style()->unpolish(ui->playPauseButton);
	ui->playPauseButton->style()->polish(ui->playPauseButton);

	ui->slider->setValue(0);
	ui->timerLabel->setText("00:00:00");
	ui->durationLabel->setText("00:00:00");
	ui->slider->setEnabled(false);

	StopTimer();
}

void MediaControls::RefreshControls()
{
	if (!source) {
		SetRestartState();
		setEnabled(false);
		return;
	} else {
		setEnabled(true);
	}

	const char *id = obs_source_get_id(source);

	if (id && *id && strcmp(id, "ffmpeg_source") == 0) {
		ui->nextButton->hide();
		ui->previousButton->hide();
	} else {
		ui->nextButton->show();
		ui->previousButton->show();
	}

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		SetRestartState();
		break;
	case OBS_MEDIA_STATE_PLAYING:
		SetPlayingState();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		SetPausedState();
		break;
	default:
		break;
	}
}

OBSSource MediaControls::GetSource()
{
	return source;
}

void MediaControls::SetSource(OBSSource newSource)
{
	signal_handler_disconnect(obs_source_get_signal_handler(source),
				  "media_play", OBSMediaPlay, this);
	signal_handler_disconnect(obs_source_get_signal_handler(source),
				  "media_pause", OBSMediaPause, this);
	signal_handler_disconnect(obs_source_get_signal_handler(source),
				  "media_restart", OBSMediaPlay, this);
	signal_handler_disconnect(obs_source_get_signal_handler(source),
				  "media_stopped", OBSMediaStopped, this);
	signal_handler_disconnect(obs_source_get_signal_handler(source),
				  "media_started", OBSMediaStarted, this);
	signal_handler_connect(obs_source_get_signal_handler(source),
			       "media_ended", OBSMediaStopped, this);

	source = newSource;

	if (source) {
		signal_handler_connect(obs_source_get_signal_handler(source),
				       "media_play", OBSMediaPlay, this);
		signal_handler_connect(obs_source_get_signal_handler(source),
				       "media_pause", OBSMediaPause, this);
		signal_handler_connect(obs_source_get_signal_handler(source),
				       "media_restart", OBSMediaPlay, this);
		signal_handler_connect(obs_source_get_signal_handler(source),
				       "media_stopped", OBSMediaStopped, this);
		signal_handler_connect(obs_source_get_signal_handler(source),
				       "media_started", OBSMediaStarted, this);
		signal_handler_connect(obs_source_get_signal_handler(source),
				       "media_ended", OBSMediaStopped, this);
	}

	RefreshControls();
}

void MediaControls::SetSliderPosition()
{
	float time = (float)obs_source_media_get_time(source);
	float duration = (float)obs_source_media_get_duration(source);

	float sliderPosition = (time / duration) * (float)ui->slider->maximum();

	ui->slider->setValue((int)sliderPosition);

	ui->timerLabel->setText(FormatSeconds((int)(time / 1000.0f)));
	ui->durationLabel->setText(FormatSeconds((int)(duration / 1000.0f)));
}

QString MediaControls::FormatSeconds(int totalSeconds)
{
	int seconds = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	QString text;
	text.sprintf("%02d:%02d:%02d", hours, minutes, seconds);

	return text;
}

void MediaControls::on_playPauseButton_clicked()
{
	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		obs_source_media_restart(source);
		break;
	case OBS_MEDIA_STATE_PLAYING:
		obs_source_media_play_pause(source, true);
		break;
	case OBS_MEDIA_STATE_PAUSED:
		obs_source_media_play_pause(source, false);
		break;
	default:
		break;
	}
}

void MediaControls::on_stopButton_clicked()
{
	obs_source_media_stop(source);
}

void MediaControls::on_nextButton_clicked()
{
	obs_source_media_next(source);
}

void MediaControls::on_previousButton_clicked()
{
	obs_source_media_previous(source);
}
