#include "moc_youtube-api-wrappers.cpp"

#include <QUrl>
#include <QMimeDatabase>
#include <QFile>

#include <string>
#include <iostream>
#include <qt-wrappers.hpp>

#include "auth-youtube.hpp"
#include "obs-app.hpp"
#include "window-basic-main.hpp"
#include "remote-text.hpp"
#include "ui-config.h"
#include "obf.h"

using namespace json11;

/* ------------------------------------------------------------------------- */
#define YOUTUBE_LIVE_API_URL "https://www.googleapis.com/youtube/v3"

#define YOUTUBE_LIVE_STREAM_URL YOUTUBE_LIVE_API_URL "/liveStreams"
#define YOUTUBE_LIVE_BROADCAST_URL YOUTUBE_LIVE_API_URL "/liveBroadcasts"
#define YOUTUBE_LIVE_BROADCAST_TRANSITION_URL YOUTUBE_LIVE_BROADCAST_URL "/transition"
#define YOUTUBE_LIVE_BROADCAST_BIND_URL YOUTUBE_LIVE_BROADCAST_URL "/bind"

#define YOUTUBE_LIVE_CHANNEL_URL YOUTUBE_LIVE_API_URL "/channels"
#define YOUTUBE_LIVE_TOKEN_URL "https://oauth2.googleapis.com/token"
#define YOUTUBE_LIVE_VIDEOCATEGORIES_URL YOUTUBE_LIVE_API_URL "/videoCategories"
#define YOUTUBE_LIVE_VIDEOS_URL YOUTUBE_LIVE_API_URL "/videos"
#define YOUTUBE_LIVE_THUMBNAIL_URL "https://www.googleapis.com/upload/youtube/v3/thumbnails/set"

#define DEFAULT_BROADCASTS_PER_QUERY "50" // acceptable values are 0 to 50, inclusive
/* ------------------------------------------------------------------------- */

bool IsYouTubeService(const std::string &service)
{
	auto it = find_if(youtubeServices.begin(), youtubeServices.end(),
			  [&service](const Auth::Def &yt) { return service == yt.service; });
	return it != youtubeServices.end();
}
bool IsUserSignedIntoYT()
{
	Auth *auth = OBSBasic::Get()->GetAuth();
	if (auth) {
		YoutubeApiWrappers *apiYouTube(dynamic_cast<YoutubeApiWrappers *>(auth));
		if (apiYouTube) {
			return true;
		}
	}
	return false;
}

bool YoutubeApiWrappers::GetTranslatedError(QString &error_message)
{
	QString translated = QTStr("YouTube.Errors." + lastErrorReason.toUtf8());
	// No translation found
	if (translated.startsWith("YouTube.Errors."))
		return false;
	error_message = translated;
	return true;
}

YoutubeApiWrappers::YoutubeApiWrappers(const Def &d) : YoutubeAuth(d) {}

bool YoutubeApiWrappers::TryInsertCommand(const char *url, const char *content_type, std::string request_type,
					  const char *data, Json &json_out, long *error_code, int data_size)
{
	long httpStatusCode = 0;

#ifdef _DEBUG
	blog(LOG_DEBUG, "YouTube API command URL: %s", url);
	if (data && data[0] == '{') // only log JSON data
		blog(LOG_DEBUG, "YouTube API command data: %s", data);
#endif
	if (token.empty())
		return false;
	std::string output;
	std::string error;
	// Increase timeout by the time it takes to transfer `data_size` at 1 Mbps
	int timeout = 60 + data_size / 125000;
	bool success = GetRemoteFile(url, output, error, &httpStatusCode, content_type, request_type, data,
				     {"Authorization: Bearer " + token}, nullptr, timeout, false, data_size);
	if (error_code)
		*error_code = httpStatusCode;

	if (!success || output.empty()) {
		if (!error.empty())
			blog(LOG_WARNING, "YouTube API request failed: %s", error.c_str());
		return false;
	}

	json_out = Json::parse(output, error);
#ifdef _DEBUG
	blog(LOG_DEBUG, "YouTube API command answer: %s", json_out.dump().c_str());
#endif
	if (!error.empty()) {
		return false;
	}
	return httpStatusCode < 400;
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

	std::string r_token = QUrl::toPercentEncoding(refresh_token.c_str()).toStdString();
	const QString url = YOUTUBE_LIVE_TOKEN_URL;
	const QString data_template = "client_id=%1"
				      "&client_secret=%2"
				      "&refresh_token=%3"
				      "&grant_type=refresh_token";
	const QString data =
		data_template.arg(QString(clientid.c_str()), QString(secret.c_str()), QString(r_token.c_str()));
	Json json_out;
	bool success =
		TryInsertCommand(QT_TO_UTF8(url), "application/x-www-form-urlencoded", "", QT_TO_UTF8(data), json_out);

	if (!success || json_out.object_items().find("error") != json_out.object_items().end())
		return false;
	token = json_out["access_token"].string_value();
	return token.empty() ? false : true;
}

bool YoutubeApiWrappers::InsertCommand(const char *url, const char *content_type, std::string request_type,
				       const char *data, Json &json_out, int data_size)
{
	long error_code;
	bool success = TryInsertCommand(url, content_type, request_type, data, json_out, &error_code, data_size);

	if (error_code == 401) {
		// Attempt to update access token and try again
		if (!UpdateAccessToken())
			return false;
		success = TryInsertCommand(url, content_type, request_type, data, json_out, &error_code, data_size);
	}

	if (json_out.object_items().find("error") != json_out.object_items().end()) {
		blog(LOG_ERROR, "YouTube API error:\n\tHTTP status: %ld\n\tURL: %s\n\tJSON: %s", error_code, url,
		     json_out.dump().c_str());

		lastError = json_out["error"]["code"].int_value();
		lastErrorReason = QString(json_out["error"]["errors"][0]["reason"].string_value().c_str());
		lastErrorMessage = QString(json_out["error"]["message"].string_value().c_str());

		// The existence of an error implies non-success even if the HTTP status code disagrees.
		success = false;
	}
	return success;
}

bool YoutubeApiWrappers::GetChannelDescription(ChannelDescription &channel_description)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
	const QByteArray url = YOUTUBE_LIVE_CHANNEL_URL "?part=snippet,contentDetails,statistics"
							"&mine=true";
	Json json_out;
	if (!InsertCommand(url, "application/json", "", nullptr, json_out)) {
		return false;
	}

	if (json_out["pageInfo"]["totalResults"].int_value() == 0) {
		lastErrorMessage = QTStr("YouTube.Auth.NoChannels");
		return false;
	}

	channel_description.id = QString(json_out["items"][0]["id"].string_value().c_str());
	channel_description.title = QString(json_out["items"][0]["snippet"]["title"].string_value().c_str());
	return channel_description.id.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::InsertBroadcast(BroadcastDescription &broadcast)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
	const QByteArray url = YOUTUBE_LIVE_BROADCAST_URL "?part=snippet,status,contentDetails";
	const Json data = Json::object{
		{"snippet",
		 Json::object{
			 {"title", QT_TO_UTF8(broadcast.title)},
			 {"description", QT_TO_UTF8(broadcast.description)},
			 {"scheduledStartTime", QT_TO_UTF8(broadcast.schedul_date_time)},
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
	if (!InsertCommand(url, "application/json", "", data.dump().c_str(), json_out)) {
		return false;
	}
	broadcast.id = QString(json_out["id"].string_value().c_str());
	return broadcast.id.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::InsertStream(StreamDescription &stream)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
	const QByteArray url = YOUTUBE_LIVE_STREAM_URL "?part=snippet,cdn,status,contentDetails";
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
	if (!InsertCommand(url, "application/json", "", data.dump().c_str(), json_out)) {
		return false;
	}
	stream.id = QString(json_out["id"].string_value().c_str());
	stream.name = QString(json_out["cdn"]["ingestionInfo"]["streamName"].string_value().c_str());
	return stream.id.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::BindStream(const QString broadcast_id, const QString stream_id)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
	const QString url_template = YOUTUBE_LIVE_BROADCAST_BIND_URL "?id=%1"
								     "&streamId=%2"
								     "&part=id,snippet,contentDetails,status";
	const QString url = url_template.arg(broadcast_id, stream_id);
	const Json data = Json::object{};
	this->broadcast_id = broadcast_id;
	Json json_out;
	return InsertCommand(QT_TO_UTF8(url), "application/json", "", data.dump().c_str(), json_out);
}

bool YoutubeApiWrappers::GetBroadcastsList(Json &json_out, const QString &page, const QString &status)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
	QByteArray url = YOUTUBE_LIVE_BROADCAST_URL "?part=snippet,contentDetails,status"
						    "&broadcastType=all&maxResults=" DEFAULT_BROADCASTS_PER_QUERY;

	if (status.isEmpty())
		url += "&mine=true";
	else
		url += "&broadcastStatus=" + status.toUtf8();

	if (!page.isEmpty())
		url += "&pageToken=" + page.toUtf8();
	return InsertCommand(url, "application/json", "", nullptr, json_out);
}

bool YoutubeApiWrappers::GetVideoCategoriesList(QVector<CategoryDescription> &category_list_out)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
	const QString url_template = YOUTUBE_LIVE_VIDEOCATEGORIES_URL "?part=snippet"
								      "&regionCode=%1"
								      "&hl=%2";
	/*
	 * All OBS locale regions aside from "US" are missing category id 29
	 * ("Nonprofits & Activism"), but it is still available to channels
	 * set to those regions via the YouTube Studio website.
	 * To work around this inconsistency with the API all locales will
	 * use the "US" region and only set the language part for localisation.
	 * It is worth noting that none of the regions available on YouTube
	 * feature any category not also available to the "US" region.
	 */
	QString url = url_template.arg("US", QLocale().name());

	Json json_out;
	if (!InsertCommand(QT_TO_UTF8(url), "application/json", "", nullptr, json_out)) {
		if (lastErrorReason != "unsupportedLanguageCode" && lastErrorReason != "invalidLanguage")
			return false;
		// Try again with en-US if YouTube error indicates an unsupported locale
		url = url_template.arg("US", "en_US");
		if (!InsertCommand(QT_TO_UTF8(url), "application/json", "", nullptr, json_out))
			return false;
	}
	category_list_out = {};
	for (auto &j : json_out["items"].array_items()) {
		// Assignable only.
		if (j["snippet"]["assignable"].bool_value()) {
			category_list_out.push_back(
				{j["id"].string_value().c_str(), j["snippet"]["title"].string_value().c_str()});
		}
	}
	return category_list_out.isEmpty() ? false : true;
}

bool YoutubeApiWrappers::SetVideoCategory(const QString &video_id, const QString &video_title,
					  const QString &video_description, const QString &categorie_id)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
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
	return InsertCommand(url, "application/json", "PUT", data.dump().c_str(), json_out);
}

bool YoutubeApiWrappers::SetVideoThumbnail(const QString &video_id, const QString &thumbnail_file)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();

	// Make sure the file hasn't been deleted since originally selecting it
	if (!QFile::exists(thumbnail_file)) {
		lastErrorMessage = QTStr("YouTube.Actions.Error.FileMissing");
		return false;
	}

	QFile thumbFile(thumbnail_file);
	if (!thumbFile.open(QFile::ReadOnly)) {
		lastErrorMessage = QTStr("YouTube.Actions.Error.FileOpeningFailed");
		return false;
	}

	const QByteArray fileContents = thumbFile.readAll();
	const QString mime = QMimeDatabase().mimeTypeForData(fileContents).name();

	const QString url = YOUTUBE_LIVE_THUMBNAIL_URL "?videoId=" + video_id;
	Json json_out;
	return InsertCommand(QT_TO_UTF8(url), QT_TO_UTF8(mime), "POST", fileContents.constData(), json_out,
			     fileContents.size());
}

bool YoutubeApiWrappers::StartBroadcast(const QString &broadcast_id)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();

	Json json_out;
	if (!FindBroadcast(broadcast_id, json_out))
		return false;

	auto lifeCycleStatus = json_out["items"][0]["status"]["lifeCycleStatus"].string_value();

	if (lifeCycleStatus == "live" || lifeCycleStatus == "liveStarting")
		// Broadcast is already (going to be) live
		return true;
	else if (lifeCycleStatus == "testStarting") {
		// User will need to wait a few seconds before attempting to start broadcast
		lastErrorMessage = QTStr("YouTube.Actions.Error.BroadcastTestStarting");
		lastErrorReason.clear();
		return false;
	}

	// Only reset if broadcast has monitoring enabled and is not already in "testing" mode
	auto monitorStreamEnabled =
		json_out["items"][0]["contentDetails"]["monitorStream"]["enableMonitorStream"].bool_value();
	if (lifeCycleStatus != "testing" && monitorStreamEnabled && !ResetBroadcast(broadcast_id, json_out))
		return false;

	const QString url_template = YOUTUBE_LIVE_BROADCAST_TRANSITION_URL "?id=%1"
									   "&broadcastStatus=%2"
									   "&part=status";
	const QString live = url_template.arg(broadcast_id, "live");
	bool success = InsertCommand(QT_TO_UTF8(live), "application/json", "POST", "{}", json_out);
	// Return a success if the command failed, but was redundant (broadcast already live)
	return success || lastErrorReason == "redundantTransition";
}

bool YoutubeApiWrappers::StartLatestBroadcast()
{
	return StartBroadcast(this->broadcast_id);
}

bool YoutubeApiWrappers::StopBroadcast(const QString &broadcast_id)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();

	const QString url_template = YOUTUBE_LIVE_BROADCAST_TRANSITION_URL "?id=%1"
									   "&broadcastStatus=complete"
									   "&part=status";
	const QString url = url_template.arg(broadcast_id);
	Json json_out;
	bool success = InsertCommand(QT_TO_UTF8(url), "application/json", "POST", "{}", json_out);
	// Return a success if the command failed, but was redundant (broadcast already stopped)
	return success || lastErrorReason == "redundantTransition";
}

bool YoutubeApiWrappers::StopLatestBroadcast()
{
	return StopBroadcast(this->broadcast_id);
}

void YoutubeApiWrappers::SetBroadcastId(QString &broadcast_id)
{
	this->broadcast_id = broadcast_id;
}

QString YoutubeApiWrappers::GetBroadcastId()
{
	return this->broadcast_id;
}

bool YoutubeApiWrappers::ResetBroadcast(const QString &broadcast_id, json11::Json &json_out)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();

	auto snippet = json_out["items"][0]["snippet"];
	auto status = json_out["items"][0]["status"];
	auto contentDetails = json_out["items"][0]["contentDetails"];
	auto monitorStream = contentDetails["monitorStream"];

	const Json data = Json::object{
		{"id", QT_TO_UTF8(broadcast_id)},
		{"snippet",
		 Json::object{
			 {"title", snippet["title"]},
			 {"description", snippet["description"]},
			 {"scheduledStartTime", snippet["scheduledStartTime"]},
			 {"scheduledEndTime", snippet["scheduledEndTime"]},
		 }},
		{"status",
		 Json::object{
			 {"privacyStatus", status["privacyStatus"]},
			 {"madeForKids", status["madeForKids"]},
			 {"selfDeclaredMadeForKids", status["selfDeclaredMadeForKids"]},
		 }},
		{"contentDetails",
		 Json::object{
			 {
				 "monitorStream",
				 Json::object{
					 {"enableMonitorStream", false},
					 {"broadcastStreamDelayMs", monitorStream["broadcastStreamDelayMs"]},
				 },
			 },
			 {"enableAutoStart", contentDetails["enableAutoStart"]},
			 {"enableAutoStop", contentDetails["enableAutoStop"]},
			 {"enableClosedCaptions", contentDetails["enableClosedCaptions"]},
			 {"enableDvr", contentDetails["enableDvr"]},
			 {"enableContentEncryption", contentDetails["enableContentEncryption"]},
			 {"enableEmbed", contentDetails["enableEmbed"]},
			 {"recordFromStart", contentDetails["recordFromStart"]},
			 {"startWithSlate", contentDetails["startWithSlate"]},
		 }},
	};

	const QString put = YOUTUBE_LIVE_BROADCAST_URL "?part=id,snippet,contentDetails,status";
	return InsertCommand(QT_TO_UTF8(put), "application/json", "PUT", data.dump().c_str(), json_out);
}

bool YoutubeApiWrappers::FindBroadcast(const QString &id, json11::Json &json_out)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
	QByteArray url = YOUTUBE_LIVE_BROADCAST_URL "?part=id,snippet,contentDetails,status"
						    "&broadcastType=all&maxResults=1";
	url += "&id=" + id.toUtf8();

	if (!InsertCommand(url, "application/json", "", nullptr, json_out))
		return false;

	auto items = json_out["items"].array_items();
	if (items.size() != 1) {
		lastErrorMessage = QTStr("YouTube.Actions.Error.BroadcastNotFound");
		return false;
	}

	return true;
}

bool YoutubeApiWrappers::FindStream(const QString &id, json11::Json &json_out)
{
	lastErrorMessage.clear();
	lastErrorReason.clear();
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
