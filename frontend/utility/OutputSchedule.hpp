#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QTimeZone>
#include <QUuid>

struct OutputSchedule {
	enum Repeat { Once, Daily, Weekly };
	QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
	QString name;
	QDate firstDate;
	QTime time;
	Repeat repeat = Once;
	int weekdays = 0; // Monday is bit 0; Sunday is bit 6.
	QDate lastDate; // Invalid means no end date.
	bool enabled = true;
	QDateTime lastRun;

	bool IsValid() const
	{
		return !id.isEmpty() && !name.trimmed().isEmpty() && firstDate.isValid() && time.isValid() &&
		       repeat >= Once && repeat <= Weekly && (repeat != Weekly || (weekdays & 127)) &&
		       (!lastDate.isValid() || lastDate >= firstDate);
	}

	QDateTime OnDate(QDate date, const QTimeZone &zone = QTimeZone::systemTimeZone()) const
	{
		if (!IsValid() || date < firstDate || (lastDate.isValid() && date > lastDate) ||
		    (repeat == Once && date != firstDate) ||
		    (repeat == Weekly && !(weekdays & (1 << (date.dayOfWeek() - 1)))))
			return {};
		QDateTime occurrence(date, time, zone);
		// Do not shift a nonexistent spring-forward wall time to a different hour.
		if (!occurrence.isValid() || occurrence.date() != date || occurrence.time() != time)
			return {};
		return occurrence;
	}

	QDateTime NextAfter(QDateTime after, const QTimeZone &zone = QTimeZone::systemTimeZone()) const
	{
		if (!enabled || !IsValid() || !after.isValid())
			return {};
		if (lastRun.isValid() && lastRun > after)
			after = lastRun;
		QDate date = after.toTimeZone(zone).date();
		if (date < firstDate)
			date = firstDate;
		// Two weeks covers a weekly event skipped by a DST gap, without scanning years.
		for (int day = 0; day < 15; ++day, date = date.addDays(1)) {
			if ((repeat == Once && date > firstDate) || (lastDate.isValid() && date > lastDate))
				break;
			const auto candidate = OnDate(date, zone);
			if (candidate.isValid() && candidate > after &&
			    (!lastRun.isValid() || candidate > lastRun))
				return candidate;
		}
		return {};
	}

	QJsonObject ToJson() const
	{
		return {{"id", id}, {"name", name}, {"firstDate", firstDate.toString(Qt::ISODate)},
			{"time", time.toString(Qt::ISODate)}, {"repeat", int(repeat)}, {"weekdays", weekdays},
			{"lastDate", lastDate.toString(Qt::ISODate)}, {"enabled", enabled},
			{"lastRun", lastRun.toUTC().toString(Qt::ISODateWithMs)}};
	}

	static OutputSchedule FromJson(const QJsonObject &json)
	{
		OutputSchedule entry;
		entry.id = json.value("id").toString();
		entry.name = json.value("name").toString();
		entry.firstDate = QDate::fromString(json.value("firstDate").toString(), Qt::ISODate);
		entry.time = QTime::fromString(json.value("time").toString(), Qt::ISODate);
		const int repeat = json.value("repeat").toInt(-1);
		entry.repeat = repeat >= Once && repeat <= Weekly ? Repeat(repeat) : Once;
		if (repeat < Once || repeat > Weekly)
			entry.id.clear();
		entry.weekdays = json.value("weekdays").toInt() & 127;
		entry.lastDate = QDate::fromString(json.value("lastDate").toString(), Qt::ISODate);
		entry.enabled = json.value("enabled").toBool(true);
		entry.lastRun = QDateTime::fromString(json.value("lastRun").toString(), Qt::ISODateWithMs);
		return entry;
	}
};

inline bool ConsumeDueOutputSchedules(QList<OutputSchedule> &entries, const QDateTime &previous,
				     const QDateTime &now, const QTimeZone &zone = QTimeZone::systemTimeZone())
{
	const auto since = previous > now.addSecs(-60) ? previous : now.addSecs(-60);
	bool due = false;
	for (auto &entry : entries) {
		const auto occurrence = entry.NextAfter(since, zone);
		if (occurrence.isValid() && occurrence <= now) {
			entry.lastRun = occurrence.toUTC();
			due = true;
		}
	}
	return due;
}
