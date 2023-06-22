// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>
#include <util/util.hpp>
#include <obs-frontend-api.h>
#include <obs-module.h>

#include <oauth-service-base.hpp>
#include <oauth-local-redirect.hpp>

#include "youtube-api.hpp"

#define QT_UTF8(str) QString::fromUtf8(str, -1)
#define QT_TO_UTF8(str) str.toUtf8().constData()

class QuickThread : public QThread {
public:
	explicit inline QuickThread(std::function<void()> func_) : func(func_)
	{
	}

private:
	virtual void run() override { func(); }

	std::function<void()> func;
};

struct OBSYoutubeActionsSettings {
	bool rememberSettings = false;
	std::string title;
	std::string description;
	YouTubeApi::LiveBroadcastPrivacyStatus privacy;
	std::string categoryId;
	YouTubeApi::LiveBroadcastLatencyPreference latency;
	bool madeForKids;
	bool autoStart;
	bool autoStop;
	bool dvr;
	bool scheduleForLater;
	YouTubeApi::LiveBroadcastProjection projection;
	std::string thumbnailFile;
};

class YoutubeChat;
namespace YouTubeApi {

class LocalRedirect : public OAuth::LocalRedirect {
	Q_OBJECT

	inline QString DialogTitle() override
	{
		return obs_module_text("YouTube.Auth.LoginDialog.Title");
	}
	inline QString DialogText() override
	{
		return obs_module_text("YouTube.Auth.LoginDialog.Text");
	}
	inline QString ReOpenUrlButtonText() override
	{
		return obs_module_text("YouTube.Auth.LoginDialog.ReOpenURL");
	}

	inline QString ServerPageTitle() override { return "OBS YouTube"; }
	inline QString ServerResponsePageLogoUrl() override
	{
		return "https://obsproject.com/assets/images/new_icon_small-r.png";
	}
	inline QString ServerResponsePageLogoAlt() override { return "OBS"; }
	inline QString ServerResponseSuccessText() override
	{
		return obs_module_text("YouTube.Auth.RedirectServer.Success");
	}
	inline QString ServerResponseFailureText() override
	{
		return obs_module_text("YouTube.Auth.RedirectServer.Failure");
	}

	inline void DebugLog(const QString &info) override
	{
		blog(LOG_DEBUG, "[obs-youtube] [YouTubeLocalRedirect] %s",
		     info.toUtf8().constData());
	};

public:
	LocalRedirect(const QString &baseAuthUrl, const QString &clientId,
		      const QString &scope, bool useState, QWidget *parent)
		: OAuth::LocalRedirect(baseAuthUrl, clientId, scope, useState,
				       parent)
	{
	}
	~LocalRedirect() {}
};

struct ChannelInfo {
	std::string id;
	std::string title;

	ChannelInfo() {}
	ChannelInfo(const std::string &id, const std::string &title)
		: id(id),
		  title(title)
	{
	}
};

enum class LiveBroadcastListStatus : int {
	ACTIVE,
	ALL,
	COMPLETED,
	UPCOMMING,
};

enum class LiveBroadcastTransitionStatus : int {
	COMPLETE,
	LIVE,
	TESTING,
};

class ServiceOAuth : public OAuth::ServiceBase {
	LiveStreamCdnIngestionType ingestionType =
		LiveStreamCdnIngestionType::RTMP;

	RequestError lastError;

	ChannelInfo channelInfo;
	std::vector<VideoCategory> videoCategoryList;
	OBSYoutubeActionsSettings uiSettings;

	std::string broadcastId;

	obs_frontend_broadcast_flow broadcastFlow = {0};
	std::string broadcastLastError;

	std::string liveStreamId;
	std::string streamKey;
	obs_broadcast_state broadcastState;
	obs_broadcast_start broadcastStartType;
	obs_broadcast_stop broadcastStopType;

	YoutubeChat *chat = nullptr;

	inline std::string UserAgent() override;

	inline const char *TokenUrl() override;

	inline std::string ClientId() override;
	inline std::string ClientSecret() override;

	inline int64_t ScopeVersion() override;

	bool LoginInternal(const OAuth::LoginReason &reason, std::string &code,
			   std::string &redirectUri) override;
	bool SignOutInternal() override;

	void SetSettings(obs_data_t *data) override;
	obs_data_t *GetSettingsData() override;

	void LoadFrontendInternal() override;
	void UnloadFrontendInternal() override;

	void AddBondedServiceFrontend(obs_service_t *service) override;
	void RemoveBondedServiceFrontend(obs_service_t *service) override;

	const char *PluginLogName() override { return "obs-youtube"; };

	void LoginError(RequestError &error) override;

	static obs_broadcast_state GetBroadcastState(void *priv);
	static obs_broadcast_start GetBroadcastStartType(void *priv);
	static obs_broadcast_stop GetBroadcastStopType(void *priv);
	static void ManageBroadcast(void *priv);

	static void StoppedStreaming(void *priv);

	static void DifferedStartBroadcast(void *priv);
	static obs_broadcast_stream_state IsBroadcastStreamActive(void *priv);
	static bool DifferedStopBroadcast(void *priv);
	static const char *GetBroadcastsLastError(void *priv);

	bool WrappedGetRemoteFile(const char *url, std::string &str,
				  long *responseCode,
				  const char *contentType = nullptr,
				  std::string request_type = "",
				  const char *postData = nullptr,
				  int postDataSize = 0);

	bool TryGetRemoteFile(const char *funcName, const char *url,
			      std::string &str,
			      const char *contentType = nullptr,
			      std::string request_type = "",
			      const char *postData = nullptr,
			      int postDataSize = 0);

	void CheckIfSuccessRequestIsError(const char *funcName,
					  const std::string &output);

public:
	ServiceOAuth();

	void SetIngestionType(LiveStreamCdnIngestionType type)
	{
		ingestionType = type;
	}
	LiveStreamCdnIngestionType GetIngestionType() { return ingestionType; }

	void SetBroadcastId(const std::string &id) { broadcastId = id; }
	std::string GetBroadcastId() { return broadcastId; }

	const char *GetStreamKey()
	{
		return streamKey.empty() ? nullptr : streamKey.c_str();
	};

	void SetNewStream(const std::string &id, const std::string &key,
			  bool autoStart, bool autoStop, bool startNow);

	void SetChatIds(const std::string &broadcastId,
			const std::string &chatId);
	void ResetChat();

	RequestError GetLastError() { return lastError; }

	bool GetChannelInfo(ChannelInfo &info);
	bool GetVideoCategoriesList(std::vector<VideoCategory> &list,
				    bool forceUS = false);
	bool GetLiveBroadcastsList(const LiveBroadcastListStatus &status,
				   std::vector<LiveBroadcast> &list);
	bool FindLiveStream(const std::string &id, LiveStream &stream);
	bool InsertLiveBroadcast(LiveBroadcast &broadcast);
	bool UpdateVideo(Video &video);
	bool SetThumbnail(const std::string &id, const char *mimeType,
			  const char *thumbnailData, int thumbnailSize);
	bool InsertLiveStream(LiveStream &stream);
	bool BindLiveBroadcast(LiveBroadcast &broadcast,
			       const std::string &streamId = {});
	bool FindLiveBroadcast(const std::string &id, LiveBroadcast &broadcast);
	bool UpdateLiveBroadcast(LiveBroadcast &broadcast);
	bool
	TransitionLiveBroadcast(const std::string &id,
				const LiveBroadcastTransitionStatus &status);

	bool InsertLiveChatMessage(LiveChatMessage &message);
};

}
