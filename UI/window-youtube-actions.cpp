#include "window-basic-main.hpp"
#include "moc_window-youtube-actions.cpp"

#include "obs-app.hpp"
#include "youtube-api-wrappers.hpp"

#include <qt-wrappers.hpp>
#include <QToolTip>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QStandardPaths>
#include <QImageReader>

const QString SchedulDateAndTimeFormat = "yyyy-MM-dd'T'hh:mm:ss'Z'";
const QString RepresentSchedulDateAndTimeFormat = "dddd, MMMM d, yyyy h:m";
const QString IndexOfGamingCategory = "20";

OBSYoutubeActions::OBSYoutubeActions(QWidget *parent, Auth *auth, bool broadcastReady)
	: QDialog(parent),
	  ui(new Ui::OBSYoutubeActions),
	  apiYouTube(dynamic_cast<YoutubeApiWrappers *>(auth)),
	  workerThread(new WorkerThread(apiYouTube)),
	  broadcastReady(broadcastReady)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
	ui->setupUi(this);

	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Public"), "public");
	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Unlisted"), "unlisted");
	ui->privacyBox->addItem(QTStr("YouTube.Actions.Privacy.Private"), "private");

	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.Normal"), "normal");
	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.Low"), "low");
	ui->latencyBox->addItem(QTStr("YouTube.Actions.Latency.UltraLow"), "ultraLow");

	UpdateOkButtonStatus();

	connect(ui->title, &QLineEdit::textChanged, this, [&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->privacyBox, &QComboBox::currentTextChanged, this,
		[&](const QString &) { this->UpdateOkButtonStatus(); });
	connect(ui->yesMakeForKids, &QRadioButton::toggled, this, [&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->notMakeForKids, &QRadioButton::toggled, this, [&](bool) { this->UpdateOkButtonStatus(); });
	connect(ui->tabWidget, &QTabWidget::currentChanged, this, [&](int) { this->UpdateOkButtonStatus(); });
	connect(ui->pushButton, &QPushButton::clicked, this, &OBSYoutubeActions::OpenYouTubeDashboard);

	connect(ui->helpAutoStartStop, &QLabel::linkActivated, this,
		[](const QString &) { QToolTip::showText(QCursor::pos(), QTStr("YouTube.Actions.AutoStartStop.TT")); });
	connect(ui->help360Video, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });
	connect(ui->helpMadeForKids, &QLabel::linkActivated, this,
		[](const QString &link) { QDesktopServices::openUrl(link); });

	ui->scheduledTime->setVisible(false);
#if QT_VERSION >= QT_VERSION_CHECK(6, 7, 0)
	connect(ui->checkScheduledLater, &QCheckBox::checkStateChanged, this,
		[&](Qt::CheckState state)
#else
	connect(ui->checkScheduledLater, &QCheckBox::stateChanged, this,
		[&](int state)
#endif
		{
			const bool checked = (state == Qt::Checked);
			ui->scheduledTime->setVisible(checked);
			if (checked) {
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
		if (thumbnailFile.isEmpty()) {
			QString filePath = OpenFile(this, QTStr("YouTube.Actions.Thumbnail.SelectFile"),
						    QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
						    QStringLiteral("Images (*.png *.jpg *.jpeg *.gif)"));

			if (!filePath.isEmpty()) {
				QFileInfo tFile(filePath);
				if (!tFile.exists()) {
					return ShowErrorDialog(this, QTStr("YouTube.Actions.Error.FileMissing"));
				} else if (tFile.size() > 2 * 1024 * 1024) {
					return ShowErrorDialog(this, QTStr("YouTube.Actions.Error.FileTooLarge"));
				}

				thumbnailFile = filePath;
				ui->selectedFileName->setText(thumbnailFile);
				ui->selectFileButton->setText(QTStr("YouTube.Actions.Thumbnail.ClearFile"));

				QImageReader imgReader(filePath);
				imgReader.setAutoTransform(true);
				const QImage newImage = imgReader.read();
				ui->thumbnailPreview->setPixmap(QPixmap::fromImage(newImage).scaled(
					160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
			}
		} else {
			thumbnailFile.clear();
			ui->selectedFileName->setText(QTStr("YouTube.Actions.Thumbnail.NoFileSelected"));
			ui->selectFileButton->setText(QTStr("YouTube.Actions.Thumbnail.SelectFile"));
			ui->thumbnailPreview->setPixmap(GetPlaceholder().pixmap(QSize(16, 16)));
		}
	};

	connect(ui->selectFileButton, &QPushButton::clicked, this, thumbSelectionHandler);
	connect(ui->thumbnailPreview, &ClickableLabel::clicked, this, thumbSelectionHandler);

	if (!apiYouTube) {
		blog(LOG_DEBUG, "YouTube API auth NOT found.");
		Cancel();
		return;
	}

	const char *name = config_get_string(OBSBasic::Get()->Config(), "YouTube", "ChannelName");
	this->setWindowTitle(QTStr("YouTube.Actions.WindowTitle").arg(name));

	QVector<CategoryDescription> category_list;
	if (!apiYouTube->GetVideoCategoriesList(category_list)) {
		ShowErrorDialog(parent, apiYouTube->GetLastError().isEmpty()
						? QTStr("YouTube.Actions.Error.General")
						: QTStr("YouTube.Actions.Error.Text").arg(apiYouTube->GetLastError()));
		Cancel();
		return;
	}
	for (auto &category : category_list) {
		ui->categoryBox->addItem(category.title, category.id);
		if (category.id == IndexOfGamingCategory) {
			ui->categoryBox->setCurrentText(category.title);
		}
	}

	connect(ui->okButton, &QPushButton::clicked, this, &OBSYoutubeActions::InitBroadcast);
	connect(ui->saveButton, &QPushButton::clicked, this, &OBSYoutubeActions::ReadyBroadcast);
	connect(ui->cancelButton, &QPushButton::clicked, this, [&]() {
		blog(LOG_DEBUG, "YouTube live broadcast creation cancelled.");
		// Close the dialog.
		Cancel();
	});

	qDeleteAll(ui->scrollAreaWidgetContents->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));

	// Add label indicating loading state
	QLabel *loadingLabel = new QLabel();
	loadingLabel->setTextFormat(Qt::RichText);
	loadingLabel->setAlignment(Qt::AlignHCenter);
	loadingLabel->setText(QStringLiteral("<big>%1</big>").arg(QTStr("YouTube.Actions.EventsLoading")));
	ui->scrollAreaWidgetContents->layout()->addWidget(loadingLabel);

	// Delete "loading..." label on completion
	connect(workerThread, &WorkerThread::finished, this, [&] {
		QLayoutItem *item = ui->scrollAreaWidgetContents->layout()->takeAt(0);
		item->widget()->deleteLater();
	});

	connect(workerThread, &WorkerThread::failed, this, [&]() {
		auto last_error = apiYouTube->GetLastError();
		if (last_error.isEmpty())
			last_error = QTStr("YouTube.Actions.Error.YouTubeApi");

		if (!apiYouTube->GetTranslatedError(last_error))
			last_error = QTStr("YouTube.Actions.Error.Text").arg(last_error);

		ShowErrorDialog(this, last_error);
		QDialog::reject();
	});

	connect(workerThread, &WorkerThread::new_item, this,
		[&](const QString &title, const QString &dateTimeString, const QString &broadcast,
		    const QString &status, bool astart, bool astop) {
			ClickableLabel *label = new ClickableLabel();
			label->setTextFormat(Qt::RichText);

			if (status == "live" || status == "testing") {
				// Resumable stream
				label->setText(QStringLiteral("<big>%1</big><br/>%2")
						       .arg(title, QTStr("YouTube.Actions.Stream.Resume")));

			} else if (dateTimeString.isEmpty()) {
				// The broadcast created by YouTube Studio has no start time.
				// Yes this does violate the restrictions set in YouTube's API
				// But why would YouTube care about consistency?
				label->setText(QStringLiteral("<big>%1</big><br/>%2")
						       .arg(title, QTStr("YouTube.Actions.Stream.YTStudio")));
			} else {
				label->setText(
					QStringLiteral("<big>%1</big><br/>%2")
						.arg(title,
						     QTStr("YouTube.Actions.Stream.ScheduledFor").arg(dateTimeString)));
			}

			label->setAlignment(Qt::AlignHCenter);
			label->setMargin(4);

			connect(label, &ClickableLabel::clicked, this, [&, label, broadcast, astart, astop]() {
				for (QWidget *i : ui->scrollAreaWidgetContents->findChildren<QWidget *>(
					     QString(), Qt::FindDirectChildrenOnly)) {

					i->setProperty("class", "");
					i->style()->unpolish(i);
					i->style()->polish(i);
				}
				label->setProperty("class", "row-selected");
				label->style()->unpolish(label);
				label->style()->polish(label);

				this->selectedBroadcast = broadcast;
				this->autostart = astart;
				this->autostop = astop;
				UpdateOkButtonStatus();
			});
			ui->scrollAreaWidgetContents->layout()->addWidget(label);

			if (selectedBroadcast == broadcast)
				label->clicked();
		});
	workerThread->start();

	OBSBasic *main = OBSBasic::Get();
	bool rememberSettings = config_get_bool(main->activeConfiguration, "YouTube", "RememberSettings");
	if (rememberSettings)
		LoadSettings();

	// Switch to events page and select readied broadcast once loaded
	if (broadcastReady) {
		ui->tabWidget->setCurrentIndex(1);
		selectedBroadcast = apiYouTube->GetBroadcastId();
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
	if (thumbnailFile.isEmpty())
		ui->thumbnailPreview->setPixmap(GetPlaceholder().pixmap(QSize(16, 16)));
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
	json11::Json broadcasts;

	for (QString broadcastStatus : {"active", "upcoming"}) {
		if (!apiYouTube->GetBroadcastsList(broadcasts, "", broadcastStatus)) {
			emit failed();
			return;
		}

		while (pending) {
			auto items = broadcasts["items"].array_items();
			for (auto item : items) {
				QString status =
					QString::fromStdString(item["status"]["lifeCycleStatus"].string_value());

				if (status == "live" || status == "testing") {
					// Check that the attached liveStream is offline (reconnectable)
					QString stream_id = QString::fromStdString(
						item["contentDetails"]["boundStreamId"].string_value());
					json11::Json stream;
					if (!apiYouTube->FindStream(stream_id, stream))
						continue;
					if (stream["status"]["streamStatus"] == "active")
						continue;
				}

				QString title = QString::fromStdString(item["snippet"]["title"].string_value());
				QString scheduledStartTime =
					QString::fromStdString(item["snippet"]["scheduledStartTime"].string_value());
				QString broadcast = QString::fromStdString(item["id"].string_value());

				// Treat already started streams as autostart for UI purposes
				bool astart = status == "live" ||
					      item["contentDetails"]["enableAutoStart"].bool_value();
				bool astop = item["contentDetails"]["enableAutoStop"].bool_value();

				QDateTime utcDTime =
					QDateTime::fromString(scheduledStartTime, SchedulDateAndTimeFormat);
				// DateTime parser means that input datetime is a local, so we need to move it
				QDateTime dateTime = utcDTime.addSecs(utcDTime.offsetFromUtc());

				QString dateTimeString = QLocale().toString(
					dateTime,
					QStringLiteral("%1  %2").arg(QLocale().dateFormat(QLocale::LongFormat),
								     QLocale().timeFormat(QLocale::ShortFormat)));

				emit new_item(title, dateTimeString, broadcast, status, astart, astop);
			}

			auto nextPageToken = broadcasts["nextPageToken"].string_value();
			if (nextPageToken.empty() || items.empty())
				break;
			else {
				if (!pending)
					return;
				if (!apiYouTube->GetBroadcastsList(broadcasts, QString::fromStdString(nextPageToken),
								   broadcastStatus)) {
					emit failed();
					return;
				}
			}
		}
	}

	emit ready();
}

void OBSYoutubeActions::UpdateOkButtonStatus()
{
	bool enable = false;

	if (ui->tabWidget->currentIndex() == 0) {
		enable = !ui->title->text().isEmpty() && !ui->privacyBox->currentText().isEmpty() &&
			 (ui->yesMakeForKids->isChecked() || ui->notMakeForKids->isChecked());
		ui->okButton->setEnabled(enable);
		ui->saveButton->setEnabled(enable);

		if (ui->checkScheduledLater->checkState() == Qt::Checked) {
			ui->okButton->setText(QTStr("YouTube.Actions.Create_Schedule"));
			ui->saveButton->setText(QTStr("YouTube.Actions.Create_Schedule_Ready"));
		} else {
			ui->okButton->setText(QTStr("YouTube.Actions.Create_GoLive"));
			ui->saveButton->setText(QTStr("YouTube.Actions.Create_Ready"));
		}
		ui->pushButton->setVisible(false);
	} else {
		enable = !selectedBroadcast.isEmpty();
		ui->okButton->setEnabled(enable);
		ui->saveButton->setEnabled(enable);
		ui->okButton->setText(QTStr("YouTube.Actions.Choose_GoLive"));
		ui->saveButton->setText(QTStr("YouTube.Actions.Choose_Ready"));

		ui->pushButton->setVisible(true);
	}
}
bool OBSYoutubeActions::CreateEventAction(YoutubeApiWrappers *api, BroadcastDescription &broadcast,
					  StreamDescription &stream, bool stream_later, bool ready_broadcast)
{
	YoutubeApiWrappers *apiYouTube = api;
	UiToBroadcast(broadcast);

	if (stream_later) {
		// DateTime parser means that input datetime is a local, so we need to move it
		auto dateTime = ui->scheduledTime->dateTime();
		auto utcDTime = dateTime.addSecs(-dateTime.offsetFromUtc());
		broadcast.schedul_date_time = utcDTime.toString(SchedulDateAndTimeFormat);
	} else {
		// stream now is always autostart/autostop
		broadcast.auto_start = true;
		broadcast.auto_stop = true;
		broadcast.schedul_date_time = QDateTime::currentDateTimeUtc().toString(SchedulDateAndTimeFormat);
	}

	autostart = broadcast.auto_start;
	autostop = broadcast.auto_stop;

	blog(LOG_DEBUG, "Scheduled date and time: %s", broadcast.schedul_date_time.toStdString().c_str());
	if (!apiYouTube->InsertBroadcast(broadcast)) {
		blog(LOG_DEBUG, "No broadcast created.");
		return false;
	}
	if (!apiYouTube->SetVideoCategory(broadcast.id, broadcast.title, broadcast.description,
					  broadcast.category.id)) {
		blog(LOG_DEBUG, "No category set.");
		return false;
	}
	if (!thumbnailFile.isEmpty()) {
		blog(LOG_INFO, "Uploading thumbnail file \"%s\"...", thumbnailFile.toStdString().c_str());
		if (!apiYouTube->SetVideoThumbnail(broadcast.id, thumbnailFile)) {
			blog(LOG_DEBUG, "No thumbnail set.");
			return false;
		}
	}

	if (!stream_later || ready_broadcast) {
		stream = {"", "", "OBS Studio Video Stream"};
		if (!apiYouTube->InsertStream(stream)) {
			blog(LOG_DEBUG, "No stream created.");
			return false;
		}
		if (!apiYouTube->BindStream(broadcast.id, stream.id)) {
			blog(LOG_DEBUG, "No stream binded.");
			return false;
		}

		if (broadcast.privacy != "private") {
			apiYouTube->SetChatId(broadcast.id);
		} else {
			apiYouTube->ResetChat();
		}
	}

#ifdef YOUTUBE_ENABLED
	if (OBSBasic::Get()->GetYouTubeAppDock())
		OBSBasic::Get()->GetYouTubeAppDock()->BroadcastCreated(broadcast.id.toStdString().c_str());
#endif

	return true;
}

bool OBSYoutubeActions::ChooseAnEventAction(YoutubeApiWrappers *api, StreamDescription &stream)
{
	YoutubeApiWrappers *apiYouTube = api;

	json11::Json json;
	if (!apiYouTube->FindBroadcast(selectedBroadcast, json)) {
		blog(LOG_DEBUG, "No broadcast found.");
		return false;
	}

	std::string boundStreamId = json["items"].array_items()[0]["contentDetails"]["boundStreamId"].string_value();
	std::string broadcastPrivacy = json["items"].array_items()[0]["status"]["privacyStatus"].string_value();

	stream.id = boundStreamId.c_str();
	if (!stream.id.isEmpty() && apiYouTube->FindStream(stream.id, json)) {
		auto item = json["items"].array_items()[0];
		auto streamName = item["cdn"]["ingestionInfo"]["streamName"].string_value();
		auto title = item["snippet"]["title"].string_value();

		stream.name = streamName.c_str();
		stream.title = title.c_str();
		api->SetBroadcastId(selectedBroadcast);
	} else {
		stream = {"", "", "OBS Studio Video Stream"};
		if (!apiYouTube->InsertStream(stream)) {
			blog(LOG_DEBUG, "No stream created.");
			return false;
		}
		if (!apiYouTube->BindStream(selectedBroadcast, stream.id)) {
			blog(LOG_DEBUG, "No stream binded.");
			return false;
		}
	}

	if (broadcastPrivacy != "private")
		apiYouTube->SetChatId(selectedBroadcast);
	else
		apiYouTube->ResetChat();

#ifdef YOUTUBE_ENABLED
	if (OBSBasic::Get()->GetYouTubeAppDock())
		OBSBasic::Get()->GetYouTubeAppDock()->BroadcastSelected(selectedBroadcast.toStdString().c_str());
#endif

	return true;
}

void OBSYoutubeActions::ShowErrorDialog(QWidget *parent, QString text)
{
	QMessageBox dlg(parent);
	dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowCloseButtonHint);
	dlg.setWindowTitle(QTStr("YouTube.Actions.Error.Title"));
	dlg.setText(text);
	dlg.setTextFormat(Qt::RichText);
	dlg.setIcon(QMessageBox::Warning);
	dlg.setStandardButtons(QMessageBox::StandardButton::Ok);
	dlg.exec();
}

void OBSYoutubeActions::InitBroadcast()
{
	BroadcastDescription broadcast;
	StreamDescription stream;
	QMessageBox msgBox(this);
	msgBox.setWindowFlags(msgBox.windowFlags() & ~Qt::WindowCloseButtonHint);
	msgBox.setWindowTitle(QTStr("YouTube.Actions.Notify.Title"));
	msgBox.setText(QTStr("YouTube.Actions.Notify.CreatingBroadcast"));
	msgBox.setStandardButtons(QMessageBox::StandardButtons());

	bool success = false;
	auto action = [&]() {
		if (ui->tabWidget->currentIndex() == 0) {
			success = this->CreateEventAction(apiYouTube, broadcast, stream,
							  ui->checkScheduledLater->isChecked());
		} else {
			success = this->ChooseAnEventAction(apiYouTube, stream);
			if (success)
				broadcast.id = this->selectedBroadcast;
		};
		QMetaObject::invokeMethod(&msgBox, "accept", Qt::QueuedConnection);
	};
	QScopedPointer<QThread> thread(CreateQThread(action));
	thread->start();
	msgBox.exec();
	thread->wait();

	if (success) {
		if (ui->tabWidget->currentIndex() == 0) {
			// Stream later usecase.
			if (ui->checkScheduledLater->isChecked()) {
				QMessageBox msg(this);
				msg.setWindowTitle(QTStr("YouTube.Actions.EventCreated.Title"));
				msg.setText(QTStr("YouTube.Actions.EventCreated.Text"));
				msg.setStandardButtons(QMessageBox::Ok);
				msg.exec();
				// Close dialog without start streaming.
				Cancel();
			} else {
				// Stream now usecase.
				blog(LOG_DEBUG, "New valid stream: %s", QT_TO_UTF8(stream.name));
				emit ok(QT_TO_UTF8(broadcast.id), QT_TO_UTF8(stream.id), QT_TO_UTF8(stream.name), true,
					true, true);
				Accept();
			}
		} else {
			// Stream to precreated broadcast usecase.
			emit ok(QT_TO_UTF8(broadcast.id), QT_TO_UTF8(stream.id), QT_TO_UTF8(stream.name), autostart,
				autostop, true);
			Accept();
		}
	} else {
		// Fail.
		auto last_error = apiYouTube->GetLastError();
		if (last_error.isEmpty())
			last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
		if (!apiYouTube->GetTranslatedError(last_error))
			last_error = QTStr("YouTube.Actions.Error.NoBroadcastCreated").arg(last_error);

		ShowErrorDialog(this, last_error);
	}
}

void OBSYoutubeActions::ReadyBroadcast()
{
	BroadcastDescription broadcast;
	StreamDescription stream;
	QMessageBox msgBox(this);
	msgBox.setWindowFlags(msgBox.windowFlags() & ~Qt::WindowCloseButtonHint);
	msgBox.setWindowTitle(QTStr("YouTube.Actions.Notify.Title"));
	msgBox.setText(QTStr("YouTube.Actions.Notify.CreatingBroadcast"));
	msgBox.setStandardButtons(QMessageBox::StandardButtons());

	bool success = false;
	auto action = [&]() {
		if (ui->tabWidget->currentIndex() == 0) {
			success = this->CreateEventAction(apiYouTube, broadcast, stream,
							  ui->checkScheduledLater->isChecked(), true);
		} else {
			success = this->ChooseAnEventAction(apiYouTube, stream);
			if (success)
				broadcast.id = this->selectedBroadcast;
		};
		QMetaObject::invokeMethod(&msgBox, "accept", Qt::QueuedConnection);
	};
	QScopedPointer<QThread> thread(CreateQThread(action));
	thread->start();
	msgBox.exec();
	thread->wait();

	if (success) {
		emit ok(QT_TO_UTF8(broadcast.id), QT_TO_UTF8(stream.id), QT_TO_UTF8(stream.name), autostart, autostop,
			false);
		Accept();
	} else {
		// Fail.
		auto last_error = apiYouTube->GetLastError();
		if (last_error.isEmpty())
			last_error = QTStr("YouTube.Actions.Error.YouTubeApi");
		if (!apiYouTube->GetTranslatedError(last_error))
			last_error = QTStr("YouTube.Actions.Error.NoBroadcastCreated").arg(last_error);

		ShowErrorDialog(this, last_error);
	}
}

void OBSYoutubeActions::UiToBroadcast(BroadcastDescription &broadcast)
{
	broadcast.title = ui->title->text();
	// ToDo: UI warning rather than silent truncation
	broadcast.description = ui->description->toPlainText().left(5000);
	broadcast.privacy = ui->privacyBox->currentData().toString();
	broadcast.category.title = ui->categoryBox->currentText();
	broadcast.category.id = ui->categoryBox->currentData().toString();
	broadcast.made_for_kids = ui->yesMakeForKids->isChecked();
	broadcast.latency = ui->latencyBox->currentData().toString();
	broadcast.auto_start = ui->checkAutoStart->isChecked();
	broadcast.auto_stop = ui->checkAutoStop->isChecked();
	broadcast.dvr = ui->checkDVR->isChecked();
	broadcast.schedul_for_later = ui->checkScheduledLater->isChecked();
	broadcast.projection = ui->check360Video->isChecked() ? "360" : "rectangular";

	if (ui->checkRememberSettings->isChecked())
		SaveSettings(broadcast);
}

void OBSYoutubeActions::SaveSettings(BroadcastDescription &broadcast)
{
	OBSBasic *main = OBSBasic::Get();

	config_set_string(main->activeConfiguration, "YouTube", "Title", QT_TO_UTF8(broadcast.title));
	config_set_string(main->activeConfiguration, "YouTube", "Description", QT_TO_UTF8(broadcast.description));
	config_set_string(main->activeConfiguration, "YouTube", "Privacy", QT_TO_UTF8(broadcast.privacy));
	config_set_string(main->activeConfiguration, "YouTube", "CategoryID", QT_TO_UTF8(broadcast.category.id));
	config_set_string(main->activeConfiguration, "YouTube", "Latency", QT_TO_UTF8(broadcast.latency));
	config_set_bool(main->activeConfiguration, "YouTube", "MadeForKids", broadcast.made_for_kids);
	config_set_bool(main->activeConfiguration, "YouTube", "AutoStart", broadcast.auto_start);
	config_set_bool(main->activeConfiguration, "YouTube", "AutoStop", broadcast.auto_start);
	config_set_bool(main->activeConfiguration, "YouTube", "DVR", broadcast.dvr);
	config_set_bool(main->activeConfiguration, "YouTube", "ScheduleForLater", broadcast.schedul_for_later);
	config_set_string(main->activeConfiguration, "YouTube", "Projection", QT_TO_UTF8(broadcast.projection));
	config_set_string(main->activeConfiguration, "YouTube", "ThumbnailFile", QT_TO_UTF8(thumbnailFile));
	config_set_bool(main->activeConfiguration, "YouTube", "RememberSettings", true);
}

void OBSYoutubeActions::LoadSettings()
{
	OBSBasic *main = OBSBasic::Get();

	const char *title = config_get_string(main->activeConfiguration, "YouTube", "Title");
	ui->title->setText(QT_UTF8(title));

	const char *desc = config_get_string(main->activeConfiguration, "YouTube", "Description");
	ui->description->setPlainText(QT_UTF8(desc));

	const char *priv = config_get_string(main->activeConfiguration, "YouTube", "Privacy");
	int index = ui->privacyBox->findData(priv);
	ui->privacyBox->setCurrentIndex(index);

	const char *catID = config_get_string(main->activeConfiguration, "YouTube", "CategoryID");
	index = ui->categoryBox->findData(catID);
	ui->categoryBox->setCurrentIndex(index);

	const char *latency = config_get_string(main->activeConfiguration, "YouTube", "Latency");
	index = ui->latencyBox->findData(latency);
	ui->latencyBox->setCurrentIndex(index);

	bool dvr = config_get_bool(main->activeConfiguration, "YouTube", "DVR");
	ui->checkDVR->setChecked(dvr);

	bool forKids = config_get_bool(main->activeConfiguration, "YouTube", "MadeForKids");
	if (forKids)
		ui->yesMakeForKids->setChecked(true);
	else
		ui->notMakeForKids->setChecked(true);

	bool schedLater = config_get_bool(main->activeConfiguration, "YouTube", "ScheduleForLater");
	ui->checkScheduledLater->setChecked(schedLater);

	bool autoStart = config_get_bool(main->activeConfiguration, "YouTube", "AutoStart");
	ui->checkAutoStart->setChecked(autoStart);

	bool autoStop = config_get_bool(main->activeConfiguration, "YouTube", "AutoStop");
	ui->checkAutoStop->setChecked(autoStop);

	const char *projection = config_get_string(main->activeConfiguration, "YouTube", "Projection");
	if (projection && *projection) {
		if (strcmp(projection, "360") == 0)
			ui->check360Video->setChecked(true);
		else
			ui->check360Video->setChecked(false);
	}

	const char *thumbFile = config_get_string(main->activeConfiguration, "YouTube", "ThumbnailFile");
	if (thumbFile && *thumbFile) {
		QFileInfo tFile(thumbFile);
		// Re-check validity before setting path again
		if (tFile.exists() && tFile.size() <= 2 * 1024 * 1024) {
			thumbnailFile = tFile.absoluteFilePath();
			ui->selectedFileName->setText(thumbnailFile);
			ui->selectFileButton->setText(QTStr("YouTube.Actions.Thumbnail.ClearFile"));

			QImageReader imgReader(thumbnailFile);
			imgReader.setAutoTransform(true);
			const QImage newImage = imgReader.read();
			ui->thumbnailPreview->setPixmap(QPixmap::fromImage(newImage).scaled(
				160, 90, Qt::KeepAspectRatio, Qt::SmoothTransformation));
		}
	}
}

void OBSYoutubeActions::OpenYouTubeDashboard()
{
	ChannelDescription channel;
	if (!apiYouTube->GetChannelDescription(channel)) {
		blog(LOG_DEBUG, "Could not get channel description.");
		ShowErrorDialog(this, apiYouTube->GetLastError().isEmpty()
					      ? QTStr("YouTube.Actions.Error.General")
					      : QTStr("YouTube.Actions.Error.Text").arg(apiYouTube->GetLastError()));
		return;
	}

	//https://studio.youtube.com/channel/UCA9bSfH3KL186kyiUsvi3IA/videos/live?filter=%5B%5D&sort=%7B%22columnType%22%3A%22date%22%2C%22sortOrder%22%3A%22DESCENDING%22%7D
	QString uri =
		QStringLiteral(
			"https://studio.youtube.com/channel/%1/videos/live?filter=[]&sort={\"columnType\"%3A\"date\"%2C\"sortOrder\"%3A\"DESCENDING\"}")
			.arg(channel.id);
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
