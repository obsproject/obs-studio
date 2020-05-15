#include <QFile>
#include <QTextStream>
#include <QScrollBar>
#include <QFont>
#include <QFontDatabase>
#include <QPushButton>
#include <QCheckBox>
#include <QLayout>
#include <string>

#include "log-viewer.hpp"
#include "qt-wrappers.hpp"

OBSLogViewer::OBSLogViewer(QWidget *parent) : QDialog(parent)
{
	setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

	QVBoxLayout *layout = new QVBoxLayout();
	layout->setContentsMargins(0, 0, 0, 0);

	const QFont fixedFont =
		QFontDatabase::systemFont(QFontDatabase::FixedFont);

	textArea = new QTextEdit();
	textArea->setReadOnly(true);
	textArea->setFont(fixedFont);

	QHBoxLayout *buttonLayout = new QHBoxLayout();
	QPushButton *clearButton = new QPushButton(QTStr("Clear"));
	connect(clearButton, &QPushButton::clicked, this,
		&OBSLogViewer::ClearText);
	QPushButton *closeButton = new QPushButton(QTStr("Close"));
	connect(closeButton, &QPushButton::clicked, this, &QDialog::hide);

	QCheckBox *showStartup = new QCheckBox(QTStr("ShowOnStartup"));
	showStartup->setChecked(ShowOnStartup());
	connect(showStartup, SIGNAL(toggled(bool)), this,
		SLOT(ToggleShowStartup(bool)));

	buttonLayout->addSpacing(10);
	buttonLayout->addWidget(showStartup);
	buttonLayout->addStretch();
	buttonLayout->addWidget(clearButton);
	buttonLayout->addWidget(closeButton);
	buttonLayout->addSpacing(10);
	buttonLayout->setContentsMargins(0, 0, 0, 4);

	layout->addWidget(textArea);
	layout->addLayout(buttonLayout);
	setLayout(layout);

	setWindowTitle(QTStr("LogViewer"));
	resize(800, 300);

	const char *geom = config_get_string(App()->GlobalConfig(), "LogViewer",
					     "geometry");

	if (geom != nullptr) {
		QByteArray ba = QByteArray::fromBase64(QByteArray(geom));
		restoreGeometry(ba);
	}

	InitLog();
}

OBSLogViewer::~OBSLogViewer()
{
	config_set_string(App()->GlobalConfig(), "LogViewer", "geometry",
			  saveGeometry().toBase64().constData());
}

void OBSLogViewer::ToggleShowStartup(bool checked)
{
	config_set_bool(App()->GlobalConfig(), "LogViewer", "ShowLogStartup",
			checked);
}

bool OBSLogViewer::ShowOnStartup()
{
	return config_get_bool(App()->GlobalConfig(), "LogViewer",
			       "ShowLogStartup");
}

extern QPointer<OBSLogViewer> obsLogViewer;

void OBSLogViewer::InitLog()
{
	char logDir[512];
	std::string path;

	if (GetConfigPath(logDir, sizeof(logDir), "obs-studio/logs")) {
		path += logDir;
		path += "/";
		path += App()->GetCurrentLog();
	}

	QFile file(QT_UTF8(path.c_str()));

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);

		while (!in.atEnd()) {
			QString line = in.readLine();
			AddLine(LOG_INFO, line);
		}

		file.close();
	}

	obsLogViewer = this;
}

void OBSLogViewer::AddLine(int type, const QString &str)
{
	QString msg = str.toHtmlEscaped();

	switch (type) {
	case LOG_WARNING:
		msg = QStringLiteral("<font color=\"#c08000\">") + msg +
		      QStringLiteral("</font>");
		break;
	case LOG_ERROR:
		msg = QStringLiteral("<font color=\"#c00000\">") + msg +
		      QStringLiteral("</font>");
		break;
	}

	QScrollBar *scroll = textArea->verticalScrollBar();
	bool bottomScrolled = scroll->value() >= scroll->maximum() - 10;

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());

	QTextCursor newCursor = textArea->textCursor();
	newCursor.movePosition(QTextCursor::End);
	newCursor.insertHtml(msg + QStringLiteral("<br>"));

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());
}

void OBSLogViewer::ClearText()
{
	textArea->clear();
}
