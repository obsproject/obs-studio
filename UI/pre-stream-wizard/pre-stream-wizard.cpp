#include "pre-stream-wizard.hpp"

#include <QWizardPage>
#include <QFormLayout>
#include <QLabel>
#include <QSize>

#include "obs-app.hpp"
#include "encoder-settings-provider-facebook.hpp"

#include "page-input-display.hpp"
#include "page-start-prompt.hpp"

namespace StreamWizard {

enum PSW_Page { Page_StartPrompt, Page_Loading, Page_Selections };

PreStreamWizard::PreStreamWizard(
	Destination dest, LaunchContext launchContext,
	QSharedPointer<EncoderSettingsRequest> currentSettings, QWidget *parent)
	: QWizard(parent)
{
	this->currentSettings_ = currentSettings;
	this->destination_ = dest;
	this->launchContext_ = launchContext;
	connect(this, &QWizard::currentIdChanged, this,
		&PreStreamWizard::onPageChanged);

	setWizardStyle(QWizard::ModernStyle);
	setWindowTitle(QTStr("PreLiveWizard.Title"));

	// First Page: Explain to the user and confirm sending to API
	QSize currentRes(currentSettings_->videoWidth,
			 currentSettings_->videoHeight);
	StartPage *startPage =
		new StartPage(destination_, launchContext_, currentRes, this);
	setPage(Page_StartPrompt, startPage);
	connect(startPage, &StartPage::userSelectedResolution, this,
		&PreStreamWizard::onUserSelectResolution);

	// Loading page: Shown when loading new settings
	setPage(Page_Loading, new QWizardPage());

	// Suggestion Selection Pahe
	setPage(Page_Selections, new QWizardPage());
}

void PreStreamWizard::requestSettings()
{
	if (destination_ == Destination::Facebook) {

		FacebookEncoderSettingsProvider *fbProvider =
			new FacebookEncoderSettingsProvider(this);
		connect(fbProvider,
			&FacebookEncoderSettingsProvider::newSettings, this,
			&PreStreamWizard::providerEncoderSettings);
		connect(fbProvider,
			&FacebookEncoderSettingsProvider::returnErrorDescription,
			this, &PreStreamWizard::providerError);
		fbProvider->setEncoderRequest(currentSettings_);
		fbProvider->run();

	} else {
		QString errorTitle = QTStr(
			"PreLiveWizard.Configure.ServiceNotAvailable.Title");
		QString errorDescription = QTStr(
			"PreLiveWizard.Configure.ServiceNotAvailable.Description");
		providerError(errorTitle, errorDescription);
	}
}

// SLOTS -------------
void PreStreamWizard::providerEncoderSettings(
	QSharedPointer<SettingsMap> response)
{
	blog(LOG_INFO, "PreStreamWizard got new settings response");
	newSettingsMap_ = response;
	if (this->currentId() == Page_Loading) {
		this->next();
	}
}

void PreStreamWizard::providerError(QString title, QString description)
{
	// TODO: Add Wizard Page with finish that shows this
}

void PreStreamWizard::onPageChanged(int id)
{
	if (id == Page_StartPrompt) {
		setOption(QWizard::NoCancelButton, false);
	}

	if (id == Page_Loading) {
		requestSettings();
	}
}

void PreStreamWizard::onUserSelectResolution(QSize newSize)
{
	blog(LOG_INFO, "Selected res %d x %d", newSize.width(),
	     newSize.height());
	EncoderSettingsRequest *req = currentSettings_.get();
	req->userSelectedResolution = QVariant(newSize);
}

} // namespace StreamWizard
