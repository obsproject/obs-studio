#pragma once

#include <QString>

struct ChannelDescription {
	QString id;
	QString title;
};

struct StreamDescription {
	QString id;
	QString name;
	QString title;
};

struct CategoryDescription {
	QString id;
	QString title;
};

struct BroadcastDescription {
	QString id;
	QString title;
	QString description;
	QString privacy;
	CategoryDescription category;
	QString latency;
	bool made_for_kids;
	bool auto_start;
	bool auto_stop;
	bool dvr;
	bool schedul_for_later;
	QString schedul_date_time;
	QString projection;
};