#include <utility/OutputSchedule.hpp>
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>

void check(bool value, const char *message)
{
	if (!value)
		throw std::runtime_error(message);
}

int main(int argc, char **argv)
{
	QCoreApplication app(argc, argv);
	try {
		const QTimeZone utc = QTimeZone::utc();
		auto at = [&](int day, int hour, int minute = 0) { return QDateTime(QDate(2026, 9, day), QTime(hour, minute), utc); };
		OutputSchedule entry;
		entry.name = "Weekly show";
		entry.firstDate = QDate(2026, 9, 1);
		entry.time = QTime(18, 0);
		entry.repeat = OutputSchedule::Weekly;
		entry.weekdays = (1 << 0) | (1 << 2);
		check(entry.NextAfter(at(6, 12), utc) == at(7, 18), "Sunday rolls to Monday");
		check(entry.NextAfter(at(7, 18), utc) == at(9, 18), "Strictly after skips fired occurrence");
		check(entry.NextAfter(at(9, 19), utc) == at(14, 18), "Weekly rollover");
		entry.lastDate = QDate(2026, 9, 9);
		check(entry.NextAfter(at(9, 17), utc) == at(9, 18), "Inclusive end date");
		check(!entry.NextAfter(at(9, 19), utc).isValid(), "End date stops recurrence");
		entry.lastDate = {};
		entry.enabled = false;
		check(!entry.NextAfter(at(6, 12), utc).isValid(), "Disabled schedule never fires");
		entry.enabled = true;
		entry.weekdays = 0;
		check(!entry.IsValid(), "Empty weekday selection rejected");
		entry.repeat = OutputSchedule::Daily;
		QList<OutputSchedule> entries{entry, entry};
		check(ConsumeDueOutputSchedules(entries, at(7, 17, 59), at(7, 18), utc), "Dispatch boundary");
		check(entries[0].lastRun == at(7, 18) && entries[1].lastRun == at(7, 18), "Consume simultaneous events");
		check(!ConsumeDueOutputSchedules(entries, at(7, 17, 59), at(7, 18), utc), "Modal reentry does not duplicate");
		check(!ConsumeDueOutputSchedules(entries, at(7, 18), at(10, 19), utc), "Skip long missed interval");
		check(ConsumeDueOutputSchedules(entries, at(7, 18), at(11, 18), utc), "Recent recurrence survives long sleep");
		check(!ConsumeDueOutputSchedules(entries, at(11, 17), at(11, 18), utc), "Clock rollback does not replay");
		const auto restored = OutputSchedule::FromJson(entries[0].ToJson());
		check(restored.IsValid() && restored.lastRun == entries[0].lastRun && restored.id == entries[0].id, "Persistence round trip");
		check(!OutputSchedule::FromJson({}).IsValid(), "Reject corrupt JSON entry");
		entry.repeat = OutputSchedule::Once;
		check(!entry.NextAfter(at(7, 12), utc).isValid(), "Expired one-time start");
		entry.firstDate = QDate(2027, 1, 1);
		check(entry.NextAfter(at(7, 12), utc).date() == entry.firstDate, "Far future first date");

		const QTimeZone ny("America/New_York");
		check(ny.isValid(), "Timezone database available");
		entry.repeat = OutputSchedule::Daily;
		entry.firstDate = QDate(2026, 3, 1);
		entry.time = QTime(2, 30);
		check(!entry.OnDate(QDate(2026, 3, 8), ny).isValid(), "Spring-forward gap is skipped");
		check(entry.NextAfter(QDateTime(QDate(2026, 3, 7), QTime(3, 0), ny), ny).date() == QDate(2026, 3, 9), "Daily recurrence resumes after DST gap");
		entry.time = QTime(1, 30);
		const auto fold = entry.OnDate(QDate(2026, 11, 1), ny);
		entry.lastRun = fold;
		check(entry.NextAfter(fold.addSecs(-1), ny).date() == QDate(2026, 11, 2), "Fall-back hour runs once");
		std::cout << "PASS: recurrence, persistence, missed starts, clock rollback and DST\n";
	} catch (const std::exception &error) {
		std::cerr << error.what() << '\n';
		return 1;
	}
}
