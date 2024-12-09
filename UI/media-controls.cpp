#include "window-basic-main.hpp"
#include "moc_media-controls.cpp"
#include "obs-app.hpp"
#include <QToolTip>
#include <QStyle>
#include <QMenu>

#include "ui_media-controls.h"

void MediaControls::OBSMediaStopped(void *data, calldata_t *)
{
	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetRestartState");
}

void MediaControls::OBSMediaPlay(void *data, calldata_t *)
{
	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void MediaControls::OBSMediaPause(void *data, calldata_t *)
{
	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPausedState");
}

void MediaControls::OBSMediaStarted(void *data, calldata_t *)
{
	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPlayingState");
}

void MediaControls::OBSMediaNext(void *data, calldata_t *)
{
	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "UpdateSlideCounter");
}

void MediaControls::OBSMediaPrevious(void *data, calldata_t *)
{
	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "UpdateSlideCounter");
}

MediaControls::MediaControls(QWidget *parent) : QWidget(parent), ui(new Ui::MediaControls)
{
	ui->setupUi(this);
	ui->playPauseButton->setProperty("class", "icon-media-play");
	ui->previousButton->setProperty("class", "icon-media-prev");
	ui->nextButton->setProperty("class", "icon-media-next");
	ui->stopButton->setProperty("class", "icon-media-stop");
	setFocusPolicy(Qt::StrongFocus);

	connect(&mediaTimer, &QTimer::timeout, this, &MediaControls::SetSliderPosition);
	connect(&seekTimer, &QTimer::timeout, this, &MediaControls::SeekTimerCallback);
	connect(ui->slider, &AbsoluteSlider::sliderPressed, this, &MediaControls::AbsoluteSliderClicked);
	connect(ui->slider, &AbsoluteSlider::absoluteSliderHovered, this, &MediaControls::AbsoluteSliderHovered);
	connect(ui->slider, &AbsoluteSlider::sliderReleased, this, &MediaControls::AbsoluteSliderReleased);
	connect(ui->slider, &AbsoluteSlider::sliderMoved, this, &MediaControls::AbsoluteSliderMoved);

	countDownTimer = config_get_bool(App()->GetUserConfig(), "BasicWindow", "MediaControlsCountdownTimer");

	QAction *restartAction = new QAction(this);
	restartAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	restartAction->setShortcut({Qt::Key_R});
	connect(restartAction, &QAction::triggered, this, &MediaControls::RestartMedia);
	addAction(restartAction);

	QAction *sliderFoward = new QAction(this);
	sliderFoward->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(sliderFoward, &QAction::triggered, this, &MediaControls::MoveSliderFoward);
	sliderFoward->setShortcut({Qt::Key_Right});
	addAction(sliderFoward);

	QAction *sliderBack = new QAction(this);
	sliderBack->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(sliderBack, &QAction::triggered, this, &MediaControls::MoveSliderBackwards);
	sliderBack->setShortcut({Qt::Key_Left});
	addAction(sliderBack);

	QAction *playPause = new QAction(this);
	playPause->setShortcutContext(Qt::WidgetWithChildrenShortcut);
	connect(playPause, &QAction::triggered, this, &MediaControls::on_playPauseButton_clicked);
	playPause->setShortcut({Qt::Key_Space});
	addAction(playPause);
}

MediaControls::~MediaControls() {}

bool MediaControls::MediaPaused()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return false;
	}

	obs_media_state state = obs_source_media_get_state(source);
	return state == OBS_MEDIA_STATE_PAUSED;
}

int64_t MediaControls::GetSliderTime(int val)
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return 0;
	}

	float percent = (float)val / (float)ui->slider->maximum();
	float duration = (float)obs_source_media_get_duration(source);
	int64_t seekTo = (int64_t)(percent * duration);

	return seekTo;
}

void MediaControls::AbsoluteSliderClicked()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_PAUSED) {
		prevPaused = true;
	} else if (state == OBS_MEDIA_STATE_PLAYING) {
		prevPaused = false;
		PauseMedia();
		StopMediaTimer();
	}

	seek = ui->slider->value();
	seekTimer.start(100);
}

void MediaControls::AbsoluteSliderReleased()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	if (seekTimer.isActive()) {
		seekTimer.stop();
		if (lastSeek != seek) {
			obs_source_media_set_time(source, GetSliderTime(seek));
		}

		UpdateLabels(seek);
		seek = lastSeek = -1;
	}

	if (!prevPaused) {
		PlayMedia();
		StartMediaTimer();
	}
}

void MediaControls::AbsoluteSliderHovered(int val)
{
	float seconds = ((float)GetSliderTime(val) / 1000.0f);
	QToolTip::showText(QCursor::pos(), FormatSeconds((int)seconds), this);
}

void MediaControls::AbsoluteSliderMoved(int val)
{
	if (seekTimer.isActive()) {
		seek = val;
		UpdateLabels(seek);
	}
}

void MediaControls::SeekTimerCallback()
{
	if (lastSeek != seek) {
		OBSSource source = OBSGetStrongRef(weakSource);
		if (source) {
			obs_source_media_set_time(source, GetSliderTime(seek));
		}
		lastSeek = seek;
	}
}

void MediaControls::StartMediaTimer()
{
	if (isSlideshow)
		return;

	if (!mediaTimer.isActive())
		mediaTimer.start(1000);
}

void MediaControls::StopMediaTimer()
{
	if (mediaTimer.isActive())
		mediaTimer.stop();
}

void MediaControls::SetPlayingState()
{
	ui->slider->setEnabled(true);
	ui->playPauseButton->setProperty("class", "icon-media-pause");
	ui->playPauseButton->style()->unpolish(ui->playPauseButton);
	ui->playPauseButton->style()->polish(ui->playPauseButton);
	ui->playPauseButton->setToolTip(QTStr("ContextBar.MediaControls.PauseMedia"));

	prevPaused = false;

	UpdateSlideCounter();
	StartMediaTimer();
}

void MediaControls::SetPausedState()
{
	ui->playPauseButton->setProperty("class", "icon-media-play");
	ui->playPauseButton->style()->unpolish(ui->playPauseButton);
	ui->playPauseButton->style()->polish(ui->playPauseButton);
	ui->playPauseButton->setToolTip(QTStr("ContextBar.MediaControls.PlayMedia"));

	StopMediaTimer();
}

void MediaControls::SetRestartState()
{
	ui->playPauseButton->setProperty("class", "icon-media-restart");
	ui->playPauseButton->style()->unpolish(ui->playPauseButton);
	ui->playPauseButton->style()->polish(ui->playPauseButton);
	ui->playPauseButton->setToolTip(QTStr("ContextBar.MediaControls.RestartMedia"));

	ui->slider->setValue(0);

	if (!isSlideshow) {
		ui->timerLabel->setText("--:--:--");
		ui->durationLabel->setText("--:--:--");
	} else {
		ui->timerLabel->setText("-");
		ui->durationLabel->setText("-");
	}

	ui->slider->setEnabled(false);

	StopMediaTimer();
}

void MediaControls::RefreshControls()
{
	OBSSource source;
	source = OBSGetStrongRef(weakSource);

	uint32_t flags = 0;
	const char *id = nullptr;

	if (source) {
		flags = obs_source_get_output_flags(source);
		id = obs_source_get_unversioned_id(source);
	}

	if (!source || !(flags & OBS_SOURCE_CONTROLLABLE_MEDIA)) {
		SetRestartState();
		setEnabled(false);
		hide();
		return;
	} else {
		setEnabled(true);
		show();
	}

	bool has_playlist = strcmp(id, "ffmpeg_source") != 0;
	ui->previousButton->setVisible(has_playlist);
	ui->nextButton->setVisible(has_playlist);

	isSlideshow = strcmp(id, "slideshow") == 0;
	ui->slider->setVisible(!isSlideshow);
	ui->emptySpaceAgain->setVisible(isSlideshow);

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
	case OBS_MEDIA_STATE_NONE:
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

	if (isSlideshow)
		UpdateSlideCounter();
	else
		SetSliderPosition();
}

OBSSource MediaControls::GetSource()
{
	return OBSGetStrongRef(weakSource);
}

void MediaControls::SetSource(OBSSource source)
{
	sigs.clear();

	if (source) {
		weakSource = OBSGetWeakRef(source);
		signal_handler_t *sh = obs_source_get_signal_handler(source);
		sigs.emplace_back(sh, "media_play", OBSMediaPlay, this);
		sigs.emplace_back(sh, "media_pause", OBSMediaPause, this);
		sigs.emplace_back(sh, "media_restart", OBSMediaPlay, this);
		sigs.emplace_back(sh, "media_stopped", OBSMediaStopped, this);
		sigs.emplace_back(sh, "media_started", OBSMediaStarted, this);
		sigs.emplace_back(sh, "media_ended", OBSMediaStopped, this);
		sigs.emplace_back(sh, "media_next", OBSMediaNext, this);
		sigs.emplace_back(sh, "media_previous", OBSMediaPrevious, this);
	} else {
		weakSource = nullptr;
	}

	RefreshControls();
}

void MediaControls::SetSliderPosition()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	float time = (float)obs_source_media_get_time(source);
	float duration = (float)obs_source_media_get_duration(source);

	float sliderPosition;

	if (duration)
		sliderPosition = (time / duration) * (float)ui->slider->maximum();
	else
		sliderPosition = 0.0f;

	ui->slider->setValue((int)sliderPosition);
	UpdateLabels((int)sliderPosition);
}

QString MediaControls::FormatSeconds(int totalSeconds)
{
	int seconds = totalSeconds % 60;
	int totalMinutes = totalSeconds / 60;
	int minutes = totalMinutes % 60;
	int hours = totalMinutes / 60;

	return QString::asprintf("%02d:%02d:%02d", hours, minutes, seconds);
}

void MediaControls::on_playPauseButton_clicked()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	obs_media_state state = obs_source_media_get_state(source);

	switch (state) {
	case OBS_MEDIA_STATE_STOPPED:
	case OBS_MEDIA_STATE_ENDED:
		RestartMedia();
		break;
	case OBS_MEDIA_STATE_PLAYING:
		PauseMedia();
		break;
	case OBS_MEDIA_STATE_PAUSED:
		PlayMedia();
		break;
	default:
		break;
	}
}

void MediaControls::RestartMedia()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		obs_source_media_restart(source);
	}
}

void MediaControls::PlayMedia()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		obs_source_media_play_pause(source, false);
	}
}

void MediaControls::PauseMedia()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		obs_source_media_play_pause(source, true);
	}
}

void MediaControls::StopMedia()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		obs_source_media_stop(source);
	}
}

void MediaControls::PlaylistNext()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		obs_source_media_next(source);
	}
}

void MediaControls::PlaylistPrevious()
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (source) {
		obs_source_media_previous(source);
	}
}

void MediaControls::on_stopButton_clicked()
{
	StopMedia();
}

void MediaControls::on_nextButton_clicked()
{
	PlaylistNext();
}

void MediaControls::on_previousButton_clicked()
{
	PlaylistPrevious();
}

void MediaControls::on_durationLabel_clicked()
{
	countDownTimer = !countDownTimer;

	config_set_bool(App()->GetUserConfig(), "BasicWindow", "MediaControlsCountdownTimer", countDownTimer);

	if (MediaPaused())
		SetSliderPosition();
}

void MediaControls::MoveSliderFoward(int seconds)
{
	OBSSource source = OBSGetStrongRef(weakSource);

	if (!source)
		return;

	int ms = obs_source_media_get_time(source);
	ms += seconds * 1000;

	obs_source_media_set_time(source, ms);
	SetSliderPosition();
}

void MediaControls::MoveSliderBackwards(int seconds)
{
	OBSSource source = OBSGetStrongRef(weakSource);

	if (!source)
		return;

	int ms = obs_source_media_get_time(source);
	ms -= seconds * 1000;

	obs_source_media_set_time(source, ms);
	SetSliderPosition();
}

void MediaControls::UpdateSlideCounter()
{
	if (!isSlideshow)
		return;

	OBSSource source = OBSGetStrongRef(weakSource);

	if (!source)
		return;

	proc_handler_t *ph = obs_source_get_proc_handler(source);
	calldata_t cd = {};

	proc_handler_call(ph, "current_index", &cd);
	int slide = calldata_int(&cd, "current_index");

	proc_handler_call(ph, "total_files", &cd);
	int total = calldata_int(&cd, "total_files");
	calldata_free(&cd);

	if (total > 0) {
		ui->timerLabel->setText(QString::number(slide + 1));
		ui->durationLabel->setText(QString::number(total));
	} else {
		ui->timerLabel->setText("-");
		ui->durationLabel->setText("-");
	}
}

void MediaControls::UpdateLabels(int val)
{
	OBSSource source = OBSGetStrongRef(weakSource);
	if (!source) {
		return;
	}

	float duration = (float)obs_source_media_get_duration(source);
	float percent = (float)val / (float)ui->slider->maximum();

	float time = percent * duration;

	ui->timerLabel->setText(FormatSeconds((int)(time / 1000.0f)));

	if (!countDownTimer)
		ui->durationLabel->setText(FormatSeconds((int)(duration / 1000.0f)));
	else
		ui->durationLabel->setText(QStringLiteral("-") + FormatSeconds((int)((duration - time) / 1000.0f)));
}
