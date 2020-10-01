#include "page-completed.hpp"

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QUrl>
#include <QSpacerItem>
#include <QDesktopServices>

#include "obs-app.hpp"

namespace StreamWizard {

CompletedPage::CompletedPage(Destination destination,
			     LaunchContext launchContext, QWidget *parent)
	: QWizardPage(parent)
{
	destination_ = destination;
	launchContext_ = launchContext;

	setTitle(QTStr("PreLiveWizard.Completed.Title"));
	setFinalPage(true);

	QVBoxLayout *mainlayout = new QVBoxLayout(this);
	setLayout(mainlayout);

	// Later will suggest starting stream if launchContext is PreStream
	QLabel *closeLabel =
		new QLabel(QTStr("PreLiveWizard.Completed.BodyText"), this);
	closeLabel->setWordWrap(true);
	mainlayout->addWidget(closeLabel);

	if (destination_ == Destination::Facebook) {
		mainlayout->addSpacerItem(new QSpacerItem(12, 12));

		QLabel *facebookGoLiveLabel = new QLabel(
			QTStr("PreLiveWizard.Completed.FacebookOnly"), this);
		facebookGoLiveLabel->setWordWrap(true);
		QPushButton *launchButton = new QPushButton(
			QTStr("PreLiveWizard.Prompt.FBResolutionHelpButton.FB"),
			this);
		launchButton->setToolTip(QTStr(
			"PreLiveWizard.Prompt.FBResolutionHelpButton.FB.ToolTip"));
		connect(launchButton, &QPushButton::clicked, this,
			&CompletedPage::didPushOpenWebsite);

		mainlayout->addWidget(facebookGoLiveLabel);
		mainlayout->addWidget(launchButton);
	}
}

void CompletedPage::didPushOpenWebsite()
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
