#include "pre-stream-wizard.hpp"

#include <QWizardPage>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFormLayout>
#include <QLabel>

namespace StreamWizard {

enum PSW_Page { Page_InfoDebug };

PreStreamWizard::PreStreamWizard(
	Destination dest,
	QSharedPointer<EncoderSettingsRequest> currentSettings, QWidget *parent)
	: QWizard(parent), destination_(dest)
{
	this->currentSettings_ = currentSettings;
	this->destination_ = dest;
	setWizardStyle(QWizard::ModernStyle);
	setWindowTitle("Encoder Config");

	setPage(Page_InfoDebug, makeDebugPage());
}

PreStreamWizard::~PreStreamWizard()
{
	currentSettings_ = nullptr;
}

QWizardPage *PreStreamWizard::makeDebugPage()
{
	QWizardPage *page = new QWizardPage(this);
	page->setTitle("Demo + Show Data");

	// Layout for the entire widget / Wizard PAge
	QVBoxLayout *mainLayout = new QVBoxLayout(page);

	// Scroll area that contains values
	QScrollArea *scroll = new QScrollArea();

	// Data list
	QWidget *formContainer = new QWidget();
	QFormLayout *form = new QFormLayout(formContainer);
	EncoderSettingsRequest *sett = currentSettings_.get();

	form->addRow("Server Url",
		     new QLabel(QString::fromUtf8(sett->serverUrl)));
	form->addRow("Service",
		     new QLabel(QString::fromUtf8(sett->serviceName)));
	form->addRow("Video Width",
		     new QLabel(QString::number(sett->videoWidth)));
	form->addRow("Video Height",
		     new QLabel(QString::number(sett->videoHeight)));
	form->addRow("Framerate", new QLabel(QString::number(sett->framerate)));
	form->addRow("V Bitrate",
		     new QLabel(QString::number(sett->videoBitrate)));
	form->addRow("A channels",
		     new QLabel(QString::number(sett->audioChannels)));
	form->addRow("A Samplerate",
		     new QLabel(QString::number(sett->audioSamplerate)));
	form->addRow("A Bitrate",
		     new QLabel(QString::number(sett->audioBitrate)));

	page->setLayout(mainLayout);
	mainLayout->addWidget(scroll);
	scroll->setWidget(formContainer);
	return page;
}

} // namespace StreamWizard
