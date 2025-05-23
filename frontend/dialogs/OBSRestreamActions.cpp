#include "OBSRestreamActions.hpp"

#include <widgets/OBSBasic.hpp>
#include <utility/OBSEventFilter.hpp>

#include <qt-wrappers.hpp>
#include <QToolTip>
#include <QDateTime>
#include <QDesktopServices>
#include <QFileInfo>
#include <QStandardPaths>
#include <QImageReader>

#include "moc_OBSRestreamActions.cpp"

OBSRestreamActions::OBSRestreamActions(QWidget *parent, Auth *auth, bool broadcastReady)
	: QDialog(parent),
	  ui(new Ui::OBSRestreamActions),
	  restreamAuth(dynamic_cast<RestreamAuth *>(auth)),
	  broadcastReady(broadcastReady)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	eventFilter = new OBSEventFilter([this](QObject *, QEvent *event) {
		if (event->type() == QEvent::ApplicationActivate) {
			auto events = this->restreamAuth->GetBroadcastInfo();
			this->UpdateBroadcastList(events);
		}
		return false;
	});

	App()->installEventFilter(eventFilter);

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

	qDeleteAll(ui->scrollAreaWidgetContents->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly));

	auto events = restreamAuth->GetBroadcastInfo();
	this->UpdateBroadcastList(events);
}

OBSRestreamActions::~OBSRestreamActions()
{
	if (eventFilter)
		App()->removeEventFilter(eventFilter);
}

void OBSRestreamActions::UpdateBroadcastList(QVector<RestreamEventDescription> &events)
{
	if (events.isEmpty()) {
		RestreamEventDescription event;
		event.id = "";
		event.title = "Live with Restream";
		event.scheduledFor = 0;
		event.showId = "";
		events.push_back(event);
	}

	auto tryToFindShowId = selectedShowId;
	if (tryToFindShowId.empty())
		tryToFindShowId = restreamAuth->GetShowId();

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
		label->setAlignment(Qt::AlignHCenter);
		label->setMargin(4);

		QString scheduledForString;
		if (event.scheduledFor > 0) {
			QDateTime dateTime = QDateTime::fromSecsSinceEpoch(event.scheduledFor);
			scheduledForString = QLocale().toString(
				dateTime, QString("%1 %2").arg(QLocale().dateFormat(QLocale::LongFormat),
							       QLocale().timeFormat(QLocale::ShortFormat)));

			label->setText(QString("<big>%1</big><br>%2: %3")
					       .arg(QString::fromStdString(event.title),
						    QTStr("Restream.Actions.BroadcastScheduled"), scheduledForString));
		} else {
			label->setText(
				QString("<big>%1</big>%2").arg(QString::fromStdString(event.title), scheduledForString));
		}

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
	QDesktopServices::openUrl(QString("https://app.restream.io/"));
}
