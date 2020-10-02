#include "page-start-prompt.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QLabel>
#include <QSpacerItem>
#include <QPushButton>
#include <QDesktopServices>
#include <QUrl>

#include "obs-app.hpp"

namespace StreamWizard {

StartPage::StartPage(Destination dest, LaunchContext launchContext,
		     QSize videoSize, QWidget *parent)
	: QWizardPage(parent)
{
	this->destination_ = dest;
	this->launchContext_ = launchContext;
	this->startVideoSize_ = videoSize;

	setTitle(QTStr("PreLiveWizard.Prompt.Title"));

	createLayout();
	connectRadioButtons();

	//Default behavior per destination
	if (destination_ == Destination::Facebook) {
		res720Button_->setChecked(true);
	} else {
		resCurrentButton_->setChecked(true);
	}
}
void StartPage::createLayout()
{
	QVBoxLayout *mainlayout = new QVBoxLayout(this);

	QLabel *explainer =
		new QLabel(QTStr("PreLiveWizard.Prompt.Explainer"), this);
	explainer->setWordWrap(true);

	QLabel *radioButtonsTitle =
		new QLabel(QTStr("PreLiveWizard.Prompt.ResSelectTitle"), this);
	radioButtonsTitle->setWordWrap(true);
	radioButtonsTitle->setStyleSheet("font-weight: bold;");

	// Radio Button Group
	QString res720Label = QTStr("PreLiveWizard.Prompt.Resolution.720");
	res720Button_ = new QRadioButton(res720Label, this);

	QString res1080label = QTStr("PreLiveWizard.Prompt.Resolution.1080");
	res1080Button_ = new QRadioButton(res1080label, this);

	QString currentResLabel =
		QTStr("PreLiveWizard.Prompt.Resolution.Current")
			.arg(QString::number(startVideoSize_.width()),
			     QString::number(startVideoSize_.height()));
	resCurrentButton_ = new QRadioButton(currentResLabel, this);

	// Optional Help Section
	QLabel *helpLabel = nullptr;
	QPushButton *helpButton = nullptr;

	if (destination_ == Destination::Facebook) {
		// If FB, specfic help section
		helpLabel = new QLabel(
			QTStr("PreLiveWizard.Prompt.FBResolutionHelp.FB"),
			this);
		helpLabel->setWordWrap(true);
		helpButton = new QPushButton(
			QTStr("PreLiveWizard.Prompt.FBResolutionHelpButton.FB"),
			this);
		helpButton->setToolTip(QTStr(
			"PreLiveWizard.Prompt.FBResolutionHelpButton.FB.ToolTip"));
		connect(helpButton, &QPushButton::clicked, this,
			&StartPage::didPushOpenWebsiteHelp);
	}

	setLayout(mainlayout);
	mainlayout->addWidget(explainer);
	mainlayout->addSpacerItem(new QSpacerItem(12, 12));
	mainlayout->addWidget(radioButtonsTitle);
	mainlayout->addWidget(res720Button_);
	mainlayout->addWidget(res1080Button_);
	mainlayout->addWidget(resCurrentButton_);

	if (helpLabel != nullptr) {
		mainlayout->addSpacerItem(new QSpacerItem(24, 24));
		mainlayout->addWidget(helpLabel);
	}
	if (helpButton != nullptr) {
		QHBoxLayout *buttonAndSpacerLayout = new QHBoxLayout();
		buttonAndSpacerLayout->addWidget(helpButton);
		QSpacerItem *buttonSpacer =
			new QSpacerItem(24, 24, QSizePolicy::MinimumExpanding,
					QSizePolicy::Minimum);
		buttonAndSpacerLayout->addSpacerItem(buttonSpacer);
		mainlayout->addLayout(buttonAndSpacerLayout);
	}
}

void StartPage::connectRadioButtons()
{
	connect(res720Button_, &QRadioButton::clicked, [=]() {
		selectedVideoSize_ = QSize(1280, 720);
		emit userSelectedResolution(selectedVideoSize_);
	});
	connect(res1080Button_, &QRadioButton::clicked, [=]() {
		selectedVideoSize_ = QSize(1920, 1080);
		emit userSelectedResolution(selectedVideoSize_);
	});
	connect(resCurrentButton_, &QRadioButton::clicked, [=]() {
		selectedVideoSize_ = QSize(startVideoSize_.width(),
					   startVideoSize_.height());
		emit userSelectedResolution(selectedVideoSize_);
	});
}

void StartPage::didPushOpenWebsiteHelp()
{
	QUrl helpUrl;

	// Prepare per-destination
	if (destination_ == Destination::Facebook) {
		helpUrl = QUrl(
			"https://www.facebook.com/live/producer/?ref=OBS_Wizard",
			QUrl::TolerantMode);
	} else {
		return;
	}

	// Launch
	if (helpUrl.isValid()) {
		QDesktopServices::openUrl(helpUrl);
	}
}

} // namespace StreamWizard
