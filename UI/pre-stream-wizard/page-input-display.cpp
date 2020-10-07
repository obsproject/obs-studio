#include "page-input-display.hpp"

#include <QWizardPage>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QFormLayout>
#include <QLabel>

#include "pre-stream-current-settings.hpp"

QWizardPage *SettingsInputPage(StreamWizard::EncoderSettingsRequest *settings)
{
	QWizardPage *page = new QWizardPage();
	page->setTitle("Demo + Show Data");

	// Layout for the entire widget / Wizard PAge
	QVBoxLayout *mainLayout = new QVBoxLayout(page);

	// Scroll area that contains values
	QScrollArea *scroll = new QScrollArea();

	// Data list
	QWidget *formContainer = new QWidget();
	QFormLayout *form = new QFormLayout(formContainer);

	form->addRow("Video Width",
		     new QLabel(QString::number(settings->videoWidth)));
	form->addRow("Video Height",
		     new QLabel(QString::number(settings->videoHeight)));
	form->addRow("Framerate",
		     new QLabel(QString::number(settings->framerate)));
	form->addRow("V Bitrate",
		     new QLabel(QString::number(settings->videoBitrate)));
	form->addRow("A channels",
		     new QLabel(QString::number(settings->audioChannels)));
	form->addRow("A Samplerate",
		     new QLabel(QString::number(settings->audioSamplerate)));
	form->addRow("A Bitrate",
		     new QLabel(QString::number(settings->audioBitrate)));

	page->setLayout(mainLayout);
	mainLayout->addWidget(scroll);
	scroll->setWidget(formContainer);
	return page;
}
