#include "OBSLogViewer.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include <QDesktopServices>
#include <QFile>
#include <QScrollBar>

#include "moc_OBSLogViewer.cpp"

OBSLogViewer::OBSLogViewer(QWidget *parent) : QDialog(parent), ui(new Ui::OBSLogViewer)
{
	setWindowFlags(windowFlags() & Qt::WindowMaximizeButtonHint & ~Qt::WindowContextHelpButtonHint);
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	bool showLogViewerOnStartup = config_get_bool(App()->GetUserConfig(), "LogViewer", "ShowLogStartup");

	ui->showStartup->setChecked(showLogViewerOnStartup);

	const char *geom = config_get_string(App()->GetUserConfig(), "LogViewer", "geometry");

	if (geom != nullptr) {
		QByteArray ba = QByteArray::fromBase64(QByteArray(geom));
		restoreGeometry(ba);
	}

	InitLog();
}

OBSLogViewer::~OBSLogViewer()
{
	config_set_string(App()->GetUserConfig(), "LogViewer", "geometry", saveGeometry().toBase64().constData());
}

void OBSLogViewer::on_showStartup_clicked(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "LogViewer", "ShowLogStartup", checked);
}

extern QPointer<OBSLogViewer> obsLogViewer;

void OBSLogViewer::InitLog()
{
	char logDir[512];
	std::string path;

	if (GetAppConfigPath(logDir, sizeof(logDir), "obs-studio/logs")) {
		path += logDir;
		path += "/";
		path += App()->GetCurrentLog();
	}

	QFile file(QT_UTF8(path.c_str()));

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);

		QTextDocument *doc = ui->textArea->document();
		QTextCursor cursor(doc);
		cursor.movePosition(QTextCursor::End);
		cursor.beginEditBlock();
		while (!in.atEnd()) {
			QString line = in.readLine();
			cursor.insertText(line);
			cursor.insertBlock();
		}
		cursor.endEditBlock();

		file.close();
	}
	QScrollBar *scroll = ui->textArea->verticalScrollBar();
	scroll->setValue(scroll->maximum());

	obsLogViewer = this;
}

void OBSLogViewer::AddLine(int type, const QString &str)
{
	QString msg = str.toHtmlEscaped();

	switch (type) {
	case LOG_WARNING:
		msg = QString("<font color=\"#c08000\">%1</font>").arg(msg);
		break;
	case LOG_ERROR:
		msg = QString("<font color=\"#c00000\">%1</font>").arg(msg);
		break;
	default:
		msg = QString("<font>%1</font>").arg(msg);
		break;
	}

	QScrollBar *scroll = ui->textArea->verticalScrollBar();
	bool bottomScrolled = scroll->value() >= scroll->maximum() - 10;

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());

	QTextDocument *doc = ui->textArea->document();
	QTextCursor cursor(doc);
	cursor.movePosition(QTextCursor::End);
	cursor.beginEditBlock();
	cursor.insertHtml(msg);
	cursor.insertBlock();
	cursor.endEditBlock();

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());
}

void OBSLogViewer::on_openButton_clicked()
{
	char logDir[512];
	if (GetAppConfigPath(logDir, sizeof(logDir), "obs-studio/logs") <= 0)
		return;

	const char *log = App()->GetCurrentLog();

	std::string path = logDir;
	path += "/";
	path += log;

	QUrl url = QUrl::fromLocalFile(QT_UTF8(path.c_str()));
	QDesktopServices::openUrl(url);
}
