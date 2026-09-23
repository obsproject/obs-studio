#include "OBSXBroadcastActions.hpp"

#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>

#include "moc_OBSXBroadcastActions.cpp"

namespace {

const char *LiveStudioUrl = "https://x.com/i/live-studio";

void ShowError(QWidget *parent, const QString &text)
{
	QMessageBox box(parent);
	box.setWindowTitle(QTStr("X.Actions.Error.Title"));
	box.setText(text);
	box.setTextFormat(Qt::RichText);
	box.setIcon(QMessageBox::Warning);
	box.exec();
}

} // namespace

OBSXBroadcastActions::OBSXBroadcastActions(QWidget *parent, Auth *auth) : QDialog(parent)
{
	api = dynamic_cast<XApiWrappers *>(auth);
	if (!api) {
		return;
	}
	valid = true;
	setWindowTitle(QTStr("X.Actions.WindowTitle"));
	resize(480, 420);
	BuildUi();
	ReloadSource();
}

void OBSXBroadcastActions::BuildUi()
{
	auto *layout = new QVBoxLayout(this);

	auto *hint = new QLabel(QTStr("X.Actions.StreamKeyHint"), this);
	hint->setWordWrap(true);
	layout->addWidget(hint);

	sourceLabel = new QLabel(this);
	sourceLabel->setWordWrap(true);
	sourceLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	layout->addWidget(sourceLabel);

	titleEdit = new QLineEdit(this);
	titleEdit->setPlaceholderText(QTStr("X.Actions.Title"));
	layout->addWidget(titleEdit);

	lowLatency = new QCheckBox(QTStr("X.Actions.LowLatency"), this);
	lowLatency->setChecked(true);
	layout->addWidget(lowLatency);

	noTweet = new QCheckBox(QTStr("X.Actions.NoTweet"), this);
	layout->addWidget(noTweet);

	layout->addWidget(new QLabel(QTStr("X.Actions.ChatOption"), this));
	chatOption = new QComboBox(this);
	chatOption->addItem(QTStr("X.Actions.Chat.Everyone"), 2);
	chatOption->addItem(QTStr("X.Actions.Chat.Verified"), 3);
	chatOption->addItem(QTStr("X.Actions.Chat.Following"), 4);
	chatOption->addItem(QTStr("X.Actions.Chat.Subscribers"), 5);
	chatOption->addItem(QTStr("X.Actions.Chat.Disabled"), 1);
	layout->addWidget(chatOption);

	status = new QLabel(this);
	status->setWordWrap(true);
	layout->addWidget(status);

	auto *actions = new QHBoxLayout();
	auto *refresh = new QPushButton(QTStr("X.Actions.Refresh"), this);
	auto *studio = new QPushButton(QTStr("X.Actions.OpenLiveStudio"), this);
	goLiveButton = new QPushButton(QTStr("X.Actions.GoLive"), this);
	auto *cancel = new QPushButton(QTStr("Cancel"), this);
	actions->addWidget(refresh);
	actions->addWidget(studio);
	actions->addStretch(1);
	actions->addWidget(goLiveButton);
	actions->addWidget(cancel);
	layout->addLayout(actions);

	connect(refresh, &QPushButton::clicked, this, &OBSXBroadcastActions::ReloadSource);
	connect(studio, &QPushButton::clicked, this, []() { QDesktopServices::openUrl(QUrl(LiveStudioUrl)); });
	connect(goLiveButton, &QPushButton::clicked, this, &OBSXBroadcastActions::GoLive);
	connect(cancel, &QPushButton::clicked, this, &QDialog::reject);
}

void OBSXBroadcastActions::ReloadSource()
{
	bool ok = false;
	QString error;
	auto work = [&]() {
		ok = api->EnsureSource();
		if (!ok) {
			error = api->LastError();
		}
	};
	ExecThreadedWithoutBlocking(work, QTStr("Auth.LoadingChannel.Title"),
				    QTStr("Auth.LoadingChannel.Text").arg(QStringLiteral("X")));
	if (!ok) {
		sourceLabel->setText(QTStr("X.Actions.SourceMissing"));
		status->setText(error);
		goLiveButton->setEnabled(false);
		return;
	}

	goLiveButton->setEnabled(true);
	status->clear();
	sourceLabel->setText(
		QTStr("X.Actions.SourceReady").arg(api->Region(), api->IngestUrl(), QString::fromStdString(api->key())));
	api->ApplyIngestToService();
}

void OBSXBroadcastActions::GoLive()
{
	if (api->IngestUrl().isEmpty() || api->key().empty()) {
		ShowError(this, QTStr("X.Actions.Error.NeedSource"));
		return;
	}

	QString title = titleEdit->text().trimmed();
	if (title.isEmpty()) {
		title = QStringLiteral("OBS Studio");
	}
	api->SetPendingPublish(title, lowLatency->isChecked(), chatOption->currentData().toInt(), noTweet->isChecked());
	api->ApplyIngestToService();
	emit ready();
	accept();
}
