#include "window-basic-about.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include <util/util.hpp>
#include <util/platform.h>
#include <platform.hpp>

OBSAbout::OBSAbout(QWidget *parent)
	: QDialog(parent),
	  ui(new Ui::OBSAbout)
{
	ui->setupUi(this);

	setFixedSize(size());

	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QString bitness;
	QString ver;

	if(sizeof(void*) == 4)
		bitness = " (32 bit)";
	else if(sizeof(void*) == 8)
		bitness = " (64 bit)";

#ifdef HAVE_OBSCONFIG_H
	ver +=  OBS_VERSION;
#else
	ver +=  LIBOBS_API_MAJOR_VER + "." +
		LIBOBS_API_MINOR_VER + "." +
		LIBOBS_API_PATCH_VER;
#endif

	ui->version->setText(ver + bitness);

	ui->contribute->setText(QTStr("About.Contribute"));
	ui->donate->setText("<a href='https://obsproject.com/donate'>" +
			QTStr("About.Donate") + "</a>");
	ui->donate->setTextInteractionFlags(Qt::TextBrowserInteraction);
	ui->donate->setOpenExternalLinks(true);

	ui->getInvolved->setText("<a href='https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.rst'>" +
			QTStr("About.GetInvolved") + "</a>");
	ui->getInvolved->setTextInteractionFlags(Qt::TextBrowserInteraction);
	ui->getInvolved->setOpenExternalLinks(true);

	ui->about->setText("<a href='#'>" + QTStr("About") + "</a>");
	ui->authors->setText("<a href='#'>" + QTStr("About.Authors") + "</a>");
	ui->license->setText("<a href='#'>" + QTStr("About.License") + "</a>");

	ui->textBrowser->hide();

	ui->name->setProperty("themeID", "aboutName");
	ui->version->setProperty("themeID", "aboutVersion");
	ui->about->setProperty("themeID", "aboutHLayout");
	ui->authors->setProperty("themeID", "aboutHLayout");
	ui->license->setProperty("themeID", "aboutHLayout");
	ui->info->setProperty("themeID", "aboutInfo");

	connect(ui->about, SIGNAL(clicked()), this, SLOT(ShowAbout()));
	connect(ui->authors, SIGNAL(clicked()), this, SLOT(ShowAuthors()));
	connect(ui->license, SIGNAL(clicked()), this, SLOT(ShowLicense()));
}

void OBSAbout::ShowAbout()
{
	ui->textBrowser->hide();
	ui->info->show();
	ui->contribute->show();
	ui->donate->show();
	ui->getInvolved->show();
}

void OBSAbout::ShowAuthors()
{
	std::string path;
	QString error = "Error! File could not be read.\n\n \
		Go to: https://github.com/obsproject/obs-studio/blob/master/AUTHORS";

	if (!GetDataFilePath("authors/AUTHORS", path)) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	ui->textBrowser->setPlainText(QString::fromStdString(path));

	BPtr<char> text = os_quick_read_utf8_file(path.c_str());

	if (!text || !*text) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	ui->textBrowser->setPlainText(QT_UTF8(text));

	ui->info->hide();
	ui->contribute->hide();
	ui->donate->hide();
	ui->getInvolved->hide();
	ui->textBrowser->show();
}

void OBSAbout::ShowLicense()
{
	std::string path;
	QString error = "Error! File could not be read.\n\n \
		Go to: https://github.com/obsproject/obs-studio/blob/master/COPYING";

	if (!GetDataFilePath("license/gplv2.txt", path)) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	BPtr<char> text = os_quick_read_utf8_file(path.c_str());

	if (!text || !*text) {
		ui->textBrowser->setPlainText(error);
		return;
	}

	ui->textBrowser->setPlainText(QT_UTF8(text));

	ui->info->hide();
	ui->contribute->hide();
	ui->donate->hide();
	ui->getInvolved->hide();
	ui->textBrowser->show();
}
