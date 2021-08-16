#pragma once

#include "auth-youtube.hpp"

#include <json11.hpp>
#include <QString>

struct ChannelDescription {
	QString id;
	QString title;
	QString country;
	QString language;
};

struct StreamDescription {
	QString id;
	QString name;
	QString title;
	QString description;
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

struct BindDescription {
	const QString id;
	const QString stream_name;
};

bool IsYouTubeService(const std::string &service);

class YoutubeApiWrappers : public YoutubeAuth {
	Q_OBJECT

	bool TryInsertCommand(const char *url, const char *content_type,
			      std::string request_type, const char *data,
			      json11::Json &ret, long *error_code = nullptr);
	bool UpdateAccessToken();
	bool InsertCommand(const char *url, const char *content_type,
			   std::string request_type, const char *data,
			   json11::Json &ret);

public:
	YoutubeApiWrappers(const Def &d);

	bool GetChannelDescription(ChannelDescription &channel_description);
	bool InsertBroadcast(BroadcastDescription &broadcast);
	bool InsertStream(StreamDescription &stream);
	bool BindStream(const QString broadcast_id, const QString stream_id);
	bool GetBroadcastsList(json11::Json &json_out, QString page);
	bool
	GetVideoCategoriesList(const QString &country, const QString &language,
			       QVector<CategoryDescription> &category_list_out);
	bool SetVideoCategory(const QString &video_id,
			      const QString &video_title,
			      const QString &video_description,
			      const QString &categorie_id);
	bool StartBroadcast(const QString &broadcast_id);
	bool StopBroadcast(const QString &broadcast_id);
	bool ResetBroadcast(const QString &broadcast_id);
	bool StartLatestBroadcast();
	bool StopLatestBroadcast();

	bool FindBroadcast(const QString &id, json11::Json &json_out);
	bool FindStream(const QString &id, json11::Json &json_out);

	QString GetLastError() { return lastErrorMessage; };
	bool GetTranslatedError(QString &error_message);

private:
	QString broadcast_id;

	int lastError;
	QString lastErrorMessage;
	QString lastErrorReason;
};
