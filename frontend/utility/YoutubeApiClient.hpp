#pragma once

#include "YoutubeApiTypes.hpp"

#include <json11.hpp>

#include <functional>
#include <string>

#include <QString>
#include <QVector>

class YoutubeApiClient {
public:
	struct TokenAccess {
		std::function<std::string()> accessToken;
		std::function<std::string()> refreshToken;
		std::function<void(std::string)> setAccessToken;
	};
	explicit YoutubeApiClient(TokenAccess tokenAccess);

	bool GetChannelDescription(ChannelDescription &channel_description);
	bool InsertBroadcast(BroadcastDescription &broadcast);
	bool InsertStream(StreamDescription &stream);
	bool BindStream(const QString broadcastId, const QString stream_id);
	bool GetBroadcastsList(json11::Json &json_out, const QString &page, const QString &status);
	bool GetVideoCategoriesList(QVector<CategoryDescription> &category_list_out);
	bool SetVideoCategory(const QString &video_id, const QString &video_title, const QString &video_description,
			      const QString &categorie_id);
	bool SetVideoThumbnail(const QString &video_id, const QString &thumbnail_file);
	bool StartBroadcast(const QString &broadcastId);
	bool StopBroadcast(const QString &broadcastId);
	bool ResetBroadcast(const QString &broadcastId, json11::Json &json_out);
	bool StartLatestBroadcast();
	bool StopLatestBroadcast();

	void SetBroadcastId(const QString &broadcastId);
	QString GetBroadcastId();

	bool FindBroadcast(const QString &id, json11::Json &json_out);
	bool FindStream(const QString &id, json11::Json &json_out);

	QString GetLastError() const { return lastErrorMessage; };
	bool GetTranslatedError(QString &error_message);

private:
	bool TryInsertCommand(const char *url, const char *content_type, std::string request_type, const char *data,
			      json11::Json &ret, long *error_code = nullptr, int data_size = 0);
	bool UpdateAccessToken();
	bool InsertCommand(const char *url, const char *content_type, std::string request_type, const char *data,
			   json11::Json &ret, int data_size = 0);

private:
	TokenAccess m_tokenAccess;
	QString broadcastId;
	int lastError = 0;
	QString lastErrorMessage;
	QString lastErrorReason;
};