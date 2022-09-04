#include "window-basic-about.hpp"
#include "window-basic-main.hpp"
#include "qt-wrappers.hpp"
#include "remote-text.hpp"
#include <util/util.hpp>
#include <util/platform.h>
#include <platform.hpp>
#include <json11.hpp>

using namespace json11;

OBSAbout::OBSAbout(QWidget *parent) : QDialog(parent), ui(new Ui::OBSAbout)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	QString bitness;
	QString ver;

	if (sizeof(void *) == 4)
		bitness = " (32 bit)";
	else if (sizeof(void *) == 8)
		bitness = " (64 bit)";

#ifdef HAVE_OBSCONFIG_H
	ver += OBS_VERSION;
#else
	ver += LIBOBS_API_MAJOR_VER + "." + LIBOBS_API_MINOR_VER + "." +
	       LIBOBS_API_PATCH_VER;
#endif

	ui->version->setText(ver + bitness);

	ui->tabWidget->setTabText(0, QTStr("About"));
	ui->tabWidget->setTabText(1, QTStr("About.Authors"));
	ui->tabWidget->setTabText(2, QTStr("About.Patrons"));
	ui->tabWidget->setTabText(3, QTStr("About.License"));

	QPointer<OBSAbout> about(this);

	OBSBasic *main = OBSBasic::Get();
	if (main->patronJson.empty() && !main->patronJsonThread) {
		RemoteTextThread *thread = new RemoteTextThread(
			"https://obsproject.com/patreon/about-box.json",
			"application/json");
		QObject::connect(thread, &RemoteTextThread::Result, main,
				 &OBSBasic::UpdatePatronJson);
		QObject::connect(thread, &RemoteTextThread::Result, this,
				 &OBSAbout::ShowPatrons);
		main->patronJsonThread.reset(thread);
		thread->start();
	}

	ShowAbout();
	ShowAuthors();
	ShowPatrons();
	ShowLicense();

	ui->name->setProperty("themeID", "aboutName");
	ui->version->setProperty("themeID", "aboutVersion");
}

void OBSAbout::ShowAbout()
{
	ui->donate->setText(
		"&nbsp;&nbsp;<a href='https://obsproject.com/contribute'>" +
		QTStr("About.Donate") + "</a>");
	ui->donate->setTextInteractionFlags(Qt::TextBrowserInteraction);
	ui->donate->setOpenExternalLinks(true);

	ui->getInvolved->setText(
		"&nbsp;&nbsp;<a href='https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.rst'>" +
		QTStr("About.GetInvolved") + "</a>");
	ui->getInvolved->setTextInteractionFlags(Qt::TextBrowserInteraction);
	ui->getInvolved->setOpenExternalLinks(true);
}

void OBSAbout::ShowPatrons()
{
	OBSBasic *main = OBSBasic::Get();

	if (main->patronJson.empty())
		return;

	std::string error;
	Json json = Json::parse(main->patronJson, error);
	const Json::array &patrons = json.array_items();
	QString text;

	text += "<h1>Top Patreon contributors:</h1>";
	text += "<p style=\"font-size:16px;\">";
	bool first = true;
	bool top = true;

	for (const Json &patron : patrons) {
		std::string name = patron["name"].string_value();
		std::string link = patron["link"].string_value();
		int amount = patron["amount"].int_value();

		if (top && amount < 10000) {
			text += "</p>";
			top = false;
		} else if (!first) {
			text += "<br/>";
		}

		if (!link.empty()) {
			text += "<a href=\"";
			text += QT_UTF8(link.c_str()).toHtmlEscaped();
			text += "\">";
		}
		text += QT_UTF8(name.c_str()).toHtmlEscaped();
		if (!link.empty())
			text += "</a>";

		if (first)
			first = false;
	}

	ui->patronsText->setHtml(text);
}

void OBSAbout::ShowAuthors()
{
	std::string path;
	QString error = "Error! File could not be read.\n\n \
		Go to: https://github.com/obsproject/obs-studio/blob/master/AUTHORS";

#ifdef __APPLE__
	if (!GetDataFilePath("AUTHORS", path)) {
#else
	if (!GetDataFilePath("authors/AUTHORS", path)) {
#endif
		ui->authorsText->setPlainText(error);
		return;
	}

	ui->authorsText->setPlainText(QString::fromStdString(path));

	BPtr<char> text = os_quick_read_utf8_file(path.c_str());

	if (!text || !*text) {
		ui->authorsText->setPlainText(error);
		return;
	}

	ui->authorsText->setPlainText(QT_UTF8(text));
}

void OBSAbout::ShowLicense()
{
	std::string path;
	QString error = "Error! File could not be read.\n\n \
		Go to: https://github.com/obsproject/obs-studio/blob/master/COPYING";

	if (!GetDataFilePath("license/gplv2.txt", path)) {
		ui->licenseText->setPlainText(error);
		return;
	}

	BPtr<char> text = os_quick_read_utf8_file(path.c_str());

	if (!text || !*text) {
		ui->licenseText->setPlainText(error);
		return;
	}

	ui->licenseText->setPlainText(QT_UTF8(text));
}
