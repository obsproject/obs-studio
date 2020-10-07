#include "pre-stream-wizard.hpp"

#include <QWizardPage>
#include <QFormLayout>
#include <QLabel>
#include <QSize>
#include <QList>

#include "obs-app.hpp"
#include "encoder-settings-provider-facebook.hpp"

#include "page-input-display.hpp"
#include "page-start-prompt.hpp"
#include "page-loading.hpp"
#include "page-select-settings.hpp"
#include "page-completed.hpp"
#include "page-error.hpp"

namespace StreamWizard {

enum PSW_Page {
	Page_StartPrompt,
	Page_Loading,
	Page_Selections,
	Page_Complete,
	Page_Error,
};

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
	setOption(QWizard::NoBackButtonOnStartPage, true);
	setOption(QWizard::NoBackButtonOnLastPage, true);

	// First Page: Explain to the user and confirm sending to API
	QSize currentRes(currentSettings_->videoWidth,
			 currentSettings_->videoHeight);
	startPage_ =
		new StartPage(destination_, launchContext_, currentRes, this);
	setPage(Page_StartPrompt, startPage_);
	connect(startPage_, &StartPage::userSelectedResolution, this,
		&PreStreamWizard::onUserSelectResolution);

	// Loading page: Shown when loading new settings
	loadingPage_ = new LoadingPage(this);
	setPage(Page_Loading, loadingPage_);

	// Suggestion Selection Page
	selectionPage_ = new SelectionPage(this);
	setPage(Page_Selections, selectionPage_);

	// Ending + Confirmation Page
	CompletedPage *completedPage =
		new CompletedPage(destination_, launchContext_, this);
	setPage(Page_Complete, completedPage);

	// Add error page shown instead of completion in failure cases
	errorPage_ = new ErrorPage(this);
	setPage(Page_Error, errorPage_);
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
	if (currentId() == Page_Loading) {
		next();
	}
}

void PreStreamWizard::providerError(QString title, QString description)
{
	sendToErrorPage_ = true;
	errorPage_->setText(title, description);
	if (currentId() == Page_Loading)
		next();
}

void PreStreamWizard::onPageChanged(int id)
{
	switch (id) {
	case Page_StartPrompt:
		setOption(QWizard::NoCancelButton, false);
		setButtons(NextStep);
		break;

	case Page_Loading:
		requestSettings();
		setButtons(CancelOnly);
		break;

	case Page_Selections:
		selectionPage_->setSettingsMap(newSettingsMap_);
		setButtons(CommitStep);
		break;

	case Page_Complete:
		setButtons(FinishOnly);
		if (newSettingsMap_ != nullptr && !newSettingsMap_.isNull()) {
			// ToDo: messaging in edge case this could be empty
			// and still make it here?
			emit applySettings(newSettingsMap_);
		};
		break;

	case Page_Error:
		sendToErrorPage_ = false;
		setButtons(FinishOnly);
		break;
	}
}

int PreStreamWizard::nextId() const
{
	switch (currentId()) {
	case Page_StartPrompt:
		return Page_Loading;
	case Page_Loading: {
		if (sendToErrorPage_)
			return Page_Error;
		if (newSettingsMap_ == nullptr || newSettingsMap_.isNull()) {
			errorPage_->setText(
				QTStr("PreLiveWizard.Configure.Error.NoData"),
				QTStr("PreLiveWizard.Configure.Error.JsonParse.Description"));
			return Page_Error;
		}
		return Page_Selections;
	}
	case Page_Selections:
		return Page_Complete;
	case Page_Complete:
		return -1;
	case Page_Error:
		return -1;
	}

	return QWizard::nextId();
}

void PreStreamWizard::setButtons(ButtonLayout layout)
{
	QList<QWizard::WizardButton> layoutList;
	layoutList << QWizard::Stretch;
	switch (layout) {
	case NextStep:
		layoutList << QWizard::NextButton << QWizard::CancelButton;
		break;
	case CommitStep:
		layoutList << QWizard::CommitButton << QWizard::CancelButton;
		break;
	case CancelOnly:
		layoutList << QWizard::CancelButton;
		break;
	case FinishOnly:
		layoutList << QWizard::FinishButton;
		break;
	}
	setButtonLayout(layoutList);
}

void PreStreamWizard::onUserSelectResolution(QSize newSize)
{
	blog(LOG_INFO, "Selected res %d x %d", newSize.width(),
	     newSize.height());
	EncoderSettingsRequest *req = currentSettings_.data();
	req->userSelectedResolution = QVariant(newSize);
}

} // namespace StreamWizard
