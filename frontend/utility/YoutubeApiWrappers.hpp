#pragma once

#include <oauth/YoutubeAuth.hpp>
#include "YoutubeApiClient.hpp"
#include "YoutubeApiTypes.hpp"

#include <json11.hpp>

#include <QString>

bool IsYouTubeService(const std::string &service);
bool IsUserSignedIntoYT();

class YoutubeApiWrappers : public YoutubeAuth {
	Q_OBJECT

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

	void SetBroadcastId(const QString &broadcast_id);
	QString GetBroadcastId();

	bool FindBroadcast(const QString &id, json11::Json &json_out);
	bool FindStream(const QString &id, json11::Json &json_out);

	QString GetLastError() const;
	bool GetTranslatedError(QString &error_message);

private:
	YoutubeApiClient apiClient;
};
