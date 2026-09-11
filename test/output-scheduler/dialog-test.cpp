#include <widgets/OutputScheduleDialog.hpp>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QFile>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QTimer>
#include <iostream>

class HiddenDialogs : public QObject {
	bool eventFilter(QObject *object, QEvent *event) override
	{
		if (event->type() == QEvent::Polish)
			if (auto *dialog = qobject_cast<QDialog *>(object))
				dialog->setAttribute(Qt::WA_DontShowOnScreen);
		return false;
	}
};

int main(int argc, char **argv)
{
	QApplication app(argc, argv);
	HiddenDialogs hidden;
	app.installEventFilter(&hidden);
	app.setStyle("Fusion");
	QPalette palette;
	palette.setColor(QPalette::Window, QColor("#191e18"));
	palette.setColor(QPalette::WindowText, QColor("#edf1df"));
	palette.setColor(QPalette::Text, QColor("#edf1df"));
	palette.setColor(QPalette::Base, QColor("#111510"));
	palette.setColor(QPalette::Button, QColor("#242c20"));
	palette.setColor(QPalette::ButtonText, QColor("#edf1df"));
	palette.setColor(QPalette::Highlight, QColor("#465837"));
	palette.setColor(QPalette::HighlightedText, QColor("#edf1df"));
	app.setPalette(palette);
	QFile theme(QString(SCHEDULER_FRONTEND) + "/data/themes/Nova.ovt");
	if (!theme.open(QIODevice::ReadOnly)) return 1;
	QString style = QString::fromUtf8(theme.readAll());
	style.remove(QRegularExpression("@OBSTheme\\w+\\s*\\{[^}]*\\}"));
	app.setStyleSheet(style);
	QFile locale(QString(SCHEDULER_FRONTEND) + "/data/locale/en-US.ini");
	if (!locale.open(QIODevice::ReadOnly)) return 1;
	QMap<QString, QString> strings;
	for (const auto &line : QString::fromUtf8(locale.readAll()).split('\n')) {
		const int equal = line.indexOf('=');
		if (equal < 0) continue;
		QString value = line.mid(equal + 1).trimmed();
		if (value.startsWith('"') && value.endsWith('"')) value = value.mid(1, value.size() - 2);
		strings.insert(line.left(equal), value.replace("\\n", "\n"));
	}
	QList<OutputSchedule> entries;
	OutputSchedule show;
	show.name = "Morning studio";
	show.firstDate = QDate::currentDate();
	show.time = QTime(9, 0);
	show.repeat = OutputSchedule::Daily;
	entries.append(show);
	show.id = "weekly";
	show.name = "Weekend creative hour";
	show.time = QTime(18, 0);
	show.repeat = OutputSchedule::Weekly;
	show.weekdays = 96;
	entries.append(show);
	int saved = 0;
	bool rendered = true;
	QTimer::singleShot(400, [&] {
		QDialog *dialog = nullptr;
		for (auto *widget : app.topLevelWidgets())
			if (widget->objectName() == "novaScheduler") dialog = qobject_cast<QDialog *>(widget);
		if (!dialog) { app.exit(1); return; }
		dialog->findChild<QLineEdit *>("scheduleName")->setText("Evening live show");
		dialog->findChild<QComboBox *>("scheduleRepeat")->setCurrentIndex(OutputSchedule::Weekly);
		dialog->findChild<QDateEdit *>("scheduleStartDate")->setDate(QDate::currentDate().addDays(1));
		for (int day = 1; day <= 7; ++day)
			dialog->findChild<QCheckBox *>(QString("scheduleDay%1").arg(day))->setChecked(day == 1 || day == 3 || day == 5);
		dialog->findChild<QPushButton *>("saveSchedule")->click();
		dialog->findChild<QCheckBox *>("scheduleEnabled")->setChecked(false);
		dialog->findChild<QPushButton *>("saveSchedule")->click();
		if (entries.last().enabled) { dialog->done(0); return; }
		dialog->findChild<QCheckBox *>("scheduleEnabled")->setChecked(true);
		dialog->findChild<QPushButton *>("saveSchedule")->click();
		if (argc > 1) rendered = dialog->grab().save(QString::fromLocal8Bit(argv[1]));
		dialog->done(0);
	});
	QTimer::singleShot(3000, [&] {
		for (auto *widget : app.topLevelWidgets()) {
			std::cerr << widget->metaObject()->className() << ": " << widget->windowTitle().toStdString() << '\n';
			if (auto *box = qobject_cast<QMessageBox *>(widget)) {
				std::cerr << box->text().toStdString() << '\n';
				box->done(0);
			}
		}
	});
	ShowOutputScheduleDialog(nullptr, entries, [&](const QList<OutputSchedule> &updated) {
		entries = updated;
		++saved;
		return true;
	}, [&](const char *key) { return strings.value(QString::fromUtf8(key), QString::fromUtf8(key)); });
	if (saved != 3 || entries.size() != 3 || entries.last().weekdays != 21 || !entries.last().enabled || !rendered) return 1;
	std::cout << "PASS: calendar editor creates and saves Monday/Wednesday/Friday recurrence\n";
}
