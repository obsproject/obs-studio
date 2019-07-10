#include <QToolTip>
#include "media-controls.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"

#define SLIDER_PRECISION 1024.0f

void MediaControls::OBSMediaStopped(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "ResetControls");
}

void MediaControls::OBSMediaPlay(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPauseIcon");
}

void MediaControls::OBSMediaPause(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "SetPlayIcon");
}

void MediaControls::OBSMediaStarted(void *data, calldata_t *calldata)
{
	UNUSED_PARAMETER(calldata);

	MediaControls *media = static_cast<MediaControls *>(data);
	QMetaObject::invokeMethod(media, "MediaStart");
}

MediaSlider::MediaSlider(QWidget *parent)
{
	setParent(parent);
	setMouseTracking(true);
}

MediaSlider::~MediaSlider()
{
	deleteLater();
}

void MediaSlider::mouseReleaseEvent(QMouseEvent *event)
{
	MediaControls *media = reinterpret_cast<MediaControls *>(parent());

	int val = minimum() + ((maximum() - minimum()) * event->x()) / width();

	mouseDown = false;

	media->MediaPlayPause(false);
	media->MediaSeekTo(val);

	event->accept();
}

void MediaSlider::mousePressEvent(QMouseEvent *event)
{
	MediaControls *media = reinterpret_cast<MediaControls *>(parent());

	media->MediaPlayPause(true);
	mouseDown = true;
	event->accept();
}

void MediaSlider::mouseMoveEvent(QMouseEvent *event)
{
	MediaControls *media = reinterpret_cast<MediaControls *>(parent());

	int val = minimum() + ((maximum() - minimum()) * event->x()) / width();
	float seconds =
		(((float)val / SLIDER_PRECISION) * media->GetMediaDuration()) /
		1000.0f;

	QToolTip::showText(QCursor::pos(), media->FormatSeconds((int)seconds),
			   this);

	if (mouseDown) {
		setValue(val);
	}

	event->accept();
}

MediaControls::MediaControls(QWidget *parent, OBSSource source_)
	: QWidget(parent), source(source_)
{
	main = reinterpret_cast<OBSBasic *>(App()->GetMainWindow());

	QVBoxLayout *vLayout = new QVBoxLayout();
	QHBoxLayout *layout = new QHBoxLayout();
	QHBoxLayout *topLayout = new QHBoxLayout();

	nameLabel = new QLabel();
	timerLabel = new QLabel();
	durationLabel = new QLabel();
	nameLabel->setProperty("themeID", "mediaControls");
	timerLabel->setProperty("themeID", "mediaControls");
	durationLabel->setProperty("themeID", "mediaControls");

	SetName(QT_UTF8(obs_source_get_name(source)));

	playPauseButton = new QPushButton();
	stopButton = new QPushButton();
	stopButton->setProperty("themeID", "stopIcon");
	nextButton = new QPushButton();
	nextButton->setProperty("themeID", "nextIcon");
	previousButton = new QPushButton();
	previousButton->setProperty("themeID", "previousIcon");
	configButton = new QPushButton();
	configButton->setProperty("themeID", "configIconSmall");
	playPauseButton->setStyleSheet("background: transparent; border:none;");
	stopButton->setStyleSheet("background: transparent; border:none;");
	nextButton->setStyleSheet("background: transparent; border:none;");
	previousButton->setStyleSheet("background: transparent; border:none;");
	configButton->setStyleSheet("background: transparent; border:none;");

	playPauseButton->setFixedSize(32, 32);
	stopButton->setFixedSize(32, 32);
	nextButton->setFixedSize(32, 32);
	previousButton->setFixedSize(32, 32);
	configButton->setFixedSize(32, 32);

	slider = new MediaSlider();
	slider->setProperty("themeID", "mediaControls");
	slider->setOrientation(Qt::Horizontal);
	slider->setMinimum(0);
	slider->setMaximum(int(SLIDER_PRECISION));

	topLayout->addWidget(timerLabel);
	topLayout->addWidget(slider);
	topLayout->addWidget(durationLabel);
	topLayout->setContentsMargins(0, 0, 0, 0);

	layout->addWidget(playPauseButton);
	layout->addWidget(previousButton);
	layout->addWidget(stopButton);
	layout->addWidget(nextButton);
	layout->addStretch();
	layout->addWidget(configButton);
	layout->setContentsMargins(0, 0, 0, 0);

	vLayout->addWidget(nameLabel);
	vLayout->addLayout(topLayout);
	vLayout->addLayout(layout);
	vLayout->setContentsMargins(4, 4, 4, 4);

	setLayout(vLayout);

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(SetSliderPosition()));

	connect(playPauseButton, SIGNAL(clicked()), this,
		SLOT(MediaPlayPause()));
	connect(stopButton, SIGNAL(clicked()), this, SLOT(MediaStop()));
	connect(nextButton, SIGNAL(clicked()), this, SLOT(MediaNext()));
	connect(previousButton, SIGNAL(clicked()), this, SLOT(MediaPrevious()));
	connect(configButton, SIGNAL(clicked()), this, SLOT(ShowConfigMenu()));

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

	timerLabel->setText("00:00:00");
	durationLabel->setText("00:00:00");

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_PLAYING) {
		StartTimer();
		SetPauseIcon();
	} else {
		SetPlayIcon();
	}
}

MediaControls::~MediaControls()
{
	deleteLater();

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

OBSSource MediaControls::GetSource()
{
	return source;
}

QString MediaControls::GetName()
{
	return nameLabel->text();
}

void MediaControls::SetName(const QString &newName)
{
	nameLabel->setText(newName);
}

int64_t MediaControls::GetMediaTime()
{
	return obs_source_media_get_time(source);
}

int64_t MediaControls::GetMediaDuration()
{
	return obs_source_media_get_duration(source);
}

void MediaControls::SetSliderPosition()
{
	float time = (float)GetMediaTime();
	float duration = (float)GetMediaDuration();

	float sliderPosition = (time / duration) * SLIDER_PRECISION;

	slider->setValue((int)sliderPosition);

	timerLabel->setText(FormatSeconds((int)(time / 1000.0f)));
	durationLabel->setText(FormatSeconds((int)(duration / 1000.0f)));
}

void MediaControls::MediaSeekTo(int value)
{
	float ms = ((float)value / SLIDER_PRECISION) *
		   obs_source_media_get_duration(source);

	obs_source_media_set_time(source, (int64_t)ms);
}

void MediaControls::SetPlayIcon()
{
	playPauseButton->setProperty("themeID", "playIcon");

	playPauseButton->style()->unpolish(playPauseButton);
	playPauseButton->style()->polish(playPauseButton);
}

void MediaControls::SetPauseIcon()
{
	playPauseButton->setProperty("themeID", "pauseIcon");

	playPauseButton->style()->unpolish(playPauseButton);
	playPauseButton->style()->polish(playPauseButton);
}

void MediaControls::SetRestartIcon()
{
	playPauseButton->setProperty("themeID", "restartIcon");

	playPauseButton->style()->unpolish(playPauseButton);
	playPauseButton->style()->polish(playPauseButton);
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

void MediaControls::MediaPlayPause()
{
	playPauseButton->blockSignals(true);

	obs_media_state state = obs_source_media_get_state(source);

	if (state == OBS_MEDIA_STATE_STOPPED || state == OBS_MEDIA_STATE_ENDED)
		obs_source_media_restart(source);
	else if (state == OBS_MEDIA_STATE_PLAYING)
		MediaPlayPause(true);
	else if (state == OBS_MEDIA_STATE_PAUSED)
		MediaPlayPause(false);

	playPauseButton->blockSignals(false);
}

void MediaControls::MediaPlayPause(bool pause)
{
	playPauseButton->blockSignals(true);

	if (pause)
		StopTimer();
	else
		StartTimer();

	obs_source_media_play_pause(source, pause);
	playPauseButton->blockSignals(false);
}

void MediaControls::MediaStart()
{
	StartTimer();
	SetPauseIcon();
	slider->setEnabled(true);
}

void MediaControls::MediaStop()
{
	stopButton->blockSignals(true);
	obs_source_media_stop(source);
	stopButton->blockSignals(false);
}

void MediaControls::MediaNext()
{
	nextButton->blockSignals(true);
	obs_source_media_next(source);
	nextButton->blockSignals(false);
}

void MediaControls::MediaPrevious()
{
	previousButton->blockSignals(true);
	obs_source_media_previous(source);
	previousButton->blockSignals(false);
}

void MediaControls::ResetControls()
{
	StopTimer();
	SetRestartIcon();
	slider->setValue(0);
	slider->setEnabled(false);
	timerLabel->setText("00:00:00");
	durationLabel->setText("00:00:00");
}

void MediaControls::ShowConfigMenu()
{
	QAction *unhideAllAction = new QAction(QTStr("UnhideAll"), this);
	QAction *hideAction = new QAction(QTStr("Hide"), this);

	QAction *filtersAction = new QAction(QTStr("Filters"), this);
	QAction *propertiesAction = new QAction(QTStr("Properties"), this);

	connect(hideAction, SIGNAL(triggered()), this,
		SLOT(HideMediaControls()));
	connect(unhideAllAction, SIGNAL(triggered()), this,
		SLOT(UnhideAllMediaControls()));

	connect(filtersAction, SIGNAL(triggered()), this, SLOT(MediaFilters()));
	connect(propertiesAction, SIGNAL(triggered()), this,
		SLOT(MediaProperties()));

	QMenu popup;
	popup.addAction(unhideAllAction);
	popup.addAction(hideAction);
	popup.addSeparator();
	popup.addAction(filtersAction);
	popup.addAction(propertiesAction);
	popup.exec(QCursor::pos());
}

void MediaControls::UnhideAllMediaControls()
{
	main->UnhideMediaControls();
}

void MediaControls::HideMediaControls()
{
	obs_data_t *settings = obs_source_get_settings(source);
	obs_data_set_bool(settings, "hideMediaControls", true);
	obs_data_release(settings);

	main->HideMediaControls(source);
}

void MediaControls::MediaFilters()
{
	main->CreateFiltersWindow(source);
}

void MediaControls::MediaProperties()
{
	main->CreatePropertiesWindow(source);
}
