#include "pre-stream-wizard.hpp"

#include <QWizardPage>
#include <QFormLayout>
#include <QLabel>
#include <QSize>

#include "page-input-display.hpp"
#include "page-start-prompt.hpp"

#include "obs-app.hpp"

namespace StreamWizard {

enum PSW_Page { Page_InfoDebug, Page_StartPrompt };

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

	// Always First Page, prompt
	QSize currentRes(currentSettings_->videoWidth,
			 currentSettings_->videoHeight);
	userSelectedNewRes_ = currentRes;
	StartPage *startPage =
		new StartPage(destination_, launchContext_, currentRes, this);
	setPage(Page_StartPrompt, startPage);
	connect(startPage, &StartPage::userSelectedResolution, this,
		&PreStreamWizard::onUserSelectResolution);
}

PreStreamWizard::~PreStreamWizard()
{
	currentSettings_ = nullptr;
}

void PreStreamWizard::onPageChanged(int id)
{
	if (id == Page_StartPrompt) {
		setOption(QWizard::NoCancelButton, false);
	}
}

void PreStreamWizard::onUserSelectResolution(QSize newSize)
{
	blog(LOG_INFO, "Selected res %d x %d", newSize.width(),
	     newSize.height());
	userSelectedNewRes_ = newSize;
}

} // namespace StreamWizard
