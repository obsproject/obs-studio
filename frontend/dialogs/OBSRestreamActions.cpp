#include "OBSRestreamActions.hpp"

#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>

#include "moc_OBSRestreamActions.cpp"

OBSRestreamActions::OBSRestreamActions(QWidget *parent, Auth *auth, bool broadcastReady)
	: QDialog(parent),
	  ui(new Ui::OBSRestreamActions),
	  restreamAuth(dynamic_cast<RestreamAuth *>(auth)),
	  broadcastReady(broadcastReady)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	// Reload the list when the user comes back from the Restream website
	connect(App(), &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
		if (state == Qt::ApplicationActive) {
			auto events = this->restreamAuth->GetBroadcastInfo();
			this->UpdateBroadcastList(events);
		}
	});

	ui->setupUi(this);
	ui->okButton->setEnabled(false);
	ui->saveButton->setEnabled(false);

	connect(ui->okButton, &QPushButton::clicked, this, &OBSRestreamActions::BroadcastSelectAndStartAction);
	connect(ui->saveButton, &QPushButton::clicked, this, &OBSRestreamActions::BroadcastSelectAction);
	connect(ui->dashboardButton, &QPushButton::clicked, this, &OBSRestreamActions::OpenRestreamDashboard);
	connect(ui->cancelButton, &QPushButton::clicked, this, [&]() {
		blog(LOG_DEBUG, "Restream live event creation cancelled.");
		reject();
	});

	auto events = restreamAuth->GetBroadcastInfo();
	this->UpdateBroadcastList(events);
}

OBSRestreamActions::~OBSRestreamActions() {}

void OBSRestreamActions::UpdateBroadcastList(QVector<RestreamEventDescription> &events)
{
	if (events.isEmpty()) {
		RestreamEventDescription event;
		event.id = "";
		event.title = "Live with Restream";
		event.scheduledFor = 0;
		event.showId = "";
		event.isInstant = true;
		events.push_back(event);
	}

	auto tryToFindShowId = selectedShowId;
	if (tryToFindShowId.empty()) {
		tryToFindShowId = restreamAuth->GetShowId();
	}

	selectedEventId = "";
	selectedShowId = "";

	qDeleteAll(ui->scrollAreaWidgetContents->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));
	EnableOkButton(false);

	for (auto event : events) {
		if (event.showId == tryToFindShowId) {
			selectedEventId = event.id;
			selectedShowId = event.showId;

			EnableOkButton(true);
			break;
		}
	}

	if (selectedShowId.empty()) {
		if (events.size()) {
			auto event = events.at(0);
			for (const auto &candidate : events) {
				if (candidate.isInstant) {
					event = candidate;
					break;
				}
			}
			selectedEventId = event.id;
			selectedShowId = event.showId;

			restreamAuth->SelectShow(selectedEventId, selectedShowId);

			EnableOkButton(true);
			emit ok(false);
		} else {
			restreamAuth->ResetShow();
			emit ok(false);
		}
	}

	for (auto event : events) {
		ClickableLabel *label = new ClickableLabel();
		label->setTextFormat(Qt::RichText);
		label->setMargin(4);

		QString details;
		if (event.scheduledFor > 0) {
			QDateTime dateTime = QDateTime::fromSecsSinceEpoch(event.scheduledFor);
			QString scheduledForString = QLocale().toString(
				dateTime, QString("%1 %2").arg(QLocale().dateFormat(QLocale::LongFormat),
							       QLocale().timeFormat(QLocale::ShortFormat)));
			details =
				QString("%1: %2").arg(QTStr("Restream.Actions.BroadcastScheduled"), scheduledForString);
		} else if (event.isInstant) {
			details = QTStr("Restream.Actions.BroadcastDefault");
		} else {
			details = QTStr("Restream.Actions.BroadcastNotScheduled");
		}

		label->setText(QString("<big>%1</big><br>%2").arg(QString::fromStdString(event.title), details));

		connect(label, &ClickableLabel::clicked, this, [&, label, event]() {
			for (QWidget *i : ui->scrollAreaWidgetContents->findChildren<QWidget *>(
				     QString(), Qt::FindDirectChildrenOnly)) {

				i->setProperty("class", "");
				i->style()->unpolish(i);
				i->style()->polish(i);
			}
			label->setProperty("class", "row-selected");
			label->style()->unpolish(label);
			label->style()->polish(label);

			selectedEventId = event.id;
			selectedShowId = event.showId;

			EnableOkButton(true);
		});

		ui->scrollAreaWidgetContents->layout()->addWidget(label);

		if (event.showId == selectedShowId) {
			label->setProperty("class", "row-selected");
			label->style()->unpolish(label);
			label->style()->polish(label);
		}
	}
}

void OBSRestreamActions::EnableOkButton(bool state)
{
	ui->okButton->setEnabled(state);
	ui->saveButton->setEnabled(state);
}

void OBSRestreamActions::BroadcastSelectAction()
{
	restreamAuth->SelectShow(selectedEventId, selectedShowId);
	emit ok(false);
	accept();
}

void OBSRestreamActions::BroadcastSelectAndStartAction()
{
	restreamAuth->SelectShow(selectedEventId, selectedShowId);
	emit ok(true);
	accept();
}

void OBSRestreamActions::OpenRestreamDashboard()
{
	QDesktopServices::openUrl(QUrl("https://app.restream.io/"));
}
