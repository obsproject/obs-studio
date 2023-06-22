// SPDX-FileCopyrightText: 2021 Yuriy Chumak <yuriy.chumak@mail.com>
// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "window-youtube-actions.hpp"

#include <QToolTip>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QStandardPaths>
#include <QImageReader>

#include <QFileDialog>
#include <QMimeDatabase>

const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";
const QString RepresentSchedulDateAndTimeFormat = "dddd, MMMM d, yyyy h:m";

static QString GetTranslatedError(const RequestError &error)
{
	if (error.type != RequestErrorType::UNKNOWN_OR_CUSTOM ||
	    error.error.empty())
		return QString();

	std::string lookupString = "YouTube.Errors.";
	lookupString += error.error;

	return QT_UTF8(obs_module_text(lookupString.c_str()));
}

using namespace YouTubeApi;

OBSYoutubeActions::OBSYoutubeActions(QWidget *parent,
				     YouTubeApi::ServiceOAuth *auth,
				     OBSYoutubeActionsSettings *settings,
				     bool broadcastReady)
	: QDialog(parent),
	  ui(new Ui::OBSYoutubeActions),
	  apiYouTube(auth),
	  workerThread(new WorkerThread(apiYouTube)),
	  settings(settings),
	  broadcastReady(broadcastReady)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	ui->setupUi(this);
	ui->privacyBox->addItem(
		QT_UTF8(obs_module_text("YouTube.Actions.Privacy.Public")),
		(int)LiveBroadcastPrivacyStatus::PUBLIC);
	ui->privacyBox->addItem(
		QT_UTF8(obs_module_text("YouTube.Actions.Privacy.Unlisted")),
		(int)LiveBroadcastPrivacyStatus::UNLISTED);
	ui->privacyBox->addItem(
		QT_UTF8(obs_module_text("YouTube.Actions.Privacy.Private")),
		(int)LiveBroadcastPrivacyStatus::PRIVATE);

	ui->latencyBox->addItem(
		QT_UTF8(obs_module_text("YouTube.Actions.Latency.Normal")),
		(int)LiveBroadcastLatencyPreference::NORMAL);
	ui->latencyBox->addItem(
		QT_UTF8(obs_module_text("YouTube.Actions.Latency.Low")),
		(int)LiveBroadcastLatencyPreference::LOW);
	ui->latencyBox->addItem(
		QT_UTF8(obs_module_text("YouTube.Actions.Latency.UltraLow")),
		(int)LiveBroadcastLatencyPreference::ULTRALOW);

	UpdateOkButtonStatus();

	connect(ui->title, &QLineEdit::textChanged, this,
		[&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->privacyBox, &QComboBox::currentTextChanged, this,
		[&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->yesMakeForKids, &QRadioButton::toggled, this,
		[&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->notMakeForKids, &QRadioButton::toggled, this,
		[&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->tabWidget, &QTabWidget::currentChanged, this,
		[&](int) { this->UpdateOkButtonStatus(); });
	connect(ui->pushButton, &QPushButton::clicked, this,
		&OBSYoutubeActions::OpenYouTubeDashboard);

	connect(ui->helpAutoStartStop, &QLabel::linkActivated, this,
		[](const QString &) {
			QToolTip::showText(
				QCursor::pos(),
				obs_module_text(
					"YouTube.Actions.AutoStartStop.TT"));
		});
	connect(ui->help360Video, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });
	connect(ui->helpMadeForKids, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });

	ui->scheduledTime->setVisible(false);
	connect(ui->checkScheduledLater, &QCheckBox::stateChanged, this,
		[&](int state) {
			ui->scheduledTime->setVisible(state);
			if (state) {
				ui->checkAutoStart->setVisible(true);
				ui->checkAutoStop->setVisible(true);
				ui->helpAutoStartStop->setVisible(true);

				ui->checkAutoStart->setChecked(false);
				ui->checkAutoStop->setChecked(false);
			} else {
				ui->checkAutoStart->setVisible(false);
				ui->checkAutoStop->setVisible(false);
				ui->helpAutoStartStop->setVisible(false);

				ui->checkAutoStart->setChecked(true);
				ui->checkAutoStop->setChecked(true);
			}
			UpdateOkButtonStatus();
		});

	ui->checkAutoStart->setVisible(false);
	ui->checkAutoStop->setVisible(false);
	ui->helpAutoStartStop->setVisible(false);

	ui->scheduledTime->setDateTime(QDateTime::currentDateTime());

	auto thumbSelectionHandler = [&]() {
		if (thumbnailFilePath.isEmpty()) {
			QString filePath = QFileDialog::getOpenFileName(
				this,
				obs_module_text(
					"YouTube.Actions.Thumbnail.SelectFile"),
				QStandardPaths::writableLocation(
					QStandardPaths::PicturesLocation),
				QString("Images (*.png *.jpg *.jpeg *.gif)"));

			if (!filePath.isEmpty()) {
				QFileInfo tFile(filePath);
				if (!tFile.exists()) {
					return ShowErrorDialog(
						this,
						obs_module_text(
							"YouTube.Actions.Error.FileMissing"));
				} else if (tFile.size() > 2 * 1024 * 1024) {
					return ShowErrorDialog(
						this,
						obs_module_text(
							"YouTube.Actions.Error.FileTooLarge"));
				}

				QFile thumbFile(filePath);
				if (!thumbFile.open(QFile::ReadOnly)) {
					return ShowErrorDialog(
						this,
						obs_module_text(
							"YouTube.Actions.Error.FileOpeningFailed"));
				}

				thumbnailData = thumbFile.readAll();
				thumbnailFilePath = filePath;
				ui->selectedFileName->setText(
					thumbnailFilePath);
				ui->selectFileButton->setText(obs_module_text(
					"YouTube.Actions.Thumbnail.ClearFile"));

				QImageReader imgReader(filePath);
				imgReader.setAutoTransform(true);
				const QImage newImage = imgReader.read();
				ui->thumbnailPreview->setPixmap(
					QPixmap::fromImage(newImage).scaled(
						160, 90, Qt::KeepAspectRatio,
						Qt::SmoothTransformation));
			}
		} else {
			thumbnailData.clear();
			thumbnailFilePath.clear();
			ui->selectedFileName->setText(obs_module_text(
				"YouTube.Actions.Thumbnail.NoFileSelected"));
			ui->selectFileButton->setText(obs_module_text(
				"YouTube.Actions.Thumbnail.SelectFile"));
			ui->thumbnailPreview->setPixmap(
				GetPlaceholder().pixmap(QSize(16, 16)));
		}
	};

	connect(ui->selectFileButton, &QPushButton::clicked, this,
		thumbSelectionHandler);
	connect(ui->thumbnailPreview, &ClickableLabel::clicked, this,
		thumbSelectionHandler);

	if (!apiYouTube) {
		blog(LOG_DEBUG, "YouTube API auth NOT found.");
		Cancel();
		return;
	}

	ChannelInfo info;
	if (!apiYouTube->GetChannelInfo(info)) {
		QString error = GetTranslatedError(apiYouTube->GetLastError());
		ShowErrorDialog(
			parent,
			error.isEmpty()
				? obs_module_text(
					  "YouTube.Actions.Error.General")
				: QT_UTF8(obs_module_text(
						  "YouTube.Actions.Error.Text"))
					  .arg(error));
		Cancel();
		return;
	};
	this->setWindowTitle(
		QT_UTF8(obs_module_text("YouTube.Actions.WindowTitle"))
			.arg(QString::fromStdString(info.title)));

	std::vector<VideoCategory> category_list;
	if (!apiYouTube->GetVideoCategoriesList(category_list)) {
		QString error = QString::fromStdString(
			apiYouTube->GetLastError().message);
		ShowErrorDialog(
			parent,
			error.isEmpty()
				? obs_module_text(
					  "YouTube.Actions.Error.General")
				: QT_UTF8(obs_module_text(
						  "YouTube.Actions.Error.Text"))
					  .arg(error));
		Cancel();
		return;
	}
	for (auto &category : category_list) {
		ui->categoryBox->addItem(
			QString::fromStdString(category.snippet.title),
			QString::fromStdString(category.id));
	}
	/* ID 20 is Gaming category */
	ui->categoryBox->setCurrentIndex(ui->categoryBox->findData("20"));

	connect(ui->okButton, &QPushButton::clicked, this,
		&OBSYoutubeActions::InitBroadcast);
	connect(ui->saveButton, &QPushButton::clicked, this,
		&OBSYoutubeActions::ReadyBroadcast);
	connect(ui->cancelButton, &QPushButton::clicked, this, [&]() {
		blog(LOG_DEBUG, "YouTube live broadcast creation cancelled.");
		// Close the dialog.
		Cancel();
	});

	qDeleteAll(ui->scrollAreaWidgetContents->findChildren<QWidget *>(
		QString(), Qt::FindDirectChildrenOnly));

	// Add label indicating loading state
	QLabel *loadingLabel = new QLabel();
	loadingLabel->setTextFormat(Qt::RichText);
	loadingLabel->setAlignment(Qt::AlignHCenter);
	loadingLabel->setText(
		QString("<big>%1</big>")
			.arg(obs_module_text("YouTube.Actions.EventsLoading")));
	ui->scrollAreaWidgetContents->layout()->addWidget(loadingLabel);

	// Delete "loading..." label on completion
	connect(workerThread, &WorkerThread::finished, this, [&] {
		QLayoutItem *item =
			ui->scrollAreaWidgetContents->layout()->takeAt(0);
		item->widget()->deleteLater();
	});

	connect(workerThread, &WorkerThread::failed, this, [&]() {
		RequestError lastError = apiYouTube->GetLastError();

		QString error = GetTranslatedError(lastError);

		if (error.isEmpty())
			error = QString::fromStdString(lastError.message);

		ShowErrorDialog(
			this,
			error.isEmpty()
				? QT_UTF8(obs_module_text(
					  "YouTube.Actions.Error.YouTubeApi"))
				: QT_UTF8(obs_module_text(
						  "YouTube.Actions.Error.Text"))
					  .arg(error));
		QDialog::reject();
	});

	connect(workerThread, &WorkerThread::new_item, this,
		[&](const QString &title, const QString &dateTimeString,
		    const QString &broadcast,
		    const LiveBroadcastLifeCycleStatus &status, bool astart,
		    bool astop) {
			ClickableLabel *label = new ClickableLabel();
			label->setTextFormat(Qt::RichText);

			if (status == LiveBroadcastLifeCycleStatus::LIVE ||
			    status == LiveBroadcastLifeCycleStatus::TESTING) {
				// Resumable stream
				label->setText(
					QString("<big>%1</big><br/>%2")
						.arg(title,
						     obs_module_text(
							     "YouTube.Actions.Stream.Resume")));

			} else if (dateTimeString.isEmpty()) {
				// The broadcast created by YouTube Studio has no start time.
				// Yes this does violate the restrictions set in YouTube's API
				// But why would YouTube care about consistency?
				label->setText(
					QString("<big>%1</big><br/>%2")
						.arg(title,
						     obs_module_text(
							     "YouTube.Actions.Stream.YTStudio")));
			} else {
				label->setText(
					QString("<big>%1</big><br/>%2")
						.arg(title,
						     QT_UTF8(obs_module_text(
								     "YouTube.Actions.Stream.ScheduledFor"))
							     .arg(dateTimeString)));
			}

			label->setAlignment(Qt::AlignHCenter);
			label->setMargin(4);

			connect(label, &ClickableLabel::clicked, this,
				[&, label, broadcast, astart, astop]() {
					for (QWidget *i :
					     ui->scrollAreaWidgetContents->findChildren<
						     QWidget *>(
						     QString(),
						     Qt::FindDirectChildrenOnly)) {

						i->setProperty(
							"isSelectedEvent",
							"false");
						i->style()->unpolish(i);
						i->style()->polish(i);
					}
					label->setProperty("isSelectedEvent",
							   "true");
					label->style()->unpolish(label);
					label->style()->polish(label);

					this->selectedBroadcast = broadcast;
					this->autostart = astart;
					this->autostop = astop;
					UpdateOkButtonStatus();
				});
			ui->scrollAreaWidgetContents->layout()->addWidget(
				label);

			if (selectedBroadcast == broadcast)
				label->clicked();
		});
	workerThread->start();

	if (settings->rememberSettings)
		LoadSettings();

	// Switch to events page and select readied broadcast once loaded
	if (broadcastReady) {
		ui->tabWidget->setCurrentIndex(1);
		selectedBroadcast =
			QString::fromStdString(apiYouTube->GetBroadcastId());
	}

#ifdef __APPLE__
	// MacOS theming issues
	this->resize(this->width() + 200, this->height() + 120);
#endif
	valid = true;
}

void OBSYoutubeActions::showEvent(QShowEvent *event)
{
	QDialog::showEvent(event);
	if (thumbnailFilePath.isEmpty())
		ui->thumbnailPreview->setPixmap(
			GetPlaceholder().pixmap(QSize(16, 16)));
}

OBSYoutubeActions::~OBSYoutubeActions()
{
	workerThread->stop();
	workerThread->wait();

	delete workerThread;
}

void WorkerThread::run()
{
	if (!pending)
		return;

	for (LiveBroadcastListStatus broadcastStatus :
	     {LiveBroadcastListStatus::ACTIVE,
	      LiveBroadcastListStatus::UPCOMMING}) {

		std::vector<LiveBroadcast> broadcasts;
		if (!apiYouTube->GetLiveBroadcastsList(broadcastStatus,
						       broadcasts)) {
			emit failed();
			return;
		}

		for (LiveBroadcast item : broadcasts) {
			LiveBroadcastLifeCycleStatus status =
				item.status.lifeCycleStatus;

			if ((status == LiveBroadcastLifeCycleStatus::LIVE ||
			     status == LiveBroadcastLifeCycleStatus::TESTING) &&
			    item.contentDetails.boundStreamId.has_value()) {
				// Check that the attached liveStream is offline (reconnectable)
				LiveStream stream;
				if (!apiYouTube->FindLiveStream(
					    item.contentDetails.boundStreamId
						    .value(),
					    stream))
					continue;
				if (stream.status.streamStatus ==
				    LiveStreamStatusEnum::ACTIVE)
					continue;
			}

			QString title =
				QString::fromStdString(item.snippet.title);
			QString scheduledStartTime = QString::fromStdString(
				item.snippet.scheduledStartTime.value_or(""));
			QString broadcast = QString::fromStdString(item.id);

			// Treat already started streams as autostart for UI purposes
			bool astart =
				status == LiveBroadcastLifeCycleStatus::LIVE ||
				item.contentDetails.enableAutoStart;
			bool astop = item.contentDetails.enableAutoStop;

			QDateTime utcDTime = QDateTime::fromString(
				scheduledStartTime, SchedulDateAndTimeFormat);
			// DateTime parser means that input datetime is a local, so we need to move it
			QDateTime dateTime =
				utcDTime.addSecs(utcDTime.offsetFromUtc());

			QString dateTimeString = QLocale().toString(
				dateTime,
				QString("%1  %2").arg(
					QLocale().dateFormat(
						QLocale::LongFormat),
					QLocale().timeFormat(
						QLocale::ShortFormat)));

			emit new_item(title, dateTimeString, broadcast, status,
				      astart, astop);

			if (!pending)
				return;
		}
	}

	emit ready();
}

void OBSYoutubeActions::UpdateOkButtonStatus()
{
	bool enable = false;

	if (ui->tabWidget->currentIndex() == 0) {
		enable = !ui->title->text().isEmpty() &&
			 !ui->privacyBox->currentText().isEmpty() &&
			 (ui->yesMakeForKids->isChecked() ||
			  ui->notMakeForKids->isChecked());
		ui->okButton->setEnabled(enable);
		ui->saveButton->setEnabled(enable);

		if (ui->checkScheduledLater->checkState() == Qt::Checked) {
			ui->okButton->setText(QT_UTF8(obs_module_text(
				"YouTube.Actions.Create_Schedule")));
			ui->saveButton->setText(QT_UTF8(obs_module_text(
				"YouTube.Actions.Create_Schedule_Ready")));
		} else {
			ui->okButton->setText(QT_UTF8(obs_module_text(
				"YouTube.Actions.Create_GoLive")));
			ui->saveButton->setText(QT_UTF8(obs_module_text(
				"YouTube.Actions.Create_Ready")));
		}
		ui->pushButton->setVisible(false);
	} else {
		enable = !selectedBroadcast.isEmpty();
		ui->okButton->setEnabled(enable);
		ui->saveButton->setEnabled(enable);
		ui->okButton->setText(QT_UTF8(
			obs_module_text("YouTube.Actions.Choose_GoLive")));
		ui->saveButton->setText(QT_UTF8(
			obs_module_text("YouTube.Actions.Choose_Ready")));

		ui->pushButton->setVisible(true);
	}
}
bool OBSYoutubeActions::CreateEventAction(LiveStream &stream, bool stream_later,
					  bool ready_broadcast)
{
	LiveBroadcast broadcast = UiToBroadcast();

	if (stream_later) {
		// DateTime parser means that input datetime is a local, so we need to move it
		auto dateTime = ui->scheduledTime->dateTime();
		auto utcDTime = dateTime.addSecs(-dateTime.offsetFromUtc());
		broadcast.snippet.scheduledStartTime =
			utcDTime.toString(SchedulDateAndTimeFormat)
				.toStdString();
	} else {
		// stream now is always autostart/autostop
		broadcast.contentDetails.enableAutoStart = true;
		broadcast.contentDetails.enableAutoStop = true;
		broadcast.snippet.scheduledStartTime =
			QDateTime::currentDateTimeUtc()
				.toString(SchedulDateAndTimeFormat)
				.toStdString();
	}

	autostart = broadcast.contentDetails.enableAutoStart;
	autostop = broadcast.contentDetails.enableAutoStop;

	blog(LOG_DEBUG, "Scheduled date and time: %s",
	     broadcast.snippet.scheduledStartTime.value().c_str());
	if (!apiYouTube->InsertLiveBroadcast(broadcast)) {
		blog(LOG_DEBUG, "No broadcast created.");
		return false;
	}
	Video video;
	video.id = broadcast.id;
	video.snippet.title = broadcast.snippet.title;
	video.snippet.description = broadcast.snippet.description;
	video.snippet.categoryId =
		ui->categoryBox->currentData().toString().toStdString();
	if (!apiYouTube->UpdateVideo(video)) {
		blog(LOG_DEBUG, "No category set.");
		return false;
	}
	if (!thumbnailFilePath.isEmpty()) {
		blog(LOG_INFO, "Uploading thumbnail file \"%s\"...",
		     thumbnailFilePath.toStdString().c_str());

		const QString mime =
			QMimeDatabase().mimeTypeForData(thumbnailData).name();

		if (!apiYouTube->SetThumbnail(broadcast.id, QT_TO_UTF8(mime),
					      thumbnailData.constData(),
					      thumbnailData.size())) {
			blog(LOG_DEBUG, "No thumbnail set.");
			return false;
		}
	}

	if (!stream_later || ready_broadcast) {
		stream.snippet.title = "OBS Studio Video Stream";
		stream.cdn.ingestionType = apiYouTube->GetIngestionType();
		stream.cdn.resolution =
			LiveStreamCdnResolution::RESOLUTION_VARIABLE;
		stream.cdn.frameRate =
			LiveStreamCdnFrameRate::FRAMERATE_VARIABLE;
		LiveStreamContentDetails contentDetails;
		contentDetails.isReusable = false;
		stream.contentDetails = contentDetails;
		if (!apiYouTube->InsertLiveStream(stream)) {
			blog(LOG_DEBUG, "No stream created.");
			return false;
		}

		if (!apiYouTube->BindLiveBroadcast(broadcast, stream.id)) {
			blog(LOG_DEBUG, "No stream binded.");
			return false;
		}

		apiYouTube->SetBroadcastId(broadcast.id);

		if (broadcast.status.privacyStatus !=
			    LiveBroadcastPrivacyStatus::PRIVATE &&
		    broadcast.snippet.liveChatId.has_value()) {
			const std::string apiLiveChatId =
				broadcast.snippet.liveChatId.value();
			apiYouTube->SetChatIds(broadcast.id, apiLiveChatId);
		} else {
			apiYouTube->ResetChat();
		}
	}

	return true;
}

bool OBSYoutubeActions::ChooseAnEventAction(LiveStream &stream)
{
	LiveBroadcast broadcast;
	if (!apiYouTube->FindLiveBroadcast(selectedBroadcast.toStdString(),
					   broadcast)) {
		blog(LOG_DEBUG, "No broadcast found.");
		return false;
	}

	std::string boundStreamId =
		broadcast.contentDetails.boundStreamId.value_or("");
	bool bindStream = true;
	if (!boundStreamId.empty() &&
	    apiYouTube->FindLiveStream(boundStreamId, stream)) {

		if (stream.cdn.ingestionType !=
		    apiYouTube->GetIngestionType()) {
			blog(LOG_WARNING,
			     "Missmatch ingestion types, binding a new stream required");
			if (!apiYouTube->BindLiveBroadcast(broadcast)) {
				blog(LOG_DEBUG, "Could not unbind stream");
				return false;
			}
		} else {
			bindStream = false;
		}
	}

	if (bindStream) {
		stream.snippet.title = "OBS Studio Video Stream";
		stream.cdn.ingestionType = apiYouTube->GetIngestionType();
		stream.cdn.resolution =
			LiveStreamCdnResolution::RESOLUTION_VARIABLE;
		stream.cdn.frameRate =
			LiveStreamCdnFrameRate::FRAMERATE_VARIABLE;
		LiveStreamContentDetails contentDetails;
		contentDetails.isReusable = false;
		stream.contentDetails = contentDetails;
		if (!apiYouTube->InsertLiveStream(stream)) {
			blog(LOG_DEBUG, "No stream created.");
			return false;
		}
		if (!apiYouTube->BindLiveBroadcast(broadcast, stream.id)) {
			blog(LOG_DEBUG, "No stream binded.");
			return false;
		}
	}

	apiYouTube->SetBroadcastId(broadcast.id);

	if (broadcast.status.privacyStatus !=
		    LiveBroadcastPrivacyStatus::PRIVATE &&
	    broadcast.snippet.liveChatId.has_value()) {
		const std::string apiLiveChatId =
			broadcast.snippet.liveChatId.value();
		apiYouTube->SetChatIds(broadcast.id, apiLiveChatId);
	} else {
		apiYouTube->ResetChat();
	}

	return true;
}

void OBSYoutubeActions::ShowErrorDialog(QWidget *parent, QString text)
{
	QMessageBox dlg(parent);
	dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	dlg.setWindowTitle(obs_module_text("YouTube.Actions.Error.Title"));
	dlg.setText(text);
	dlg.setTextFormat(Qt::RichText);
	dlg.setIcon(QMessageBox::Warning);
	dlg.setStandardButtons(QMessageBox::StandardButton::Ok);
	dlg.exec();
}

void OBSYoutubeActions::InitBroadcast()
{
	LiveStream stream;
	QMessageBox msgBox(this);
	msgBox.setWindowFlags(msgBox.windowFlags() &
			      ~Qt::WindowCloseButtonHint);
	msgBox.setWindowTitle(obs_module_text("YouTube.Actions.Notify.Title"));
	msgBox.setText(
		obs_module_text("YouTube.Actions.Notify.CreatingBroadcast"));
	msgBox.setStandardButtons(QMessageBox::StandardButtons());

	bool success = false;
	auto action = [&]() {
		if (ui->tabWidget->currentIndex() == 0) {
			success = this->CreateEventAction(
				stream, ui->checkScheduledLater->isChecked());
		} else {
			success = this->ChooseAnEventAction(stream);
		};
		QMetaObject::invokeMethod(&msgBox, "accept",
					  Qt::QueuedConnection);
	};
	QScopedPointer<QThread> thread(new QuickThread(action));
	thread->start();
	msgBox.exec();
	thread->wait();

	if (success) {
		if (ui->tabWidget->currentIndex() == 0) {
			// Stream later usecase.
			if (ui->checkScheduledLater->isChecked()) {
				QMessageBox msg(this);
				msg.setWindowTitle(obs_module_text(
					"YouTube.Actions.EventCreated.Title"));
				msg.setText(obs_module_text(
					"YouTube.Actions.EventCreated.Text"));
				msg.setStandardButtons(QMessageBox::Ok);
				msg.exec();
				// Close dialog without start streaming.
				Cancel();
			} else {
				// Stream now usecase.
				blog(LOG_DEBUG, "New valid stream: %s",
				     stream.cdn.ingestionInfo.streamName
					     .c_str());
				apiYouTube->SetNewStream(
					stream.id,
					stream.cdn.ingestionInfo.streamName,
					true, true, true);
				Accept();
			}
		} else {
			// Stream to precreated broadcast usecase.
			apiYouTube->SetNewStream(
				stream.id, stream.cdn.ingestionInfo.streamName,
				autostart, autostop, true);
			Accept();
		}
	} else {
		// Fail.
		RequestError lastError = apiYouTube->GetLastError();
		QString error = GetTranslatedError(lastError);
		ShowErrorDialog(
			this,
			error.isEmpty()
				? obs_module_text(
					  "YouTube.Actions.Error.YouTubeApi")
				: QT_UTF8(obs_module_text(
						  "YouTube.Actions.Error.NoBroadcastCreated"))
					  .arg(error));
	}
}

void OBSYoutubeActions::ReadyBroadcast()
{
	LiveStream stream;
	QMessageBox msgBox(this);
	msgBox.setWindowFlags(msgBox.windowFlags() &
			      ~Qt::WindowCloseButtonHint);
	msgBox.setWindowTitle(obs_module_text("YouTube.Actions.Notify.Title"));
	msgBox.setText(
		obs_module_text("YouTube.Actions.Notify.CreatingBroadcast"));
	msgBox.setStandardButtons(QMessageBox::StandardButtons());

	bool success = false;
	auto action = [&]() {
		if (ui->tabWidget->currentIndex() == 0) {
			success = this->CreateEventAction(
				stream, ui->checkScheduledLater->isChecked(),
				true);
		} else {
			success = this->ChooseAnEventAction(stream);
		};
		QMetaObject::invokeMethod(&msgBox, "accept",
					  Qt::QueuedConnection);
	};
	QScopedPointer<QThread> thread(new QuickThread(action));
	thread->start();
	msgBox.exec();
	thread->wait();

	if (success) {
		apiYouTube->SetNewStream(stream.id,
					 stream.cdn.ingestionInfo.streamName,
					 autostart, autostop, false);
		Accept();
	} else {
		// Fail.
		RequestError lastError = apiYouTube->GetLastError();
		QString error = GetTranslatedError(lastError);
		ShowErrorDialog(
			this,
			error.isEmpty()
				? obs_module_text(
					  "YouTube.Actions.Error.YouTubeApi")
				: QT_UTF8(obs_module_text(
						  "YouTube.Actions.Error.NoBroadcastCreated"))
					  .arg(error));
	}
}

LiveBroadcast OBSYoutubeActions::UiToBroadcast()
{
	LiveBroadcast broadcast;

	broadcast.snippet.title = ui->title->text().toStdString();
	// ToDo: UI warning rather than silent truncation
	broadcast.snippet.description =
		ui->description->toPlainText().left(5000).toStdString();
	broadcast.status.privacyStatus =
		(LiveBroadcastPrivacyStatus)ui->privacyBox->currentData()
			.toInt();
	broadcast.status.selfDeclaredMadeForKids =
		ui->yesMakeForKids->isChecked();
	broadcast.contentDetails.latencyPreference =
		(LiveBroadcastLatencyPreference)ui->latencyBox->currentData()
			.toInt();
	broadcast.contentDetails.enableAutoStart =
		ui->checkAutoStart->isChecked();
	broadcast.contentDetails.enableAutoStop =
		ui->checkAutoStop->isChecked();
	broadcast.contentDetails.enableDvr = ui->checkDVR->isChecked();
	broadcast.contentDetails.projection =
		ui->check360Video->isChecked()
			? LiveBroadcastProjection::THREE_HUNDRED_SIXTY
			: LiveBroadcastProjection::RECTANGULAR;

	SaveSettings();

	return broadcast;
}

void OBSYoutubeActions::SaveSettings()
{
	settings->rememberSettings = ui->checkRememberSettings->isChecked();

	if (!settings->rememberSettings)
		return;

	settings->title = ui->title->text().toStdString();
	// ToDo: UI warning rather than silent truncation
	settings->description =
		ui->description->toPlainText().left(5000).toStdString();
	settings->privacy =
		(LiveBroadcastPrivacyStatus)ui->privacyBox->currentData()
			.toInt();
	settings->categoryId =
		ui->categoryBox->currentData().toString().toStdString();
	settings->madeForKids = ui->yesMakeForKids->isChecked();
	settings->latency =
		(LiveBroadcastLatencyPreference)ui->latencyBox->currentData()
			.toInt();
	settings->autoStart = ui->checkAutoStart->isChecked();
	settings->autoStop = ui->checkAutoStop->isChecked();
	settings->dvr = ui->checkDVR->isChecked();
	settings->scheduleForLater = ui->checkScheduledLater->isChecked();
	settings->projection =
		ui->check360Video->isChecked()
			? LiveBroadcastProjection::THREE_HUNDRED_SIXTY
			: LiveBroadcastProjection::RECTANGULAR;
	settings->thumbnailFile = thumbnailFilePath.toStdString();
}

void OBSYoutubeActions::LoadSettings()
{
	ui->checkRememberSettings->setChecked(settings->rememberSettings);

	ui->title->setText(QString::fromStdString(settings->title));

	ui->description->setPlainText(
		QString::fromStdString(settings->description));

	int index = ui->privacyBox->findData((int)settings->privacy);
	ui->privacyBox->setCurrentIndex(index);

	index = ui->categoryBox->findData(
		QString::fromStdString(settings->categoryId));
	ui->categoryBox->setCurrentIndex(index);

	index = ui->latencyBox->findData((int)settings->latency);
	ui->latencyBox->setCurrentIndex(index);

	ui->checkDVR->setChecked(settings->dvr);

	if (settings->madeForKids)
		ui->yesMakeForKids->setChecked(true);
	else
		ui->notMakeForKids->setChecked(true);

	ui->checkScheduledLater->setChecked(settings->scheduleForLater);

	ui->checkAutoStart->setChecked(settings->autoStart);

	ui->checkAutoStop->setChecked(settings->autoStop);

	ui->check360Video->setChecked(
		settings->projection ==
		LiveBroadcastProjection::THREE_HUNDRED_SIXTY);

	const char *thumbFile = settings->thumbnailFile.c_str();

	if (thumbFile && *thumbFile) {
		QFileInfo tFile(thumbFile);
		// Re-check validity before setting path again
		if (tFile.exists() && tFile.size() <= 2 * 1024 * 1024) {

			QFile thumbnailFile(thumbFile);
			if (!thumbnailFile.open(QFile::ReadOnly)) {
				return;
			}

			thumbnailData = thumbnailFile.readAll();
			thumbnailFilePath = tFile.absoluteFilePath();
			ui->selectedFileName->setText(thumbnailFilePath);
			ui->selectFileButton->setText(obs_module_text(
				"YouTube.Actions.Thumbnail.ClearFile"));

			QImageReader imgReader(thumbnailFilePath);
			imgReader.setAutoTransform(true);
			const QImage newImage = imgReader.read();
			ui->thumbnailPreview->setPixmap(
				QPixmap::fromImage(newImage).scaled(
					160, 90, Qt::KeepAspectRatio,
					Qt::SmoothTransformation));
		}
	}
}

void OBSYoutubeActions::OpenYouTubeDashboard()
{
	ChannelInfo channel;
	if (!apiYouTube->GetChannelInfo(channel)) {
		blog(LOG_DEBUG, "Could not get channel description.");
		RequestError lastError = apiYouTube->GetLastError();
		ShowErrorDialog(
			this,
			lastError.message.empty()
				? obs_module_text(
					  "YouTube.Actions.Error.General")
				: QT_UTF8(obs_module_text(
						  "YouTube.Actions.Error.Text"))
					  .arg(QString::fromStdString(
						  lastError.message)));
		return;
	}

	//https://studio.youtube.com/channel/UCA9bSfH3KL186kyiUsvi3IA/videos/live?filter=%5B%5D&sort=%7B%22columnType%22%3A%22date%22%2C%22sortOrder%22%3A%22DESCENDING%22%7D
	QString uri =
		QString("https://studio.youtube.com/channel/%1/videos/live?filter=[]&sort={\"columnType\"%3A\"date\"%2C\"sortOrder\"%3A\"DESCENDING\"}")
			.arg(QString::fromStdString(channel.id));
	QDesktopServices::openUrl(uri);
}

void OBSYoutubeActions::Cancel()
{
	workerThread->stop();
	reject();
}
void OBSYoutubeActions::Accept()
{
	workerThread->stop();
	accept();
}
