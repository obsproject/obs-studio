#include "OBSAbout.hpp"
#include <OBSApp.hpp>
#include <utility/RemoteTextThread.hpp>
#include <utility/platform.hpp>

#include <qt-wrappers.hpp>

#include <json11.hpp>

#include "moc_OBSAbout.cpp"

namespace {
QString patronJson;
} // namespace

using namespace json11;

extern bool steam;

OBSAbout::OBSAbout(QWidget *parent) : QDialog(parent), ui(new Ui::OBSAbout)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	ui->setupUi(this);

	QString bitness;

	if (sizeof(void *) == 4)
		bitness = " (32 bit)";
	else if (sizeof(void *) == 8)
		bitness = " (64 bit)";

	QString ver = obs_get_version_string();

	ui->version->setText(ver + bitness);

	ui->contribute->setText(QTStr("About.Contribute"));

	if (steam) {
		delete ui->donate;
	} else {
		ui->donate->setText("&nbsp;&nbsp;<a href='https://obsproject.com/contribute'>" + QTStr("About.Donate") +
				    "</a>");
		ui->donate->setTextInteractionFlags(Qt::TextBrowserInteraction);
		ui->donate->setOpenExternalLinks(true);
	}

	ui->getInvolved->setText(
		"&nbsp;&nbsp;<a href='https://github.com/obsproject/obs-studio/blob/master/CONTRIBUTING.rst'>" +
		QTStr("About.GetInvolved") + "</a>");
	ui->getInvolved->setTextInteractionFlags(Qt::TextBrowserInteraction);
	ui->getInvolved->setOpenExternalLinks(true);

	ui->about->setText("<a href='#'>" + QTStr("About") + "</a>");
	ui->authors->setText("<a href='#'>" + QTStr("About.Authors") + "</a>");
	ui->license->setText("<a href='#'>" + QTStr("About.License") + "</a>");

	ui->name->setProperty("class", "text-heading");
	ui->version->setProperty("class", "text-large");
	ui->about->setProperty("class", "bg-base");
	ui->authors->setProperty("class", "bg-base");
	ui->license->setProperty("class", "bg-base");
	ui->info->setProperty("class", "");

	connect(ui->about, &ClickableLabel::clicked, this, &OBSAbout::ShowAbout);
	connect(ui->authors, &ClickableLabel::clicked, this, &OBSAbout::ShowAuthors);
	connect(ui->license, &ClickableLabel::clicked, this, &OBSAbout::ShowLicense);

	if (patronJson.isEmpty()) {
		RemoteTextThread *thread =
			new RemoteTextThread("https://obsproject.com/patreon/about-box.json", "application/json");
		connect(thread, &RemoteTextThread::Result, this, &OBSAbout::updatePatronJson);
		patronJsonThread.reset(thread);
		thread->start();
	} else {
		ShowAbout();
	}
}

OBSAbout::~OBSAbout()
{
	if (patronJsonThread && patronJsonThread->isRunning())
		patronJsonThread->wait();
}

void OBSAbout::updatePatronJson(const QString &text, const QString &error)
{
	if (!error.isEmpty())
		return;

	patronJson = text;

	ShowAbout();
}

void OBSAbout::ShowAbout()
{
	if (patronJson.isEmpty())
		return;

	std::string error;
	Json json = Json::parse(patronJson.toStdString(), error);
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

		if (top && amount < 5000) {
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

	ui->textBrowser->setHtml(text);
}

void OBSAbout::ShowAuthors()
{
	std::string path;
	QString error = QTStr("About.Error").arg("https://github.com/obsproject/obs-studio/blob/master/AUTHORS");

#ifdef __APPLE__
	if (!GetDataFilePath("AUTHORS", path)) {
#else
	if (!GetDataFilePath("authors/AUTHORS", path)) {
#endif
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
}

void OBSAbout::ShowLicense()
{
	std::string path;
	QString error = QTStr("About.Error").arg("https://github.com/obsproject/obs-studio/blob/master/COPYING");

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
}
