#include "youtube-api-wrappers.hpp"

#include <QUrl>

#include <string>
#include <iostream>

#include "auth-youtube.hpp"
#include "obs-app.hpp"
#include "qt-wrappers.hpp"
#include "remote-text.hpp"
#include "ui-config.h"
#include "obf.h"

using namespace json11;

/* ------------------------------------------------------------------------- */
#define YOUTUBE_LIVE_API_URL "https://www.googleapis.com/youtube/v3"

#define YOUTUBE_LIVE_STREAM_URL YOUTUBE_LIVE_API_URL "/liveStreams"
#define YOUTUBE_LIVE_BROADCAST_URL YOUTUBE_LIVE_API_URL "/liveBroadcasts"
#define YOUTUBE_LIVE_BROADCAST_TRANSITION_URL \
	YOUTUBE_LIVE_BROADCAST_URL "/transition"
#define YOUTUBE_LIVE_BROADCAST_BIND_URL YOUTUBE_LIVE_BROADCAST_URL "/bind"

#define YOUTUBE_LIVE_CHANNEL_URL YOUTUBE_LIVE_API_URL "/channels"
#define YOUTUBE_LIVE_TOKEN_URL "https://oauth2.googleapis.com/token"
#define YOUTUBE_LIVE_VIDEOCATEGORIES_URL YOUTUBE_LIVE_API_URL "/videoCategories"
#define YOUTUBE_LIVE_VIDEOS_URL YOUTUBE_LIVE_API_URL "/videos"

#define DEFAULT_BROADCASTS_PER_QUERY \
	"7" // acceptable values are 0 to 50, inclusive
/* ------------------------------------------------------------------------- */

bool IsYouTubeService(const std::string &service)
{
	auto it = find_if(youtubeServices.begin(), youtubeServices.end(),
			  [&service](const Auth::Def &yt) {
				  return service == yt.service;
			  });
	return it != youtubeServices.end();
}

YoutubeApiWrappers::YoutubeApiWrappers(const Def &d) : YoutubeAuth(d) {}

bool YoutubeApiWrappers::TryInsertCommand(const char *url,
					  const char *content_type,
					  std::string request_type,
					  const char *data, Json &json_out,
					  long *error_code)
{
	if (error_code)
		*error_code = 0;
#ifdef _DEBUG
	blog(LOG_DEBUG, "YouTube API command URL: %s", url);
	blog(LOG_DEBUG, "YouTube API command data: %s", data);
#endif
	if (token.empty())
		return false;
	std::string output;
	std::string error;
	bool success = GetRemoteFile(url, output, error, error_code,
				     content_type, request_type, data,
				     {"Authorization: Bearer " + token},
				     nullptr, 5);

	if (!success || output.empty())
		return false;
	json_out = Json::parse(output, error);
#ifdef _DEBUG
	blog(LOG_DEBUG, "YouTube API command answer: %s",
	     json_out.dump().c_str());
#endif
	if (!error.empty()) {
		return false;
	}
	return true;
}

bool YoutubeApiWrappers::UpdateAccessToken()
{
	if (refresh_token.empty()) {
		return false;
	}

	std::string clientid = YOUTUBE_CLIENTID;
	std::string secret = YOUTUBE_SECRET;
	deobfuscate_str(&clientid[0], YOUTUBE_CLIENTID_HASH);
	deobfuscate_str(&secret[0], YOUTUBE_SECRET_HASH);

	std::string r_token =
		QUrl::toPercentEncoding(refresh_token.c_str()).toStdString();
	const QString url = YOUTUBE_LIVE_TOKEN_URL;
	const QString data_template = "client_id=%1"
				      "&client_secret=%2"
				      "&refresh_token=%3"
				      "&grant_type=refresh_token";
	const QString data = data_template.arg(QString(clientid.c_str()),
					       QString(secret.c_str()),
					       QString(r_token.c_str()));
	Json json_out;
	bool success = TryInsertCommand(QT_TO_UTF8(url),
					"application/x-www-form-urlencoded", "",
					QT_TO_UTF8(data), json_out);

	if (!success || json_out.object_items().find("error") !=
				json_out.object_items().end())
		return false;
	token = json_out["access_token"].string_value();
	return token.empty() ? false : true;
}

bool YoutubeApiWrappers::InsertCommand(const char *url,
				       const char *content_type,
				       std::string request_type,
				       const char *data, Json &json_out)
{
	long error_code;
	if (!TryInsertCommand(url, content_type, request_type, data, json_out,
			      &error_code)) {
		if (error_code == 401) {
			if (!UpdateAccessToken()) {
				return false;
			}
			//The second try after update token.
			return TryInsertCommand(url, content_type, request_type,
						data, json_out);
		}
		return false;
	}
	if (json_out.object_items().find("error") !=
	    json_out.object_items().end()) {
		lastError = json_out["error"]["code"].int_value();
		lastErrorMessage = QString(
			json_out["error"]["message"].string_value().c_str());
		if (json_out["error"]["code"] == 401) {
			if (!UpdateAccessToken()) {
				return false;
			}
			//The second try after update token.
			return TryInsertCommand(url, content_type, request_type,
						data, json_out);
		}
		return false;
	}
	return true;
}

bool YoutubeApiWrappers::GetChannelDescription(
	ChannelDescription &channel_description)
{
	lastErrorMessage.clear();
	const QByteArray url = YOUTUBE_LIVE_CHANNEL_URL
		"?part=snippet,contentDetails,statistics"
		"&mine=true";
	Json json_out;
	if (!InsertCommand(url, "application/json", "", nullptr, json_out)) {
		return false;
	}
	channel_description.id =
		QString(json_out["items"][0]["id"].string_value().c_str());
	channel_description.country =
		QString(json_out["items"][0]["snippet"]["country"]
				.string_value()
				.c_str());
	channel_description.language =
		QString(json_out["items"][0]["snippet"]["defaultLanguage"]
				.string_value()
				.c_str());
	channel_description.title = QString(
		json_out["items"][0]["snippet"]["title"].string_value().c_str());
	return channel_description.id.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::InsertBroadcast(BroadcastDescription &broadcast)
{
	// Youtube API: The Title property's value must be between 1 and 100 characters long.
	if (broadcast.title.isEmpty() || broadcast.title.length() > 100) {
		blog(LOG_ERROR, "Insert broadcast FAIL: Wrong title.");
		lastErrorMessage = "Broadcast title too long.";
		return false;
	}
	// Youtube API: The property's value can contain up to 5000 characters.
	if (broadcast.description.length() > 5000) {
		blog(LOG_ERROR, "Insert broadcast FAIL: Description too long.");
		lastErrorMessage = "Broadcast description too long.";
		return false;
	}
	const QByteArray url = YOUTUBE_LIVE_BROADCAST_URL
		"?part=snippet,status,contentDetails";
	const Json data = Json::object{
		{"snippet",
		 Json::object{
			 {"title", QT_TO_UTF8(broadcast.title)},
			 {"description", QT_TO_UTF8(broadcast.description)},
			 {"scheduledStartTime",
			  QT_TO_UTF8(broadcast.schedul_date_time)},
		 }},
		{"status",
		 Json::object{
			 {"privacyStatus", QT_TO_UTF8(broadcast.privacy)},
			 {"selfDeclaredMadeForKids", broadcast.made_for_kids},
		 }},
		{"contentDetails",
		 Json::object{
			 {"latencyPreference", QT_TO_UTF8(broadcast.latency)},
			 {"enableAutoStart", broadcast.auto_start},
			 {"enableAutoStop", broadcast.auto_stop},
			 {"enableDvr", broadcast.dvr},
			 {"projection", QT_TO_UTF8(broadcast.projection)},
			 {
				 "monitorStream",
				 Json::object{
					 {"enableMonitorStream", false},
				 },
			 },
		 }},
	};
	Json json_out;
	if (!InsertCommand(url, "application/json", "", data.dump().c_str(),
			   json_out)) {
		return false;
	}
	broadcast.id = QString(json_out["id"].string_value().c_str());
	return broadcast.id.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::InsertStream(StreamDescription &stream)
{
	// Youtube API documentation: The snippet.title property's value in the liveStream resource must be between 1 and 128 characters long.
	if (stream.title.isEmpty() || stream.title.length() > 128) {
		blog(LOG_ERROR, "Insert stream FAIL: wrong argument");
		return false;
	}
	// Youtube API: The snippet.description property's value in the liveStream resource can have up to 10000 characters.
	if (stream.description.length() > 10000) {
		blog(LOG_ERROR, "Insert stream FAIL: Description too long.");
		return false;
	}
	const QByteArray url = YOUTUBE_LIVE_STREAM_URL
		"?part=snippet,cdn,status,contentDetails";
	const Json data = Json::object{
		{"snippet",
		 Json::object{
			 {"title", QT_TO_UTF8(stream.title)},
		 }},
		{"cdn",
		 Json::object{
			 {"frameRate", "variable"},
			 {"ingestionType", "rtmp"},
			 {"resolution", "variable"},
		 }},
		{"contentDetails", Json::object{{"isReusable", false}}},
	};
	Json json_out;
	if (!InsertCommand(url, "application/json", "", data.dump().c_str(),
			   json_out)) {
		return false;
	}
	stream.id = QString(json_out["id"].string_value().c_str());
	stream.name = QString(json_out["cdn"]["ingestionInfo"]["streamName"]
				      .string_value()
				      .c_str());
	return stream.id.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::BindStream(const QString broadcast_id,
				    const QString stream_id)
{
	const QString url_template = YOUTUBE_LIVE_BROADCAST_BIND_URL
		"?id=%1"
		"&streamId=%2"
		"&part=id,snippet,contentDetails,status";
	const QString url = url_template.arg(broadcast_id, stream_id);
	const Json data = Json::object{};
	this->broadcast_id = broadcast_id;
	Json json_out;
	return InsertCommand(QT_TO_UTF8(url), "application/json", "",
			     data.dump().c_str(), json_out);
}

bool YoutubeApiWrappers::GetBroadcastsList(Json &json_out, QString page)
{
	lastErrorMessage.clear();
	QByteArray url = YOUTUBE_LIVE_BROADCAST_URL
		"?part=snippet,contentDetails,status"
		"&broadcastType=all&maxResults=" DEFAULT_BROADCASTS_PER_QUERY
		"&mine=true";
	if (!page.isEmpty())
		url += "&pageToken=" + page.toUtf8();
	return InsertCommand(url, "application/json", "", nullptr, json_out);
}

bool YoutubeApiWrappers::GetVideoCategoriesList(
	const QString &country, const QString &language,
	QVector<CategoryDescription> &category_list_out)
{
	const QString url_template = YOUTUBE_LIVE_VIDEOCATEGORIES_URL
		"?part=snippet"
		"&regionCode=%1"
		"&hl=%2";
	const QString url =
		url_template.arg(country.isEmpty() ? "US" : country,
				 language.isEmpty() ? "en" : language);
	Json json_out;
	if (!InsertCommand(QT_TO_UTF8(url), "application/json", "", nullptr,
			   json_out)) {
		return false;
	}
	category_list_out = {};
	for (auto &j : json_out["items"].array_items()) {
		// Assignable only.
		if (j["snippet"]["assignable"].bool_value()) {
			category_list_out.push_back(
				{j["id"].string_value().c_str(),
				 j["snippet"]["title"].string_value().c_str()});
		}
	}
	return category_list_out.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::SetVideoCategory(const QString &video_id,
					  const QString &video_title,
					  const QString &video_description,
					  const QString &categorie_id)
{
	const QByteArray url = YOUTUBE_LIVE_VIDEOS_URL "?part=snippet";
	const Json data = Json::object{
		{"id", QT_TO_UTF8(video_id)},
		{"snippet",
		 Json::object{
			 {"title", QT_TO_UTF8(video_title)},
			 {"description", QT_TO_UTF8(video_description)},
			 {"categoryId", QT_TO_UTF8(categorie_id)},
		 }},
	};
	Json json_out;
	return InsertCommand(url, "application/json", "PUT",
			     data.dump().c_str(), json_out);
}

bool YoutubeApiWrappers::StartBroadcast(const QString &broadcast_id)
{
	lastErrorMessage.clear();

	if (!ResetBroadcast(broadcast_id))
		return false;

	const QString url_template = YOUTUBE_LIVE_BROADCAST_TRANSITION_URL
		"?id=%1"
		"&broadcastStatus=%2"
		"&part=status";
	const QString live = url_template.arg(broadcast_id, "live");
	Json json_out;
	return InsertCommand(QT_TO_UTF8(live), "application/json", "POST", "{}",
			     json_out);
}

bool YoutubeApiWrappers::StartLatestBroadcast()
{
	return StartBroadcast(this->broadcast_id);
}

bool YoutubeApiWrappers::StopBroadcast(const QString &broadcast_id)
{
	lastErrorMessage.clear();

	const QString url_template = YOUTUBE_LIVE_BROADCAST_TRANSITION_URL
		"?id=%1"
		"&broadcastStatus=complete"
		"&part=status";
	const QString url = url_template.arg(broadcast_id);
	Json json_out;
	return InsertCommand(QT_TO_UTF8(url), "application/json", "POST", "{}",
			     json_out);
}

bool YoutubeApiWrappers::StopLatestBroadcast()
{
	return StopBroadcast(this->broadcast_id);
}

bool YoutubeApiWrappers::ResetBroadcast(const QString &broadcast_id)
{
	lastErrorMessage.clear();

	const QString url_template = YOUTUBE_LIVE_BROADCAST_URL
		"?part=id,snippet,contentDetails,status"
		"&id=%1";
	const QString url = url_template.arg(broadcast_id);
	Json json_out;

	if (!InsertCommand(QT_TO_UTF8(url), "application/json", "", nullptr,
			   json_out))
		return false;

	const QString put = YOUTUBE_LIVE_BROADCAST_URL
		"?part=id,snippet,contentDetails,status";

	auto snippet = json_out["items"][0]["snippet"];
	auto status = json_out["items"][0]["status"];
	auto contentDetails = json_out["items"][0]["contentDetails"];
	auto monitorStream = contentDetails["monitorStream"];

	const Json data = Json::object{
		{"id", QT_TO_UTF8(broadcast_id)},
		{"snippet",
		 Json::object{
			 {"title", snippet["title"]},
			 {"scheduledStartTime", snippet["scheduledStartTime"]},
		 }},
		{"status",
		 Json::object{
			 {"privacyStatus", status["privacyStatus"]},
			 {"madeForKids", status["madeForKids"]},
			 {"selfDeclaredMadeForKids",
			  status["selfDeclaredMadeForKids"]},
		 }},
		{"contentDetails",
		 Json::object{
			 {
				 "monitorStream",
				 Json::object{
					 {"enableMonitorStream", false},
					 {"broadcastStreamDelayMs",
					  monitorStream["broadcastStreamDelayMs"]},
				 },
			 },
			 {"enableDvr", contentDetails["enableDvr"]},
			 {"enableContentEncryption",
			  contentDetails["enableContentEncryption"]},
			 {"enableEmbed", contentDetails["enableEmbed"]},
			 {"recordFromStart", contentDetails["recordFromStart"]},
			 {"startWithSlate", contentDetails["startWithSlate"]},
		 }},
	};
	return InsertCommand(QT_TO_UTF8(put), "application/json", "PUT",
			     data.dump().c_str(), json_out);
}

bool YoutubeApiWrappers::FindBroadcast(const QString &id,
				       json11::Json &json_out)
{
	lastErrorMessage.clear();
	QByteArray url = YOUTUBE_LIVE_BROADCAST_URL
		"?part=id,snippet,contentDetails,status"
		"&broadcastType=all&maxResults=1";
	url += "&id=" + id.toUtf8();

	if (!InsertCommand(url, "application/json", "", nullptr, json_out))
		return false;

	auto items = json_out["items"].array_items();
	if (items.size() != 1) {
		lastErrorMessage = "No active broadcast found.";
		return false;
	}

	return true;
}

bool YoutubeApiWrappers::FindStream(const QString &id, json11::Json &json_out)
{
	lastErrorMessage.clear();
	QByteArray url = YOUTUBE_LIVE_STREAM_URL "?part=id,snippet,cdn,status"
						 "&maxResults=1";
	url += "&id=" + id.toUtf8();

	if (!InsertCommand(url, "application/json", "", nullptr, json_out))
		return false;

	auto items = json_out["items"].array_items();
	if (items.size() != 1) {
		lastErrorMessage = "No active broadcast found.";
		return false;
	}

	return true;
}
