#include "OBSLogViewer.hpp"

#include <OBSApp.hpp>

#include <qt-wrappers.hpp>

#include <QDesktopServices>
#include <QFile>
#include <QScrollBar>
#include <QKeyEvent>
#include <QRegularExpression>
#include <QListView>
#include <QMenu>
#include <QStandardItem>

#include "moc_OBSLogViewer.cpp"

OBSLogViewer::OBSLogViewer(QWidget *parent) : QDialog(parent), ui(new Ui::OBSLogViewer)
{
	setWindowFlags(windowFlags() & Qt::WindowMaximizeButtonHint & ~Qt::WindowContextHelpButtonHint);
	setAttribute(Qt::WA_DeleteOnClose);

	ui->setupUi(this);

	ui->searchBox->setProperty("themeID", "searchBox");

	// QSizePolicy buttonSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Expanding,
	// 			     QSizePolicy::ControlType::PushButton);
	// QIcon icon;
	// icon.addFile(QString::fromUtf8(":/res/images/close.svg"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
	// ui->closeSearch->setIcon(icon);
	// ui->closeSearch->setProperty("class", "icon-close");
	// ui->closeSearch->setSizePolicy(buttonSizePolicy);

	ui->searchNext->setEnabled(false);
	ui->searchPrev->setEnabled(false);
	ui->searchCount->setEnabled(false);

	ui->showInfo->setChecked(true);
	ui->showWarnings->setChecked(true);
	ui->showErrors->setChecked(true);
	ui->showDebug->setChecked(false);

	connect(ui->showInfo, &QAbstractButton::toggled, this, &OBSLogViewer::RefreshLogView);
	connect(ui->showWarnings, &QAbstractButton::toggled, this, &OBSLogViewer::RefreshLogView);
	connect(ui->showErrors, &QAbstractButton::toggled, this, &OBSLogViewer::RefreshLogView);
	connect(ui->showDebug, &QAbstractButton::toggled, this, &OBSLogViewer::RefreshLogView);

	// connect(ui->textArea, &QWidget::customContextMenuRequested, this,
	// 	&OBSLogViewer::on_textArea_customContextMenuRequested);

	bool showLogViewerOnStartup = config_get_bool(App()->GetUserConfig(), "LogViewer", "ShowLogStartup");

	ui->showStartup->setChecked(showLogViewerOnStartup);

	const char *geom = config_get_string(App()->GetUserConfig(), "LogViewer", "geometry");

	if (geom != nullptr) {
		QByteArray ba = QByteArray::fromBase64(QByteArray(geom));
		restoreGeometry(ba);
	}

	InitLog();

	connect(App(), &OBSApp::logLineAdded, this, &OBSLogViewer::AddLine);
}

OBSLogViewer::~OBSLogViewer()
{
	config_set_string(App()->GetUserConfig(), "LogViewer", "geometry", saveGeometry().toBase64().constData());
}

void OBSLogViewer::on_showStartup_clicked(bool checked)
{
	config_set_bool(App()->GetUserConfig(), "LogViewer", "ShowLogStartup", checked);
}

void OBSLogViewer::on_clearButton_clicked()
{
	lines.clear();
	RefreshLogView();
}

void OBSLogViewer::on_searchBox_textChanged(const QString &)
{
	UpdateSearch();
}

void OBSLogViewer::on_regexOption_toggled(bool)
{
	UpdateSearch();
}

void OBSLogViewer::on_caseSensitive_toggled(bool)
{
	UpdateSearch();
}

void OBSLogViewer::FocusSearch()
{
	ui->searchBox->setFocus(Qt::OtherFocusReason);

	QTextCursor cursor = ui->textArea->textCursor();
	if (cursor.hasSelection()) {
		QString selectedText = cursor.selectedText();
		if (!selectedText.isEmpty()) {
			ui->searchBox->setText(selectedText);
		}
	}

	ui->searchBox->selectAll();
}

void OBSLogViewer::keyPressEvent(QKeyEvent *event)
{
	if (event->matches(QKeySequence::Find)) {
		FocusSearch();
		return;
	}

	if (event->key() == Qt::Key_Escape && ui->searchWidget->isVisible()) {
		ui->searchBox->clear();
		return;
	}

	QDialog::keyPressEvent(event);
}

void OBSLogViewer::InitLog()
{
	char logDir[512];
	std::string path;

	if (GetAppConfigPath(logDir, sizeof(logDir), "obs-studio/logs")) {
		path += logDir;
		path += "/";
		path += App()->GetCurrentLog();
	}

	lines.clear();

	QFile file(QT_UTF8(path.c_str()));

	if (file.open(QIODevice::ReadOnly)) {
		QTextStream in(&file);

		QTextDocument *doc = ui->textArea->document();
		QTextCursor cursor(doc);
		cursor.movePosition(QTextCursor::End);
		cursor.beginEditBlock();
		while (!in.atEnd()) {
			QString line = in.readLine();
			AddLine(LOG_INFO, line);
		}
		cursor.endEditBlock();

		file.close();
	}
	QScrollBar *scroll = ui->textArea->verticalScrollBar();
	scroll->setValue(scroll->maximum());
}

void OBSLogViewer::RefreshLogView()
{
	ui->textArea->clear();

	QScrollBar *scroll = ui->textArea->verticalScrollBar();
	int savedScroll = scroll->value();
	bool bottomScrolled = savedScroll >= scroll->maximum() - 10;

	QTextDocument *doc = ui->textArea->document();
	QTextCursor cursor(doc);
	cursor.beginEditBlock();

	for (const LogLine &line : lines) {
		if (line.type == LOG_INFO && !ui->showInfo->isChecked())
			continue;
		if (line.type == LOG_WARNING && !ui->showWarnings->isChecked())
			continue;
		if (line.type == LOG_ERROR && !ui->showErrors->isChecked())
			continue;
		if (line.type == LOG_DEBUG && !ui->showDebug->isChecked())
			continue;

		QString msg = line.text.toHtmlEscaped();

		switch (line.type) {
		case LOG_WARNING:
			msg = QString("<font color=\"#c08000\">%1</font>").arg(msg);
			break;
		case LOG_ERROR:
			msg = QString("<font color=\"#c00000\">%1</font>").arg(msg);
			break;
		case LOG_DEBUG:
			msg = QString("<font color=\"#808080\">%1</font>").arg(msg);
			break;
		default:
			msg = QString("<font>%1</font>").arg(msg);
			break;
		}
		ui->textArea->appendHtml(msg);
	}

	cursor.endEditBlock();

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());
	else
		scroll->setValue(savedScroll);

	UpdateSearch();
}

void OBSLogViewer::AddLine(int type, const QString &text)
{
	LogLine line;
	line.type = type;
	line.text = text;
	lines.push_back(line);

	if (type == LOG_INFO && !ui->showInfo->isChecked())
		return;
	if (type == LOG_WARNING && !ui->showWarnings->isChecked())
		return;
	if (type == LOG_ERROR && !ui->showErrors->isChecked())
		return;
	if (type == LOG_DEBUG && !ui->showDebug->isChecked())
		return;

	QString msg = text.toHtmlEscaped();

	switch (type) {
	case LOG_WARNING:
		msg = QString("<font color=\"#c08000\">%1</font>").arg(msg);
		break;
	case LOG_ERROR:
		msg = QString("<font color=\"#c00000\">%1</font>").arg(msg);
		break;
	case LOG_DEBUG:
		msg = QString("<font color=\"#808080\">%1</font>").arg(msg);
		break;
	default:
		msg = QString("<font>%1</font>").arg(msg);
		break;
	}

	QScrollBar *scroll = ui->textArea->verticalScrollBar();
	bool bottomScrolled = scroll->value() >= scroll->maximum() - 10;

	ui->textArea->appendHtml(msg);

	if (bottomScrolled)
		scroll->setValue(scroll->maximum());

	if (!ui->searchBox->text().isEmpty())
		UpdateSearch();
}

void OBSLogViewer::UpdateSearch()
{
	ui->textArea->setExtraSelections({});

	QString text = ui->searchBox->text();
	if (text.isEmpty()) {
		ui->searchCount->setText("0/0");
		ui->searchCount->setEnabled(false);
		ui->searchNext->setEnabled(false);
		ui->searchPrev->setEnabled(false);
		return;
	}

	QTextDocument *doc = ui->textArea->document();
	QRegularExpression regex;

	QTextDocument::FindFlags flags;
	if (ui->caseSensitive->isChecked())
		flags |= QTextDocument::FindCaseSensitively;

	if (ui->regexOption->isChecked()) {
		regex.setPattern(text);
		if (!ui->caseSensitive->isChecked())
			regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	}

	QList<QTextEdit::ExtraSelection> selections;
	int count = 0;
	int currentIndex = 0;
	int cursorPosition = ui->textArea->textCursor().position();

	QTextCursor cursor(doc);
	while (!cursor.isNull() && !cursor.atEnd()) {
		if (ui->regexOption->isChecked())
			cursor = doc->find(regex, cursor);
		else
			cursor = doc->find(text, cursor, flags);

		if (!cursor.isNull()) {
			QTextEdit::ExtraSelection sel;
			sel.cursor = cursor;
			sel.format.setBackground(QColor(Qt::yellow));
			sel.format.setForeground(QColor(Qt::black));
			selections.append(sel);
			count++;

			if (cursor.position() <= cursorPosition)
				currentIndex = count;
		}
	}

	ui->textArea->setExtraSelections(selections);
	if (count == 0) {
		ui->searchCount->setText("0/0");
		ui->searchCount->setEnabled(false);
		ui->searchNext->setEnabled(false);
		ui->searchPrev->setEnabled(false);
	} else {
		ui->searchCount->setText(QString("%1/%2").arg(currentIndex).arg(count));
		ui->searchCount->setEnabled(true);
		ui->searchNext->setEnabled(true);
		ui->searchPrev->setEnabled(true);
	}
}

void OBSLogViewer::on_searchNext_clicked()
{
	QString text = ui->searchBox->text();
	if (text.isEmpty())
		return;

	QRegularExpression regex;
	QTextDocument::FindFlags flags;
	if (ui->caseSensitive->isChecked())
		flags |= QTextDocument::FindCaseSensitively;

	if (ui->regexOption->isChecked()) {
		regex.setPattern(text);
		if (!ui->caseSensitive->isChecked())
			regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	}

	bool found = false;
	if (ui->regexOption->isChecked())
		found = ui->textArea->find(regex);
	else
		found = ui->textArea->find(text, flags);

	if (!found) {
		ui->textArea->moveCursor(QTextCursor::Start);
		if (ui->regexOption->isChecked())
			ui->textArea->find(regex);
		else
			ui->textArea->find(text, flags);
	}
}

void OBSLogViewer::on_searchPrev_clicked()
{
	QString text = ui->searchBox->text();
	if (text.isEmpty())
		return;

	QTextDocument::FindFlags flags = QTextDocument::FindBackward;
	if (ui->caseSensitive->isChecked())
		flags |= QTextDocument::FindCaseSensitively;

	QRegularExpression regex;
	if (ui->regexOption->isChecked()) {
		regex.setPattern(text);
		if (!ui->caseSensitive->isChecked())
			regex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
	}

	bool found = false;
	if (ui->regexOption->isChecked())
		found = ui->textArea->find(regex, flags);
	else
		found = ui->textArea->find(text, flags);

	if (!found) {
		ui->textArea->moveCursor(QTextCursor::End);
		if (ui->regexOption->isChecked())
			ui->textArea->find(regex, flags);
		else
			ui->textArea->find(text, flags);
	}
}

void OBSLogViewer::on_textArea_customContextMenuRequested(const QPoint &pos)
{
	QMenu *menu = ui->textArea->createStandardContextMenu(pos);
	menu->addSeparator();
	QAction *action = menu->addAction(QTStr("LogViewer.Search"));
	action->setShortcut(QKeySequence::Find);

	QAction *sel = menu->exec(ui->textArea->mapToGlobal(pos));
	if (sel == action)
		FocusSearch();

	connect(menu, &QMenu::aboutToHide, menu, &QMenu::deleteLater);
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
