#pragma once

#include "auth-youtube.hpp"

#include <json11.hpp>
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

bool IsYouTubeService(const std::string &service);
bool IsUserSignedIntoYT();

class YoutubeApiWrappers : public YoutubeAuth {
	Q_OBJECT

	bool TryInsertCommand(const char *url, const char *content_type, std::string request_type, const char *data,
			      json11::Json &ret, long *error_code = nullptr, int data_size = 0);
	bool UpdateAccessToken();
	bool InsertCommand(const char *url, const char *content_type, std::string request_type, const char *data,
			   json11::Json &ret, int data_size = 0);

public:
	YoutubeApiWrappers(const Def &d);

	bool GetChannelDescription(ChannelDescription &channel_description);
	bool InsertBroadcast(BroadcastDescription &broadcast);
	bool InsertStream(StreamDescription &stream);
	bool BindStream(const QString broadcast_id, const QString stream_id);
	bool GetBroadcastsList(json11::Json &json_out, const QString &page, const QString &status);
	bool GetVideoCategoriesList(QVector<CategoryDescription> &category_list_out);
	bool SetVideoCategory(const QString &video_id, const QString &video_title, const QString &video_description,
			      const QString &categorie_id);
	bool SetVideoThumbnail(const QString &video_id, const QString &thumbnail_file);
	bool StartBroadcast(const QString &broadcast_id);
	bool StopBroadcast(const QString &broadcast_id);
	bool ResetBroadcast(const QString &broadcast_id, json11::Json &json_out);
	bool StartLatestBroadcast();
	bool StopLatestBroadcast();

	void SetBroadcastId(QString &broadcast_id);
	QString GetBroadcastId();

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
