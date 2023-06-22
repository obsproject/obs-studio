// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "youtube-oauth.hpp"

#include <QPushButton>

#include <obf.h>
#include <oauth-local-redirect.hpp>
#include <obs.hpp>

#include "window-youtube-actions.hpp"
#include "youtube-chat.hpp"

constexpr const char *AUTH_URL = "https://accounts.google.com/o/oauth2/v2/auth";
constexpr const char *TOKEN_URL = "https://www.googleapis.com/oauth2/v4/token";
constexpr const char *API_URL = "https://www.googleapis.com/youtube/v3";
constexpr const char *LIVE_BROADCASTS_API_ENDPOINT = "/liveBroadcasts";
constexpr const char *LIVE_STREAMS_API_ENDPOINT = "/liveStreams";

constexpr const char *CLIENT_ID = (const char *)YOUTUBE_CLIENTID;
constexpr uint64_t CLIENT_ID_HASH = YOUTUBE_CLIENTID_HASH;

constexpr const char *CLIENT_SECRET = (const char *)YOUTUBE_SECRET;
constexpr uint64_t CLIENT_SECRET_HASH = YOUTUBE_SECRET_HASH;

constexpr const char *SCOPE = "https://www.googleapis.com/auth/youtube";
constexpr int64_t SCOPE_VERSION = 1;

constexpr const char *CHAT_DOCK_NAME = "ytChat";

namespace YouTubeApi {

ServiceOAuth::ServiceOAuth() : OAuth::ServiceBase()
{
	broadcastFlow.priv = this;
	broadcastFlow.flags =
		OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_START |
		OBS_BROADCAST_FLOW_ALLOW_DIFFERED_BROADCAST_STOP;
	broadcastFlow.get_broadcast_state = GetBroadcastState;
	broadcastFlow.get_broadcast_start_type = GetBroadcastStartType;
	broadcastFlow.get_broadcast_stop_type = GetBroadcastStopType;
	broadcastFlow.manage_broadcast = ManageBroadcast;
	broadcastFlow.stopped_streaming = StoppedStreaming;
	broadcastFlow.differed_start_broadcast = DifferedStartBroadcast;
	broadcastFlow.is_broadcast_stream_active = IsBroadcastStreamActive;
	broadcastFlow.differed_stop_broadcast = DifferedStopBroadcast;
	broadcastFlow.get_last_error = GetBroadcastsLastError;

	broadcastState = OBS_BROADCAST_NONE;
	broadcastStartType = OBS_BROADCAST_START_WITH_STREAM;
	broadcastStopType = OBS_BROADCAST_STOP_WITH_STREAM;
}

std::string ServiceOAuth::UserAgent()
{
	std::string userAgent("obs-youtube ");
	userAgent += obs_get_version_string();

	return userAgent;
}

const char *ServiceOAuth::TokenUrl()
{
	return TOKEN_URL;
}

std::string ServiceOAuth::ClientId()
{
	std::string clientId = CLIENT_ID;
	deobfuscate_str(&clientId[0], CLIENT_ID_HASH);

	return clientId;
}

std::string ServiceOAuth::ClientSecret()
{
	std::string clientSecret = CLIENT_SECRET;
	deobfuscate_str(&clientSecret[0], CLIENT_SECRET_HASH);

	return clientSecret;
}

int64_t ServiceOAuth::ScopeVersion()
{
	return SCOPE_VERSION;
}

static inline void WarningBox(const QString &title, const QString &text,
			      QWidget *parent)
{
	QMessageBox warn(QMessageBox::Warning, title, text,
			 QMessageBox::NoButton, parent);
	QPushButton *ok = warn.addButton(QMessageBox::Ok);
	ok->setText(ok->tr("Ok"));
	warn.exec();
}

static int LoginConfirmation(const OAuth::LoginReason &reason, QWidget *parent)
{
	const char *reasonText = nullptr;
	switch (reason) {
	case OAuth::LoginReason::CONNECT:
		return QMessageBox::Ok;
	case OAuth::LoginReason::SCOPE_CHANGE:
		reasonText = obs_module_text(
			"YouTube.Auth.ReLoginDialog.ScopeChange");
		break;
	case OAuth::LoginReason::REFRESH_TOKEN_FAILED:
		reasonText = obs_module_text(
			"YouTube.Auth.ReLoginDialog.RefreshTokenFailed");
		break;
	case OAuth::LoginReason::PROFILE_DUPLICATION:
		reasonText = obs_module_text(
			"YouTube.Auth.ReLoginDialog.ProfileDuplication");
		break;
	}

	QString title = QString::fromUtf8(
		obs_module_text("YouTube.Auth.ReLoginDialog.Title"), -1);
	QString text =
		QString::fromUtf8(
			obs_module_text("YouTube.Auth.ReLoginDialog.Text"))
			.arg(reasonText);

	QMessageBox ques(QMessageBox::Warning, title, text,
			 QMessageBox::NoButton, parent);
	QPushButton *button = ques.addButton(QMessageBox::Yes);
	button->setText(button->tr("Yes"));
	button = ques.addButton(QMessageBox::No);
	button->setText(button->tr("No"));

	return ques.exec();
}

bool ServiceOAuth::LoginInternal(const OAuth::LoginReason &reason,
				 std::string &code, std::string &redirectUri)
{
	QWidget *parent =
		reinterpret_cast<QWidget *>(obs_frontend_get_main_window());

	if (reason != OAuth::LoginReason::CONNECT &&
	    LoginConfirmation(reason, parent) != QMessageBox::Ok) {
		return false;
	}

	LocalRedirect login(AUTH_URL, ClientId().c_str(), SCOPE, true, parent);

	if (login.exec() != QDialog::Accepted) {
		QString error = login.GetLastError();

		if (!error.isEmpty()) {
			blog(LOG_ERROR, "[%s][%s]: %s", PluginLogName(),
			     __FUNCTION__, error.toUtf8().constData());

			QString title = QString::fromUtf8(
				obs_module_text(
					"YouTube.Auth.LoginError.Title"),
				-1);
			QString text =
				QString::fromUtf8(
					obs_module_text(
						"YouTube.Auth.LoginError.Text"),
					-1)
					.arg(error);

			WarningBox(title, text, parent);
		}

		return false;
	}

	code = login.GetCode().toStdString();
	redirectUri = login.GetRedirectUri().toStdString();

	return true;
}

bool ServiceOAuth::SignOutInternal()
{
	QWidget *parent =
		reinterpret_cast<QWidget *>(obs_frontend_get_main_window());

	QString title = QString::fromUtf8(
		obs_module_text("YouTube.Auth.SignOutDialog.Title"), -1);
	QString text = QString::fromUtf8(
		obs_module_text("YouTube.Auth.SignOutDialog.Text"), -1);

	QMessageBox ques(QMessageBox::Question, title, text,
			 QMessageBox::NoButton, parent);
	QPushButton *button = ques.addButton(QMessageBox::Yes);
	button->setText(button->tr("Yes"));
	button = ques.addButton(QMessageBox::No);
	button->setText(button->tr("No"));

	return ques.exec() == QMessageBox::Yes;
}

void ServiceOAuth::SetSettings(obs_data_t *data)
{
	uiSettings.rememberSettings = obs_data_get_bool(data, "remember");

	if (!uiSettings.rememberSettings)
		return;

	uiSettings.title = obs_data_get_string(data, "title");
	uiSettings.description = obs_data_get_string(data, "description");
	uiSettings.privacy =
		(YouTubeApi::LiveBroadcastPrivacyStatus)obs_data_get_int(
			data, "privacy");
	uiSettings.categoryId = obs_data_get_string(data, "category_id");
	uiSettings.latency =
		(YouTubeApi::LiveBroadcastLatencyPreference)obs_data_get_int(
			data, "latency");
	uiSettings.madeForKids = obs_data_get_bool(data, "made_for_kids");
	uiSettings.autoStart = obs_data_get_bool(data, "auto_start");
	uiSettings.autoStop = obs_data_get_bool(data, "auto_stop");
	uiSettings.dvr = obs_data_get_bool(data, "dvr");
	uiSettings.scheduleForLater =
		obs_data_get_bool(data, "schedule_for_later");
	uiSettings.projection =
		(YouTubeApi::LiveBroadcastProjection)obs_data_get_int(
			data, "projection");
	uiSettings.thumbnailFile = obs_data_get_string(data, "thumbnail_file");
}

obs_data_t *ServiceOAuth::GetSettingsData()
{
	OBSData data = obs_data_create();

	obs_data_set_bool(data, "remember", uiSettings.rememberSettings);

	if (!uiSettings.rememberSettings)
		return data;

	obs_data_set_string(data, "title", uiSettings.title.c_str());
	obs_data_set_string(data, "description",
			    uiSettings.description.c_str());
	obs_data_set_int(data, "privacy", (int)uiSettings.privacy);
	obs_data_set_string(data, "category_id", uiSettings.categoryId.c_str());
	obs_data_set_int(data, "latency", (int)uiSettings.latency);
	obs_data_set_bool(data, "made_for_kids", uiSettings.madeForKids);
	obs_data_set_bool(data, "auto_start", uiSettings.autoStart);
	obs_data_set_bool(data, "auto_stop", uiSettings.autoStop);
	obs_data_set_bool(data, "dvr", uiSettings.dvr);
	obs_data_set_bool(data, "schedule_for_later",
			  uiSettings.scheduleForLater);
	obs_data_set_int(data, "projection", (int)uiSettings.projection);
	obs_data_set_string(data, "thumbnail_file",
			    uiSettings.thumbnailFile.c_str());

	return data;
}

void ServiceOAuth::LoadFrontendInternal()
{
	if (!obs_frontend_is_browser_available())
		return;

	if (chat) {
		blog(LOG_ERROR, "[%s][%s] The chat was not unloaded",
		     PluginLogName(), __FUNCTION__);
		return;
	}

	chat = new YoutubeChat(this);
	obs_frontend_add_dock_by_id(CHAT_DOCK_NAME,
				    obs_module_text("YouTube.Chat.Dock"), chat);
}

void ServiceOAuth::UnloadFrontendInternal()
{
	if (!obs_frontend_is_browser_available())
		return;

	obs_frontend_remove_dock(CHAT_DOCK_NAME);

	chat = nullptr;
}

void ServiceOAuth::AddBondedServiceFrontend(obs_service_t *service)
{
	obs_frontend_add_broadcast_flow(service, &broadcastFlow);
}

void ServiceOAuth::RemoveBondedServiceFrontend(obs_service_t *service)
{
	obs_frontend_remove_broadcast_flow(service);
}

void ServiceOAuth::LoginError(RequestError &error)
{
	QWidget *parent =
		reinterpret_cast<QWidget *>(obs_frontend_get_main_window());
	QString title = QString::fromUtf8(
		obs_module_text("YouTube.Auth.LoginError.Title"), -1);
	QString text =
		QString::fromUtf8(
			obs_module_text("YouTube.Auth.LoginError.Text2"), -1)
			.arg(QString::fromStdString(error.message))
			.arg(QString::fromStdString(error.error));

	WarningBox(title, text, parent);
}

obs_broadcast_state ServiceOAuth::GetBroadcastState(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);

	return self->broadcastState;
}

obs_broadcast_start ServiceOAuth::GetBroadcastStartType(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);

	return self->broadcastStartType;
}

obs_broadcast_stop ServiceOAuth::GetBroadcastStopType(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);

	return self->broadcastStopType;
}

void ServiceOAuth::ManageBroadcast(void *priv)
{
	obs_frontend_push_ui_translation(obs_module_get_string);
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);
	QWidget *parent =
		reinterpret_cast<QWidget *>(obs_frontend_get_main_window());

	OBSYoutubeActions dialog(parent, self, &self->uiSettings, false);
	dialog.exec();
	obs_frontend_pop_ui_translation();
}

void ServiceOAuth::StoppedStreaming(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);
	if (self->broadcastState != OBS_BROADCAST_ACTIVE &&
	    self->broadcastStartType ==
		    OBS_BROADCAST_START_DIFFER_FROM_STREAM) {
		self->broadcastState = OBS_BROADCAST_NONE;
	}

	if (self->broadcastStopType == OBS_BROADCAST_STOP_WITH_STREAM) {
		self->broadcastState = OBS_BROADCAST_NONE;
	}
}

static std::string GetTranslatedError(const RequestError &error)
{
	if (error.type != RequestErrorType::UNKNOWN_OR_CUSTOM ||
	    error.error.empty())
		return error.message;

	std::string lookupString = "YouTube.Errors.";
	lookupString += error.error;

	return obs_module_text(lookupString.c_str());
}

void ServiceOAuth::DifferedStartBroadcast(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);

	LiveBroadcast broadcast;
	if (!self->FindLiveBroadcast(self->broadcastId, broadcast)) {
		self->broadcastLastError = self->lastError.message;
		return;
	}

	// Broadcast is already (going to be) live
	if (broadcast.status.lifeCycleStatus ==
		    LiveBroadcastLifeCycleStatus::LIVE ||
	    broadcast.status.lifeCycleStatus ==
		    LiveBroadcastLifeCycleStatus::LIVE_STARTING) {
		self->broadcastState = OBS_BROADCAST_ACTIVE;
		return;
	}

	if (broadcast.status.lifeCycleStatus ==
	    LiveBroadcastLifeCycleStatus::TEST_STARTING) {
		self->broadcastLastError = obs_module_text(
			"YouTube.Actions.Error.BroadcastTestStarting");
		return;
	}

	// Only reset if broadcast has monitoring enabled and is not already in "testing" mode
	if (broadcast.status.lifeCycleStatus !=
		    LiveBroadcastLifeCycleStatus::TESTING &&
	    broadcast.contentDetails.monitorStream.enableMonitorStream) {
		broadcast.contentDetails.monitorStream.enableMonitorStream =
			false;
		if (!self->UpdateLiveBroadcast(broadcast)) {
			self->broadcastLastError =
				GetTranslatedError(self->lastError);
			return;
		}
	}

	if (self->TransitionLiveBroadcast(
		    self->broadcastId, LiveBroadcastTransitionStatus::LIVE)) {
		self->broadcastState = OBS_BROADCAST_ACTIVE;
		return;
	}

	self->broadcastLastError = GetTranslatedError(self->lastError);
	// Return a success if the command failed, but was redundant (broadcast already live)
	if (self->lastError.error == "redundantTransition")
		self->broadcastState = OBS_BROADCAST_ACTIVE;
}

obs_broadcast_stream_state ServiceOAuth::IsBroadcastStreamActive(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);

	LiveStream stream;
	if (!self->FindLiveStream(self->liveStreamId, stream))
		return OBS_BROADCAST_STREAM_FAILURE;

	if (stream.status.streamStatus == LiveStreamStatusEnum::ACTIVE)
		return OBS_BROADCAST_STREAM_ACTIVE;

	return OBS_BROADCAST_STREAM_INACTIVE;
}

bool ServiceOAuth::DifferedStopBroadcast(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);

	self->broadcastState = OBS_BROADCAST_NONE;
	if (self->TransitionLiveBroadcast(
		    self->broadcastId, LiveBroadcastTransitionStatus::COMPLETE))
		return true;

	self->broadcastLastError = GetTranslatedError(self->lastError);
	// Return a success if the command failed, but was redundant (broadcast already stopped)
	return self->lastError.error == "redundantTransition";
}

const char *ServiceOAuth::GetBroadcastsLastError(void *priv)
{
	ServiceOAuth *self = reinterpret_cast<ServiceOAuth *>(priv);

	if (self->broadcastLastError.empty())
		return nullptr;

	return self->broadcastLastError.c_str();
}

void ServiceOAuth::SetNewStream(const std::string &id, const std::string &key,
				bool autoStart, bool autoStop, bool startNow)
{
	liveStreamId = id;
	streamKey = key;

	broadcastState = autoStart ? OBS_BROADCAST_ACTIVE
				   : OBS_BROADCAST_INACTIVE;

	if (autoStart) {
		broadcastStartType =
			startNow ? OBS_BROADCAST_START_WITH_STREAM_NOW
				 : OBS_BROADCAST_START_WITH_STREAM;
	} else {
		broadcastStartType = OBS_BROADCAST_START_DIFFER_FROM_STREAM;
	}
	broadcastStopType = autoStop ? OBS_BROADCAST_STOP_WITH_STREAM
				     : OBS_BROADCAST_STOP_DIFFER_FROM_STREAM;
}

void ServiceOAuth::SetChatIds(const std::string &broadcastId,
			      const std::string &chatId)
{
	if (!chat)
		return;

	chat->SetChatIds(broadcastId, chatId);
}

void ServiceOAuth::ResetChat()
{
	if (!chat)
		return;

	chat->ResetIds();
}

static inline int Timeout(int dataSize = 0)
{
	return 5 + dataSize / 125000;
}

bool ServiceOAuth::WrappedGetRemoteFile(const char *url, std::string &str,
					long *responseCode,
					const char *contentType,
					std::string request_type,
					const char *postData, int postDataSize)
{
	std::string error;
	CURLcode curlCode =
		GetRemoteFile(UserAgent().c_str(), url, str, error,
			      responseCode, contentType, request_type, postData,
			      {"Authorization: Bearer " + AccessToken()},
			      nullptr, Timeout(postDataSize), postDataSize);

	if (curlCode != CURLE_OK && curlCode != CURLE_HTTP_RETURNED_ERROR) {
		lastError = RequestError(RequestErrorType::CURL_REQUEST_FAILED,
					 error);
		return false;
	}

	if (*responseCode == 200)
		return true;

	try {
		ErrorResponse errorResponse = nlohmann::json::parse(str);
		lastError = RequestError(errorResponse.error.message,
					 errorResponse.error.errors[0].reason);

	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(
			RequestErrorType::ERROR_JSON_PARSING_FAILED, e.what());
	}

	return false;
}

bool ServiceOAuth::TryGetRemoteFile(const char *funcName, const char *url,
				    std::string &str, const char *contentType,
				    std::string request_type,
				    const char *postData, int postDataSize)
{
	long responseCode = 0;
	if (WrappedGetRemoteFile(url, str, &responseCode, contentType,
				 request_type, postData, postDataSize)) {
		return true;
	}

	if (lastError.type == RequestErrorType::CURL_REQUEST_FAILED) {
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(), funcName,
		     lastError.message.c_str(), lastError.error.c_str());
	} else if (lastError.type ==
		   RequestErrorType::ERROR_JSON_PARSING_FAILED) {
		blog(LOG_ERROR, "[%s][%s] HTTP status: %ld\n%s",
		     PluginLogName(), funcName, responseCode, str.c_str());
	} else {
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(), funcName,
		     lastError.error.c_str(), lastError.message.c_str());
	}

	if (responseCode != 401)
		return false;

	lastError = RequestError();
	if (!RefreshAccessToken(lastError))
		return false;

	return WrappedGetRemoteFile(url, str, &responseCode, contentType,
				    request_type, postData, postDataSize);
}

void ServiceOAuth::CheckIfSuccessRequestIsError(const char *funcName,
						const std::string &output)
{
	try {
		ErrorResponse errorResponse = nlohmann::json::parse(output);
		lastError = RequestError(errorResponse.error.message,
					 errorResponse.error.errors[0].reason);
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(), funcName,
		     lastError.message.c_str(), lastError.error.c_str());

	} catch (nlohmann::json::exception &e) {
		RequestError error(RequestErrorType::ERROR_JSON_PARSING_FAILED,
				   e.what());
		blog(LOG_DEBUG, "[%s][%s] %s: %s", PluginLogName(), funcName,
		     error.message.c_str(), error.error.c_str());
	}
}

bool ServiceOAuth::GetChannelInfo(ChannelInfo &info)
{
	if (!(channelInfo.id.empty() || channelInfo.title.empty())) {
		info = channelInfo;
		return true;
	}

	std::string url = API_URL;
	url += "/channels?part=snippet&mine=true";

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "")) {
		blog(LOG_ERROR, "[%s][%s] Failed to retrieve channel info",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	ChannelListSnippetMineResponse response;
	try {
		response = nlohmann::json::parse(output);

		if (response.items.empty()) {
			lastError =
				RequestError("No channel found", "NoChannels");
			blog(LOG_ERROR, "[%s][%s] %s", PluginLogName(),
			     __FUNCTION__, lastError.message.c_str());
			return false;
		}

		channelInfo = ChannelInfo(response.items[0].id,
					  response.items[0].snippet.title);
		info = channelInfo;
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::GetVideoCategoriesList(std::vector<VideoCategory> &list,
					  bool forceUS)
{
	if (!videoCategoryList.empty()) {
		list = videoCategoryList;
		return true;
	}

	std::string url = API_URL;
	std::string locale;
	if (forceUS) {
		locale = "en_US";
	} else {
		locale = obs_get_locale();
		locale.replace(locale.find("-"), 1, "_");
	}
	/*
	 * All OBS locale regions aside from "US" are missing category id 29
	 * ("Nonprofits & Activism"), but it is still available to channels
	 * set to those regions via the YouTube Studio website.
	 * To work around this inconsistency with the API all locales will
	 * use the "US" region and only set the language part for localisation.
	 * It is worth noting that none of the regions available on YouTube
	 * feature any category not also available to the "US" region.
	 */
	url += "/videoCategories?part=snippet&regionCode=US&hl=" + locale;

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "")) {
		blog(LOG_ERROR,
		     "[%s][%s] Failed to retrieve video category list with %s locale",
		     PluginLogName(), __FUNCTION__, locale.c_str());

		return forceUS ? false : GetVideoCategoriesList(list, true);
	}

	VideoCategoryListResponse response;
	try {
		response = nlohmann::json::parse(output);

		if (response.items.empty()) {
			lastError = RequestError("No categories found", "");
			blog(LOG_ERROR, "[%s][%s] %s", PluginLogName(),
			     __FUNCTION__, lastError.message.c_str());
			return false;
		}

		videoCategoryList = response.items;
		list = videoCategoryList;
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::GetLiveBroadcastsList(const LiveBroadcastListStatus &status,
					 std::vector<LiveBroadcast> &list)
{
	std::string statusStr;
	switch (status) {
	case LiveBroadcastListStatus::ACTIVE:
		statusStr = "active";
		break;
	case LiveBroadcastListStatus::ALL:
		statusStr = "all";
		break;
	case LiveBroadcastListStatus::COMPLETED:
		statusStr = "completed";
		break;
	case LiveBroadcastListStatus::UPCOMMING:
		statusStr = "upcoming";
		break;
	}

	std::string baseUrl = API_URL;
	baseUrl += LIVE_BROADCASTS_API_ENDPOINT;
	/* 50 is the biggest accepted value for 'maxResults' */
	baseUrl +=
		"?part=snippet,contentDetails,status&broadcastType=all&maxResults=50&broadcastStatus=" +
		statusStr;

	std::string output;
	lastError = RequestError();

	bool hasPage = true;
	std::string url = baseUrl;
	while (hasPage) {
		if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
				      "application/json", "")) {
			blog(LOG_ERROR,
			     "[%s][%s] Failed to retrieve live broadcast list with status \"%s\"",
			     PluginLogName(), __FUNCTION__, statusStr.c_str());

			return false;
		}

		LiveBroadcastListResponse response;
		try {
			response = nlohmann::json::parse(output);

			if (response.items.empty()) {
				lastError = RequestError(
					"No live broadcast found with status \"" +
						statusStr + "\"",
					"");
				blog(LOG_DEBUG, "[%s][%s] %s", PluginLogName(),
				     __FUNCTION__, lastError.message.c_str());
				return true;
			}

			std::copy(response.items.begin(), response.items.end(),
				  std::back_inserter(list));

			hasPage = response.nextPageToken.has_value();
			if (!hasPage)
				return true;

			url = baseUrl;
			url += "&pageToken=" + response.nextPageToken.value();

		} catch (nlohmann::json::exception &e) {
			lastError = RequestError(
				RequestErrorType::JSON_PARSING_FAILED,
				e.what());
			blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
			     __FUNCTION__, lastError.message.c_str(),
			     lastError.error.c_str());
			break;
		}
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::FindLiveStream(const std::string &id, LiveStream &stream)
{
	std::string url = API_URL;
	url += LIVE_STREAMS_API_ENDPOINT;
	url += "?part=id,snippet,cdn,status&maxResults=1&id=" + id;

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "")) {
		blog(LOG_ERROR, "[%s][%s] Failed to retrieve channel info",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	LiveStreamListResponse response;
	try {
		response = nlohmann::json::parse(output);

		if (response.items.empty()) {
			lastError = RequestError("No stream found", "");
			blog(LOG_ERROR, "[%s][%s] %s", PluginLogName(),
			     __FUNCTION__, lastError.message.c_str());
			return false;
		}

		stream = response.items[0];
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::InsertLiveBroadcast(LiveBroadcast &broadcast)
{
	std::string url = API_URL;
	url += LIVE_BROADCASTS_API_ENDPOINT;
	url += "?part=snippet,status,contentDetails";

	nlohmann::json json;
	try {
		json = broadcast;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
		return false;
	}

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "", json.dump().c_str())) {
		blog(LOG_ERROR, "[%s][%s] Failed to insert broadcast",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	try {
		broadcast = nlohmann::json::parse(output);
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::UpdateVideo(Video &video)
{
	std::string url = API_URL;
	url += "/videos?part=snippet";

	nlohmann::json json;
	try {
		json = video;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
		return false;
	}

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "PUT", json.dump().c_str())) {
		blog(LOG_ERROR, "[%s][%s] Failed to update video info",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	try {
		video = nlohmann::json::parse(output);
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::SetThumbnail(const std::string &id, const char *mimeType,
				const char *thumbnailData, int thumbnailSize)
{
	std::string url =
		"https://www.googleapis.com/upload/youtube/v3/thumbnails/set?videoId=" +
		id;

	std::string output;
	lastError = RequestError();
	if (TryGetRemoteFile(__FUNCTION__, url.c_str(), output, mimeType,
			     "POST", thumbnailData, thumbnailSize)) {
		return true;
	}

	blog(LOG_ERROR, "[%s][%s] Failed to upload thumbnail", PluginLogName(),
	     __FUNCTION__);

	return false;
}

bool ServiceOAuth::InsertLiveStream(LiveStream &stream)
{
	std::string url = API_URL;
	url += LIVE_STREAMS_API_ENDPOINT;
	url += "?part=snippet,cdn,status,contentDetails";

	nlohmann::json json;
	try {
		json = stream;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
		return false;
	}

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "", json.dump().c_str())) {
		blog(LOG_ERROR, "[%s][%s] Failed to insert stream",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	try {
		stream = nlohmann::json::parse(output);
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::BindLiveBroadcast(LiveBroadcast &broadcast,
				     const std::string &streamId)
{
	std::string url = API_URL;
	url += LIVE_BROADCASTS_API_ENDPOINT;
	url += "/bind?id=" + broadcast.id;
	if (!streamId.empty())
		url += "&streamId=" + streamId;
	url += "&part=id,snippet,contentDetails,status";

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "", "{}")) {
		blog(LOG_ERROR, "[%s][%s] Failed to %s broadcast",
		     PluginLogName(), __FUNCTION__,
		     streamId.empty() ? "unbind" : "bind");
		return false;
	}

	try {
		broadcast = nlohmann::json::parse(output);
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::FindLiveBroadcast(const std::string &id,
				     LiveBroadcast &broadcast)
{
	std::string url = API_URL;
	url += LIVE_BROADCASTS_API_ENDPOINT;
	url += "?part=id,snippet,contentDetails,status&broadcastType=all&maxResults=1&id=" +
	       id;

	std::string output;
	lastError = RequestError();

	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "")) {
		blog(LOG_ERROR,
		     "[%s][%s] Failed to retrieve live broadcast list",
		     PluginLogName(), __FUNCTION__);

		return false;
	}

	LiveBroadcastListResponse response;
	try {
		response = nlohmann::json::parse(output);

		if (response.items.empty()) {
			lastError = RequestError("No live broadcast found", "");
			blog(LOG_ERROR, "[%s][%s] %s", PluginLogName(),
			     __FUNCTION__, lastError.message.c_str());
			return false;
		}

		broadcast = response.items[0];
		return true;

	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::UpdateLiveBroadcast(LiveBroadcast &broadcast)
{
	std::string url = API_URL;
	url += LIVE_BROADCASTS_API_ENDPOINT;
	url += "?part=id,snippet,contentDetails,status";

	nlohmann::json json;
	try {
		json = broadcast;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
		return false;
	}

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "PUT", json.dump().c_str())) {
		blog(LOG_ERROR, "[%s][%s] Failed to update live broadcast",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	try {
		broadcast = nlohmann::json::parse(output);
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

bool ServiceOAuth::TransitionLiveBroadcast(
	const std::string &id, const LiveBroadcastTransitionStatus &status)
{
	std::string url = API_URL;
	url += LIVE_BROADCASTS_API_ENDPOINT;
	url += "/transition?id=" + id + "&broadcastStatus=";

	switch (status) {
	case LiveBroadcastTransitionStatus::COMPLETE:
		url += "complete";
		break;
	case LiveBroadcastTransitionStatus::LIVE:
		url += "live";
		break;
	case LiveBroadcastTransitionStatus::TESTING:
		url += "testing";
		break;
	}

	url += "&part=status";

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "POST", "{}")) {
		blog(LOG_ERROR, "[%s][%s] Failed to update live broadcast",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	return true;
}

bool ServiceOAuth::InsertLiveChatMessage(LiveChatMessage &message)
{
	std::string url = API_URL;
	url += "/liveChat/messages?part=snippet";

	nlohmann::json json;
	try {
		json = message;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
		return false;
	}

	std::string output;
	lastError = RequestError();
	if (!TryGetRemoteFile(__FUNCTION__, url.c_str(), output,
			      "application/json", "POST",
			      json.dump().c_str())) {
		blog(LOG_ERROR, "[%s][%s] Failed to send message",
		     PluginLogName(), __FUNCTION__);
		return false;
	}

	try {
		message = nlohmann::json::parse(output);
		return true;
	} catch (nlohmann::json::exception &e) {
		lastError = RequestError(RequestErrorType::JSON_PARSING_FAILED,
					 e.what());
		blog(LOG_ERROR, "[%s][%s] %s: %s", PluginLogName(),
		     __FUNCTION__, lastError.message.c_str(),
		     lastError.error.c_str());
	}

	/* Check if it is an error response */
	CheckIfSuccessRequestIsError(__FUNCTION__, output);

	return false;
}

}
