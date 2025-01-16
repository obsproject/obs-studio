#include "AutoConfigStartPage.hpp"
#include "AutoConfig.hpp"
#include "ui_AutoConfigStartPage.h"

#include <widgets/OBSBasic.hpp>

#include "moc_AutoConfigStartPage.cpp"

#define wiz reinterpret_cast<AutoConfig *>(wizard())

AutoConfigStartPage::AutoConfigStartPage(QWidget *parent) : QWizardPage(parent), ui(new Ui_AutoConfigStartPage)
{
	ui->setupUi(this);
	setTitle(QTStr("Basic.AutoConfig.StartPage"));
	setSubTitle(QTStr("Basic.AutoConfig.StartPage.SubTitle"));

	OBSBasic *main = OBSBasic::Get();
	if (main->VCamEnabled()) {
		QRadioButton *prioritizeVCam =
			new QRadioButton(QTStr("Basic.AutoConfig.StartPage.PrioritizeVirtualCam"), this);
		QBoxLayout *box = reinterpret_cast<QBoxLayout *>(layout());
		box->insertWidget(2, prioritizeVCam);

		connect(prioritizeVCam, &QPushButton::clicked, this, &AutoConfigStartPage::PrioritizeVCam);
	}
}

AutoConfigStartPage::~AutoConfigStartPage() {}

int AutoConfigStartPage::nextId() const
{
	return wiz->type == AutoConfig::Type::VirtualCam ? AutoConfig::TestPage : AutoConfig::VideoPage;
}

void AutoConfigStartPage::on_prioritizeStreaming_clicked()
{
	wiz->type = AutoConfig::Type::Streaming;
}

void AutoConfigStartPage::on_prioritizeRecording_clicked()
{
	wiz->type = AutoConfig::Type::Recording;
}

void AutoConfigStartPage::PrioritizeVCam()
{
	wiz->type = AutoConfig::Type::VirtualCam;
}
