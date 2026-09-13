#include "OutputScheduleDialog.hpp"

#include <QCalendarWidget>
#include <QCheckBox>
#include <QComboBox>
#include <QDateEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTextCharFormat>
#include <QTimeEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <array>

namespace {
class ScheduleDialog : public QDialog {
public:
	using QDialog::QDialog;
	std::function<bool()> canClose;
	void reject() override
	{
		if (!canClose || canClose())
			QDialog::reject();
	}
};
}

void ShowOutputScheduleDialog(QWidget *parent, const QList<OutputSchedule> &entries,
			      const std::function<bool(const QList<OutputSchedule> &)> &save,
			      const std::function<QString(const char *)> &tr)
{
	ScheduleDialog dialog(parent);
	dialog.setObjectName("novaScheduler");
	dialog.setWindowTitle(tr("Scheduler.Title"));
	dialog.resize(1000, 740);
	auto *root = new QVBoxLayout(&dialog);
	root->setContentsMargins(20, 20, 20, 20);
	root->setSpacing(12);
	auto *heading = new QLabel(tr("Scheduler.Title"), &dialog);
	heading->setObjectName("novaHeading");
	root->addWidget(heading);
	auto *help = new QLabel(tr("Scheduler.Help"), &dialog);
	help->setWordWrap(true);
	root->addWidget(help);
	auto *columns = new QHBoxLayout;
	root->addLayout(columns, 1);
	auto *left = new QVBoxLayout;
	auto *right = new QVBoxLayout;
	columns->addLayout(left, 1);
	columns->addLayout(right, 1);
	columns->setSpacing(20);
	auto *calendar = new QCalendarWidget(&dialog);
	calendar->setObjectName("scheduleCalendar");
	calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);
	calendar->setGridVisible(false);
	QTextCharFormat weekdayFormat;
	weekdayFormat.setForeground(dialog.palette().windowText());
	for (int day = 1; day <= 7; ++day)
		calendar->setWeekdayTextFormat(Qt::DayOfWeek(day), weekdayFormat);
	left->addWidget(calendar);
	auto *dayHeading = new QLabel(&dialog);
	left->addWidget(dayHeading);
	auto *agenda = new QListWidget(&dialog);
	agenda->setObjectName("scheduleAgenda");
	left->addWidget(agenda, 1);
	left->addWidget(new QLabel(tr("Scheduler.CalendarLegend"), &dialog));
	right->addWidget(new QLabel(tr("Scheduler.All"), &dialog));
	auto *list = new QListWidget(&dialog);
	list->setObjectName("scheduleList");
	list->setMaximumHeight(150);
	right->addWidget(list);
	auto *form = new QFormLayout;
	right->addLayout(form);
	auto *name = new QLineEdit(&dialog);
	name->setObjectName("scheduleName");
	name->setPlaceholderText(tr("Scheduler.NamePlaceholder"));
	form->addRow(tr("Scheduler.Name"), name);
	auto *repeat = new QComboBox(&dialog);
	repeat->setObjectName("scheduleRepeat");
	repeat->addItems({tr("Scheduler.Once"), tr("Scheduler.Daily"), tr("Scheduler.Weekly")});
	form->addRow(tr("Scheduler.Repeat"), repeat);
	auto *time = new QTimeEdit(QTime::currentTime().addSecs(300), &dialog);
	time->setObjectName("scheduleTime");
	time->setDisplayFormat("HH:mm");
	form->addRow(tr("Scheduler.Time"), time);
	auto *first = new QDateEdit(QDate::currentDate(), &dialog);
	first->setObjectName("scheduleStartDate");
	first->setCalendarPopup(true);
	form->addRow(tr("Scheduler.StartDate"), first);
	auto *daysWidget = new QWidget(&dialog);
	auto *daysLayout = new QHBoxLayout(daysWidget);
	daysLayout->setContentsMargins(0, 0, 0, 0);
	std::array<QCheckBox *, 7> days;
	for (int day = 1; day <= 7; ++day) {
		auto *button = new QCheckBox(QLocale().standaloneDayName(day, QLocale::ShortFormat), daysWidget);
		button->setObjectName(QString("scheduleDay%1").arg(day));
		button->setAccessibleName(QLocale().standaloneDayName(day, QLocale::LongFormat));
		days[day - 1] = button;
		daysLayout->addWidget(button);
	}
	right->addWidget(daysWidget);
	auto *endRow = new QHBoxLayout;
	auto *hasEnd = new QCheckBox(tr("Scheduler.EndDate"), &dialog);
	auto *last = new QDateEdit(QDate::currentDate().addMonths(1), &dialog);
	last->setCalendarPopup(true);
	endRow->addWidget(hasEnd);
	endRow->addWidget(last);
	right->addLayout(endRow);
	auto *enabled = new QCheckBox(tr("Scheduler.Enabled"), &dialog);
	enabled->setObjectName("scheduleEnabled");
	right->addWidget(enabled);
	auto *next = new QLabel(&dialog);
	next->setWordWrap(true);
	right->addWidget(next);
	auto *actions = new QHBoxLayout;
	auto *add = new QPushButton(tr("Scheduler.New"), &dialog);
	auto *apply = new QPushButton(tr("Scheduler.Save"), &dialog);
	apply->setObjectName("saveSchedule");
	auto *remove = new QPushButton(tr("Scheduler.Delete"), &dialog);
	actions->addWidget(add);
	actions->addWidget(apply);
	actions->addWidget(remove);
	right->addLayout(actions);
	right->addStretch(1);
	auto *close = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
	root->addWidget(close);

	QString selected;
	bool loading = false;
	bool dirty = false;
	auto draft = [&] {
		OutputSchedule entry;
		if (!selected.isEmpty())
			entry.id = selected;
		entry.name = name->text().trimmed();
		entry.firstDate = first->date();
		entry.time = QTime(time->time().hour(), time->time().minute());
		entry.repeat = OutputSchedule::Repeat(repeat->currentIndex());
		entry.enabled = enabled->isChecked();
		entry.lastDate = hasEnd->isChecked() && entry.repeat != OutputSchedule::Once ? last->date() : QDate();
		for (int day = 0; day < 7; ++day)
			if (days[day]->isChecked())
				entry.weekdays |= 1 << day;
		return entry;
	};
	auto updatePreview = [&] {
		const auto entry = draft();
		daysWidget->setVisible(entry.repeat == OutputSchedule::Weekly);
		hasEnd->setEnabled(entry.repeat != OutputSchedule::Once);
		last->setEnabled(hasEnd->isChecked() && hasEnd->isEnabled());
		const auto upcoming = entry.NextAfter(QDateTime::currentDateTimeUtc());
		next->setText(!entry.enabled ? tr("Scheduler.Paused") : upcoming.isValid()
			? tr("Scheduler.Next").arg(QLocale().toString(upcoming.toLocalTime(), QLocale::ShortFormat))
			: tr("Scheduler.NoNext"));
	};
	auto mayDiscard = [&] {
		return !dirty || QMessageBox::question(&dialog, dialog.windowTitle(), tr("Scheduler.Discard"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes;
	};
	dialog.canClose = mayDiscard;
	auto load = [&](const OutputSchedule &entry, bool existing) {
		loading = true;
		selected = existing ? entry.id : QString();
		name->setText(entry.name);
		repeat->setCurrentIndex(entry.repeat);
		time->setTime(entry.time);
		first->setDate(entry.firstDate);
		hasEnd->setChecked(entry.lastDate.isValid());
		last->setDate(entry.lastDate.isValid() ? entry.lastDate : entry.firstDate.addMonths(1));
		enabled->setChecked(entry.enabled);
		for (int day = 0; day < 7; ++day)
			days[day]->setChecked(entry.weekdays & (1 << day));
		remove->setEnabled(existing);
		dirty = false;
		loading = false;
		updatePreview();
	};
	auto newEntry = [&] {
		OutputSchedule entry;
		entry.firstDate = calendar->selectedDate();
		const auto soon = QDateTime::currentDateTime().addSecs(300);
		entry.time = QTime(soon.time().hour(), soon.time().minute());
		if (entry.firstDate == QDate::currentDate())
			entry.firstDate = soon.date();
		entry.weekdays = 1 << (entry.firstDate.dayOfWeek() - 1);
		load(entry, false);
	};
	auto refresh = [&] {
		QSignalBlocker block(list);
		list->clear();
		agenda->clear();
		calendar->setDateTextFormat(QDate(), QTextCharFormat());
		QTextCharFormat mark;
		mark.setFontWeight(QFont::Bold);
		mark.setBackground(dialog.palette().highlight());
		mark.setForeground(dialog.palette().highlightedText());
		const QDate month(calendar->yearShown(), calendar->monthShown(), 1);
		for (const auto &entry : entries) {
			const auto upcoming = entry.NextAfter(QDateTime::currentDateTimeUtc());
			const QString status = !entry.enabled ? tr("Scheduler.Paused") : upcoming.isValid()
				? tr("Scheduler.Next").arg(QLocale().toString(upcoming.toLocalTime(), QLocale::ShortFormat))
				: tr("Scheduler.NoNext");
			auto *item = new QListWidgetItem(entry.name + "\n" + status, list);
			item->setData(Qt::UserRole, entry.id);
			if (entry.id == selected)
				list->setCurrentItem(item);
			for (int day = 1; entry.enabled && day <= month.daysInMonth(); ++day) {
				const QDate date = month.addDays(day - 1);
				if (entry.OnDate(date).isValid())
					calendar->setDateTextFormat(date, mark);
			}
			if (entry.OnDate(calendar->selectedDate()).isValid()) {
				auto *event = new QListWidgetItem(entry.time.toString("HH:mm") + "  " + entry.name +
					(entry.enabled ? QString() : "  " + tr("Scheduler.Paused")), agenda);
				event->setData(Qt::UserRole, entry.id);
			}
		}
		agenda->sortItems();
		dayHeading->setText(QLocale().toString(calendar->selectedDate(), QLocale::LongFormat));
		if (!agenda->count()) {
			auto *empty = new QListWidgetItem(tr("Scheduler.EmptyDay"), agenda);
			empty->setFlags(Qt::NoItemFlags);
		}
	};
	auto selectEntry = [&](QListWidgetItem *item) {
		const auto id = item ? item->data(Qt::UserRole).toString() : QString();
		if (!item || !mayDiscard()) {
			refresh();
			return;
		}
		for (const auto &entry : entries)
			if (entry.id == id) {
				load(entry, true);
				break;
			}
		refresh();
	};
	QObject::connect(list, &QListWidget::itemClicked, &dialog, selectEntry);
	QObject::connect(agenda, &QListWidget::itemClicked, &dialog, selectEntry);
	QObject::connect(list, &QListWidget::itemActivated, &dialog, selectEntry);
	QObject::connect(agenda, &QListWidget::itemActivated, &dialog, selectEntry);
	QObject::connect(calendar, &QCalendarWidget::selectionChanged, &dialog, [&] {
		if (selected.isEmpty())
			first->setDate(calendar->selectedDate());
		refresh();
	});
	QObject::connect(calendar, &QCalendarWidget::currentPageChanged, &dialog, refresh);
	auto edited = [&] { if (!loading) { dirty = true; updatePreview(); } };
	QObject::connect(name, &QLineEdit::textChanged, &dialog, edited);
	QObject::connect(repeat, &QComboBox::currentIndexChanged, &dialog, edited);
	QObject::connect(time, &QTimeEdit::timeChanged, &dialog, edited);
	QObject::connect(first, &QDateEdit::dateChanged, &dialog, edited);
	QObject::connect(last, &QDateEdit::dateChanged, &dialog, edited);
	QObject::connect(hasEnd, &QCheckBox::toggled, &dialog, edited);
	QObject::connect(enabled, &QCheckBox::toggled, &dialog, edited);
	for (auto *day : days)
		QObject::connect(day, &QCheckBox::toggled, &dialog, edited);
	QObject::connect(add, &QPushButton::clicked, &dialog, [&] {
		if (mayDiscard()) { newEntry(); refresh(); }
	});
	QObject::connect(apply, &QPushButton::clicked, &dialog, [&] {
		auto entry = draft();
		if (!entry.IsValid() || (entry.enabled && !entry.NextAfter(QDateTime::currentDateTimeUtc()).isValid())) {
			QMessageBox::warning(&dialog, dialog.windowTitle(), tr("Scheduler.Invalid"));
			return;
		}
		auto updated = entries;
		bool found = false;
		for (auto &existing : updated)
			if (existing.id == selected) {
				entry.lastRun = existing.lastRun;
				existing = entry;
				found = true;
				break;
			}
		if (!found)
			updated.append(entry);
		if (!save(updated)) {
			QMessageBox::warning(&dialog, dialog.windowTitle(), tr("Basic.Main.Schedule.SaveFailed"));
			return;
		}
		load(entry, true);
		refresh();
	});
	QObject::connect(remove, &QPushButton::clicked, &dialog, [&] {
		if (selected.isEmpty() || QMessageBox::question(&dialog, dialog.windowTitle(), tr("Scheduler.ConfirmDelete"),
			QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
			return;
		auto updated = entries;
		for (auto it = updated.begin(); it != updated.end();)
			if (it->id == selected) it = updated.erase(it); else ++it;
		if (!save(updated)) {
			QMessageBox::warning(&dialog, dialog.windowTitle(), tr("Basic.Main.Schedule.SaveFailed"));
			return;
		}
		newEntry();
		refresh();
	});
	QObject::connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
	QTimer timer;
	QObject::connect(&timer, &QTimer::timeout, &dialog, refresh);
	timer.start(15000);
	newEntry();
	refresh();
	dialog.exec();
	dialog.canClose = {};
}
