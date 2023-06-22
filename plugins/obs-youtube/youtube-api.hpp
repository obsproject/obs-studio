// SPDX-FileCopyrightText: 2023 tytan652 <tytan652@tytanium.xyz>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <oauth.hpp>

namespace YouTubeApi {
using nlohmann::json;

#ifndef NLOHMANN_OPTIONAL_YouTubeApi_HELPER
#define NLOHMANN_OPTIONAL_YouTubeApi_HELPER
template<typename T>
inline std::optional<T> get_stack_optional(const json &j, const char *property)
{
	auto it = j.find(property);
	if (it != j.end() && !it->is_null()) {
		return j.at(property).get<std::optional<T>>();
	}
	return std::optional<T>();
}
#endif

/* NOTE: Heavily stripped out structure of the object */
struct ErrorVerbose {
	std::string reason;
};

struct Error {
	int64_t code;
	std::string message;
	std::vector<ErrorVerbose> errors;
};

struct ErrorResponse {
	Error error;
};

/* NOTE: Heavily stripped out structure of the object */
struct ChannelSnippet {
	std::string title;
};

/* NOTE: Heavily stripped out structure of the object */
struct Channel {
	std::string id;
	/* NOTE: "snippet" should be optional but since we will always ask for
	 * this part. So put as not optional. */
	ChannelSnippet snippet;
};

/* NOTE: Heavily stripped out structure of the response */
struct ChannelListSnippetMineResponse {
	std::vector<Channel> items;
};

struct VideoCategorySnippet {
	std::string channelId;
	std::string title;
	bool assignable;
};

/* NOTE: Stripped out structure of the object */
struct VideoCategory {
	std::string id;
	VideoCategorySnippet snippet;
};

/* NOTE: Heavily stripped out structure of the response */
struct VideoCategoryListResponse {
	std::vector<VideoCategory> items;
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveBroadcastSnippet {
	std::string title;
	std::string description;
	std::optional<std::string> scheduledStartTime;
	std::optional<std::string> scheduledEndTime;
	std::optional<std::string> liveChatId;
};

enum class LiveBroadcastLifeCycleStatus : int {
	COMPLETE,
	CREATED,
	LIVE,
	LIVE_STARTING,
	READY,
	REVOKED,
	TEST_STARTING,
	TESTING,
};

enum class LiveBroadcastPrivacyStatus : int {
	PRIVATE,
	PUBLIC,
	UNLISTED,
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveBroadcastStatus {
	LiveBroadcastLifeCycleStatus lifeCycleStatus;
	LiveBroadcastPrivacyStatus privacyStatus;
	bool selfDeclaredMadeForKids;
};

struct LiveBroadcastMonitorStream {
	bool enableMonitorStream;
	std::optional<uint64_t> broadcastStreamDelayMs;
};

enum class LiveBroadcastClosedCaptionsType : int {
	DISABLED,
	HTTP_POST,
	EMBEDDED,
};

enum class LiveBroadcastProjection : int {
	RECTANGULAR,
	THREE_HUNDRED_SIXTY,
};

enum class LiveBroadcastLatencyPreference : int {
	NORMAL,
	LOW,
	ULTRALOW,
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveBroadcastContentDetails {
	std::optional<std::string> boundStreamId;
	LiveBroadcastMonitorStream monitorStream;
	std::optional<bool> enableEmbed;
	bool enableDvr;
	std::optional<bool> recordFromStart;
	std::optional<LiveBroadcastClosedCaptionsType> closedCaptionsType;
	LiveBroadcastProjection projection;
	LiveBroadcastLatencyPreference latencyPreference;
	bool enableAutoStart;
	bool enableAutoStop;
};

/* NOTE: We always request all of those */
struct LiveBroadcast {
	std::string id;
	LiveBroadcastSnippet snippet;
	LiveBroadcastStatus status;
	LiveBroadcastContentDetails contentDetails;
};

/* NOTE: Stripped out structure of the response */
struct LiveBroadcastListResponse {
	std::optional<std::string> nextPageToken;
	std::vector<LiveBroadcast> items;
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveStreamSnippet {
	std::string title;
};

enum class LiveStreamCdnIngestionType : int {
	DASH,
	HLS,
	RTMP,
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveStreamCdnIngestionInfo {
	std::string streamName;
};

enum class LiveStreamCdnResolution : int {
	RESOLUTION_VARIABLE,
	RESOLUTION_240P,
	RESOLUTION_360P,
	RESOLUTION_480P,
	RESOLUTION_720P,
	RESOLUTION_1080P,
	RESOLUTION_1440P,
	RESOLUTION_2160P,
};

enum class LiveStreamCdnFrameRate : int {
	FRAMERATE_VARIABLE,
	FRAMERATE_30FPS,
	FRAMERATE_60FPS,
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveStreamCdn {
	LiveStreamCdnIngestionType ingestionType;
	LiveStreamCdnIngestionInfo ingestionInfo;
	LiveStreamCdnResolution resolution;
	LiveStreamCdnFrameRate frameRate;
};

enum class LiveStreamStatusEnum : int {
	ACTIVE,
	CREATED,
	/* Blame MSVC to not allow ERROR */
	ERR0R,
	INACTIVE,
	READY,
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveStreamStatus {
	LiveStreamStatusEnum streamStatus;
};

/* NOTE: Stripped out structure of the object */
struct LiveStreamContentDetails {
	bool isReusable;
};

/* NOTE: We always request all of those */
struct LiveStream {
	std::string id;
	LiveStreamSnippet snippet;
	LiveStreamCdn cdn;
	LiveStreamStatus status;
	std::optional<LiveStreamContentDetails> contentDetails;
};

/* NOTE: Stripped out structure of the response */
struct LiveStreamListResponse {
	std::vector<LiveStream> items;
};

/* NOTE: Stripped out structure of the object */
struct VideoSnippet {
	std::string title;
	std::string description;
	std::string categoryId;
};

/* NOTE: Heavily stripped out structure of the object */
struct Video {
	std::string id;
	VideoSnippet snippet;
};

enum class LiveChatTextMessageType : int {
	CHAT_ENDED_EVENT,
	MESSAGE_DELETED_EVENT,
	SPONSOR_ONLY_MODE_ENDED_EVENT,
	SPONSOR_ONLY_MODE_STARTED_EVENT,
	NEW_SPONSOR_EVENT,
	MEMBER_MILESTONE_CHAT_EVENT,
	SUPER_CHAT_EVENT,
	SUPER_STICKER_EVENT,
	TEXT_MESSAGE_EVENT,
	TOMBSTONE,
	USER_BANNED_EVENT,
	MEMBERSHIP_GIFTING_EVENT,
	GIFT_MEMBERSHIP_RECEIVED_EVENT,
};

/* NOTE: Stripped out structure of the object */
struct LiveChatTextMessageDetails {
	std::string messageText;
};

/* NOTE: Heavily stripped out structure of the object */
struct LiveChatMessageSnippet {
	LiveChatTextMessageType type;
	std::string liveChatId;
	LiveChatTextMessageDetails textMessageDetails;
};

/* NOTE: Stripped out structure of the object */
struct LiveChatMessage {
	LiveChatMessageSnippet snippet;
};

inline void from_json(const json &j, ErrorVerbose &s)
{
	s.reason = j.at("reason").get<std::string>();
}

inline void from_json(const json &j, Error &s)
{
	s.code = j.at("code").get<int64_t>();
	s.message = j.at("message").get<std::string>();
	s.errors = j.at("errors").get<std::vector<ErrorVerbose>>();
}

inline void from_json(const json &j, ErrorResponse &s)
{
	s.error = j.at("error").get<Error>();
}

inline void from_json(const json &j, ChannelSnippet &s)
{
	s.title = j.at("title").get<std::string>();
}

inline void from_json(const json &j, Channel &s)
{
	s.id = j.at("id").get<std::string>();
	s.snippet = j.at("snippet").get<ChannelSnippet>();
}

inline void from_json(const json &j, ChannelListSnippetMineResponse &s)
{
	s.items = j.at("items").get<std::vector<Channel>>();
}

inline void from_json(const json &j, VideoCategorySnippet &s)
{
	s.channelId = j.at("channelId").get<std::string>();
	s.title = j.at("title").get<std::string>();
	s.assignable = j.at("assignable").get<bool>();
}

inline void from_json(const json &j, VideoCategory &s)
{
	s.id = j.at("id").get<std::string>();
	s.snippet = j.at("snippet").get<VideoCategorySnippet>();
}

inline void from_json(const json &j, VideoCategoryListResponse &s)
{
	s.items = j.at("items").get<std::vector<VideoCategory>>();
}

inline void from_json(const json &j, LiveBroadcastSnippet &s)
{
	s.title = j.at("title").get<std::string>();
	s.description = j.at("description").get<std::string>();
	s.scheduledStartTime =
		get_stack_optional<std::string>(j, "scheduledStartTime");
	s.scheduledEndTime =
		get_stack_optional<std::string>(j, "scheduledEndTime");
	s.liveChatId = get_stack_optional<std::string>(j, "liveChatId");
}

inline void to_json(json &j, const LiveBroadcastSnippet &s)
{
	j = json::object();
	j["title"] = s.title;
	j["description"] = s.description;
	j["scheduledStartTime"] = s.scheduledStartTime;
	j["scheduledEndTime"] = s.scheduledEndTime;
}

inline void from_json(const json &j, LiveBroadcastLifeCycleStatus &e)
{
	if (j == "complete")
		e = LiveBroadcastLifeCycleStatus::COMPLETE;
	else if (j == "created")
		e = LiveBroadcastLifeCycleStatus::CREATED;
	else if (j == "live")
		e = LiveBroadcastLifeCycleStatus::LIVE;
	else if (j == "liveStarting")
		e = LiveBroadcastLifeCycleStatus::LIVE_STARTING;
	else if (j == "ready")
		e = LiveBroadcastLifeCycleStatus::READY;
	else if (j == "revoked")
		e = LiveBroadcastLifeCycleStatus::REVOKED;
	else if (j == "testStarting")
		e = LiveBroadcastLifeCycleStatus::TEST_STARTING;
	else if (j == "testing")
		e = LiveBroadcastLifeCycleStatus::TESTING;
	else {
		throw std::runtime_error("Unknown \"lifeCycleStatus\" value");
	}
}

inline void from_json(const json &j, LiveBroadcastPrivacyStatus &e)
{
	if (j == "private")
		e = LiveBroadcastPrivacyStatus::PRIVATE;
	else if (j == "public")
		e = LiveBroadcastPrivacyStatus::PUBLIC;
	else if (j == "unlisted")
		e = LiveBroadcastPrivacyStatus::UNLISTED;
	else {
		throw std::runtime_error("Unknown \"privacyStatus\" value");
	}
}

inline void to_json(json &j, const LiveBroadcastPrivacyStatus &e)
{
	switch (e) {
	case LiveBroadcastPrivacyStatus::PRIVATE:
		j = "private";
		break;
	case LiveBroadcastPrivacyStatus::PUBLIC:
		j = "public";
		break;
	case LiveBroadcastPrivacyStatus::UNLISTED:
		j = "unlisted";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveBroadcastStatus &s)
{
	s.lifeCycleStatus =
		j.at("lifeCycleStatus").get<LiveBroadcastLifeCycleStatus>();
	s.privacyStatus =
		j.at("privacyStatus").get<LiveBroadcastPrivacyStatus>();
	s.selfDeclaredMadeForKids = j.at("selfDeclaredMadeForKids").get<bool>();
}

inline void to_json(json &j, const LiveBroadcastStatus &s)
{
	j = json::object();
	j["privacyStatus"] = s.privacyStatus;
	j["selfDeclaredMadeForKids"] = s.selfDeclaredMadeForKids;
}

inline void from_json(const json &j, LiveBroadcastMonitorStream &s)
{
	s.enableMonitorStream = j.at("enableMonitorStream").get<bool>();
	s.broadcastStreamDelayMs =
		get_stack_optional<uint64_t>(j, "broadcastStreamDelayMs");
}

inline void to_json(json &j, const LiveBroadcastMonitorStream &s)
{
	j = json::object();
	j["enableMonitorStream"] = s.enableMonitorStream;
	j["broadcastStreamDelayMs"] = s.broadcastStreamDelayMs;
}

inline void from_json(const json &j, LiveBroadcastLatencyPreference &e)
{
	if (j == "normal")
		e = LiveBroadcastLatencyPreference::NORMAL;
	else if (j == "low")
		e = LiveBroadcastLatencyPreference::LOW;
	else if (j == "ultraLow")
		e = LiveBroadcastLatencyPreference::ULTRALOW;
	else {
		throw std::runtime_error("Unknown \"latencyPreference\" value");
	}
}

inline void to_json(json &j, const LiveBroadcastLatencyPreference &e)
{
	switch (e) {
	case LiveBroadcastLatencyPreference::NORMAL:
		j = "normal";
		break;
	case LiveBroadcastLatencyPreference::LOW:
		j = "low";
		break;
	case LiveBroadcastLatencyPreference::ULTRALOW:
		j = "ultraLow";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveBroadcastClosedCaptionsType &e)
{
	if (j == "closedCaptionsDisabled")
		e = LiveBroadcastClosedCaptionsType::DISABLED;
	else if (j == "closedCaptionsHttpPost")
		e = LiveBroadcastClosedCaptionsType::HTTP_POST;
	else if (j == "closedCaptionsEmbedded")
		e = LiveBroadcastClosedCaptionsType::EMBEDDED;
	else {
		throw std::runtime_error(
			"Unknown \"closedCaptionsType\" value");
	}
}

inline void to_json(json &j, const LiveBroadcastClosedCaptionsType &e)
{
	switch (e) {
	case LiveBroadcastClosedCaptionsType::DISABLED:
		j = "closedCaptionsDisabled";
		break;
	case LiveBroadcastClosedCaptionsType::HTTP_POST:
		j = "closedCaptionsHttpPost";
		break;
	case LiveBroadcastClosedCaptionsType::EMBEDDED:
		j = "closedCaptionsEmbedded";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveBroadcastProjection &e)
{
	if (j == "rectangular")
		e = LiveBroadcastProjection::RECTANGULAR;
	else if (j == "360")
		e = LiveBroadcastProjection::THREE_HUNDRED_SIXTY;
	else {
		throw std::runtime_error("Unknown \"latencyPreference\" value");
	}
}

inline void to_json(json &j, const LiveBroadcastProjection &e)
{
	switch (e) {
	case LiveBroadcastProjection::RECTANGULAR:
		j = "rectangular";
		break;
	case LiveBroadcastProjection::THREE_HUNDRED_SIXTY:
		j = "360";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveBroadcastContentDetails &s)
{
	s.boundStreamId = get_stack_optional<std::string>(j, "boundStreamId");
	s.monitorStream =
		j.at("monitorStream").get<LiveBroadcastMonitorStream>();
	s.enableEmbed = get_stack_optional<bool>(j, "enableEmbed");
	s.enableDvr = j.at("enableDvr").get<bool>();
	s.recordFromStart = get_stack_optional<bool>(j, "recordFromStart");
	s.closedCaptionsType =
		get_stack_optional<LiveBroadcastClosedCaptionsType>(
			j, "closedCaptionsType");
	s.projection = j.at("projection").get<LiveBroadcastProjection>();
	s.latencyPreference =
		j.at("latencyPreference").get<LiveBroadcastLatencyPreference>();
	s.enableAutoStart = j.at("enableAutoStart").get<bool>();
	s.enableAutoStop = j.at("enableAutoStop").get<bool>();
}

inline void to_json(json &j, const LiveBroadcastContentDetails &s)
{
	j = json::object();
	j["monitorStream"] = s.monitorStream;
	j["enableEmbed"] = s.enableEmbed;
	j["enableDvr"] = s.enableDvr;
	j["recordFromStart"] = s.recordFromStart;
	j["closedCaptionsType"] = s.closedCaptionsType;
	j["projection"] = s.projection;
	j["latencyPreference"] = s.latencyPreference;
	j["enableAutoStart"] = s.enableAutoStart;
	j["enableAutoStop"] = s.enableAutoStop;
}

inline void from_json(const json &j, LiveBroadcast &s)
{
	s.id = j.at("id").get<std::string>();
	s.snippet = j.at("snippet").get<LiveBroadcastSnippet>();
	s.status = j.at("status").get<LiveBroadcastStatus>();
	s.contentDetails =
		j.at("contentDetails").get<LiveBroadcastContentDetails>();
}

inline void to_json(json &j, const LiveBroadcast &s)
{
	j = json::object();
	j["id"] = s.id;
	j["snippet"] = s.snippet;
	j["status"] = s.status;
	j["contentDetails"] = s.contentDetails;
}

inline void from_json(const json &j, LiveBroadcastListResponse &s)
{
	s.nextPageToken = get_stack_optional<std::string>(j, "nextPageToken");
	s.items = j.at("items").get<std::vector<LiveBroadcast>>();
}

inline void from_json(const json &j, LiveStreamSnippet &s)
{
	s.title = j.at("title").get<std::string>();
}

inline void to_json(json &j, const LiveStreamSnippet &s)
{
	j = json::object();
	j["title"] = s.title;
}

inline void from_json(const json &j, LiveStreamCdnIngestionType &e)
{
	if (j == "dash")
		e = LiveStreamCdnIngestionType::DASH;
	else if (j == "hls")
		e = LiveStreamCdnIngestionType::HLS;
	else if (j == "rtmp")
		e = LiveStreamCdnIngestionType::RTMP;
	else {
		throw std::runtime_error("Unknown \"ingestionType\" value");
	}
}

inline void to_json(json &j, const LiveStreamCdnIngestionType &e)
{
	switch (e) {
	case LiveStreamCdnIngestionType::DASH:
		j = "dash";
		break;
	case LiveStreamCdnIngestionType::HLS:
		j = "hls";
		break;
	case LiveStreamCdnIngestionType::RTMP:
		j = "rtmp";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveStreamCdnIngestionInfo &s)
{
	s.streamName = j.at("streamName").get<std::string>();
}

inline void from_json(const json &j, LiveStreamCdnResolution &e)
{
	if (j == "variable")
		e = LiveStreamCdnResolution::RESOLUTION_VARIABLE;
	else if (j == "240p")
		e = LiveStreamCdnResolution::RESOLUTION_240P;
	else if (j == "360p")
		e = LiveStreamCdnResolution::RESOLUTION_360P;
	else if (j == "480p")
		e = LiveStreamCdnResolution::RESOLUTION_480P;
	else if (j == "720p")
		e = LiveStreamCdnResolution::RESOLUTION_720P;
	else if (j == "1080p")
		e = LiveStreamCdnResolution::RESOLUTION_1080P;
	else if (j == "1440p")
		e = LiveStreamCdnResolution::RESOLUTION_1440P;
	else if (j == "2160p")
		e = LiveStreamCdnResolution::RESOLUTION_2160P;
	else {
		throw std::runtime_error("Unknown \"resolution\" value");
	}
}

inline void to_json(json &j, const LiveStreamCdnResolution &e)
{
	switch (e) {
	case LiveStreamCdnResolution::RESOLUTION_VARIABLE:
		j = "variable";
		break;
	case LiveStreamCdnResolution::RESOLUTION_240P:
		j = "240p";
		break;
	case LiveStreamCdnResolution::RESOLUTION_360P:
		j = "360p";
		break;
	case LiveStreamCdnResolution::RESOLUTION_480P:
		j = "480p";
		break;
	case LiveStreamCdnResolution::RESOLUTION_720P:
		j = "720p";
		break;
	case LiveStreamCdnResolution::RESOLUTION_1080P:
		j = "1080p";
		break;
	case LiveStreamCdnResolution::RESOLUTION_1440P:
		j = "1440p";
		break;
	case LiveStreamCdnResolution::RESOLUTION_2160P:
		j = "2160p";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveStreamCdnFrameRate &e)
{
	if (j == "variable")
		e = LiveStreamCdnFrameRate::FRAMERATE_VARIABLE;
	else if (j == "30fps")
		e = LiveStreamCdnFrameRate::FRAMERATE_30FPS;
	else if (j == "60fps")
		e = LiveStreamCdnFrameRate::FRAMERATE_60FPS;
	else {
		throw std::runtime_error("Unknown \"frameRate\" value");
	}
}

inline void to_json(json &j, const LiveStreamCdnFrameRate &e)
{
	switch (e) {
	case LiveStreamCdnFrameRate::FRAMERATE_VARIABLE:
		j = "variable";
		break;
	case LiveStreamCdnFrameRate::FRAMERATE_30FPS:
		j = "30fps";
		break;
	case LiveStreamCdnFrameRate::FRAMERATE_60FPS:
		j = "60fps";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveStreamCdn &s)
{
	s.ingestionType =
		j.at("ingestionType").get<LiveStreamCdnIngestionType>();
	s.ingestionInfo =
		j.at("ingestionInfo").get<LiveStreamCdnIngestionInfo>();
	s.resolution = j.at("resolution").get<LiveStreamCdnResolution>();
	s.frameRate = j.at("frameRate").get<LiveStreamCdnFrameRate>();
}

inline void to_json(json &j, const LiveStreamCdn &s)
{
	j = json::object();
	j["ingestionType"] = s.ingestionType;
	j["resolution"] = s.resolution;
	j["frameRate"] = s.frameRate;
}

inline void from_json(const json &j, LiveStreamStatusEnum &e)
{
	if (j == "active")
		e = LiveStreamStatusEnum::ACTIVE;
	else if (j == "created")
		e = LiveStreamStatusEnum::CREATED;
	else if (j == "error")
		e = LiveStreamStatusEnum::ERR0R;
	else if (j == "inactive")
		e = LiveStreamStatusEnum::INACTIVE;
	else if (j == "ready")
		e = LiveStreamStatusEnum::READY;
	else {
		throw std::runtime_error("Unknown \"streamStatus\" value");
	}
}

inline void from_json(const json &j, LiveStreamStatus &s)
{
	s.streamStatus = j.at("streamStatus").get<LiveStreamStatusEnum>();
}

inline void from_json(const json &j, LiveStreamContentDetails &s)
{
	s.isReusable = j.at("isReusable").get<bool>();
}

inline void to_json(json &j, const LiveStreamContentDetails &s)
{
	j = json::object();
	j["isReusable"] = s.isReusable;
}

inline void from_json(const json &j, LiveStream &s)
{
	s.id = j.at("id").get<std::string>();
	s.snippet = j.at("snippet").get<LiveStreamSnippet>();
	s.cdn = j.at("cdn").get<LiveStreamCdn>();
	s.status = j.at("status").get<LiveStreamStatus>();
	s.contentDetails = get_stack_optional<LiveStreamContentDetails>(
		j, "contentDetails");
}

inline void to_json(json &j, const LiveStream &s)
{
	j = json::object();
	j["snippet"] = s.snippet;
	j["cdn"] = s.cdn;
	j["contentDetails"] = s.contentDetails;
}

inline void from_json(const json &j, LiveStreamListResponse &s)
{
	s.items = j.at("items").get<std::vector<LiveStream>>();
}

inline void from_json(const json &j, VideoSnippet &s)
{
	s.title = j.at("title").get<std::string>();
	s.description = j.at("description").get<std::string>();
	s.categoryId = j.at("categoryId").get<std::string>();
}

inline void to_json(json &j, const VideoSnippet &s)
{
	j = json::object();
	j["title"] = s.title;
	j["description"] = s.description;
	j["categoryId"] = s.categoryId;
}

inline void from_json(const json &j, Video &s)
{
	s.id = j.at("id").get<std::string>();
	s.snippet = j.at("snippet").get<VideoSnippet>();
}

inline void to_json(json &j, const Video &s)
{
	j = json::object();
	j["id"] = s.id;
	j["snippet"] = s.snippet;
}

inline void from_json(const json &j, LiveChatTextMessageType &e)
{
	if (j == "chatEndedEvent")
		e = LiveChatTextMessageType::CHAT_ENDED_EVENT;
	else if (j == "messageDeletedEvent")
		e = LiveChatTextMessageType::MESSAGE_DELETED_EVENT;
	else if (j == "sponsorOnlyModeEndedEvent")
		e = LiveChatTextMessageType::SPONSOR_ONLY_MODE_ENDED_EVENT;
	else if (j == "sponsorOnlyModeStartedEvent")
		e = LiveChatTextMessageType::SPONSOR_ONLY_MODE_STARTED_EVENT;
	else if (j == "newSponsorEvent")
		e = LiveChatTextMessageType::NEW_SPONSOR_EVENT;
	else if (j == "memberMilestoneChatEvent")
		e = LiveChatTextMessageType::MEMBER_MILESTONE_CHAT_EVENT;
	else if (j == "superChatEvent")
		e = LiveChatTextMessageType::SUPER_CHAT_EVENT;
	else if (j == "superStickerEvent")
		e = LiveChatTextMessageType::SUPER_STICKER_EVENT;
	else if (j == "textMessageEvent")
		e = LiveChatTextMessageType::TEXT_MESSAGE_EVENT;
	else if (j == "tombstone")
		e = LiveChatTextMessageType::TOMBSTONE;
	else if (j == "userBannedEvent")
		e = LiveChatTextMessageType::USER_BANNED_EVENT;
	else if (j == "membershipGiftingEvent")
		e = LiveChatTextMessageType::MEMBERSHIP_GIFTING_EVENT;
	else if (j == "giftMembershipReceivedEvent")
		e = LiveChatTextMessageType::GIFT_MEMBERSHIP_RECEIVED_EVENT;
	else {
		throw std::runtime_error("Unknown \"type\" value");
	}
}
inline void to_json(json &j, const LiveChatTextMessageType &e)
{
	switch (e) {
	case LiveChatTextMessageType::CHAT_ENDED_EVENT:
		j = "chatEndedEvent";
		break;
	case LiveChatTextMessageType::MESSAGE_DELETED_EVENT:
		j = "messageDeletedEvent";
		break;
	case LiveChatTextMessageType::SPONSOR_ONLY_MODE_ENDED_EVENT:
		j = "sponsorOnlyModeEndedEvent";
		break;
	case LiveChatTextMessageType::SPONSOR_ONLY_MODE_STARTED_EVENT:
		j = "sponsorOnlyModeStartedEvent";
		break;
	case LiveChatTextMessageType::NEW_SPONSOR_EVENT:
		j = "newSponsorEvent";
		break;
	case LiveChatTextMessageType::MEMBER_MILESTONE_CHAT_EVENT:
		j = "memberMilestoneChatEvent";
		break;
	case LiveChatTextMessageType::SUPER_CHAT_EVENT:
		j = "superChatEvent";
		break;
	case LiveChatTextMessageType::SUPER_STICKER_EVENT:
		j = "superStickerEvent";
		break;
	case LiveChatTextMessageType::TEXT_MESSAGE_EVENT:
		j = "textMessageEvent";
		break;
	case LiveChatTextMessageType::TOMBSTONE:
		j = "tombstone";
		break;
	case LiveChatTextMessageType::USER_BANNED_EVENT:
		j = "userBannedEvent";
		break;
	case LiveChatTextMessageType::MEMBERSHIP_GIFTING_EVENT:
		j = "membershipGiftingEvent";
		break;
	case LiveChatTextMessageType::GIFT_MEMBERSHIP_RECEIVED_EVENT:
		j = "giftMembershipReceivedEvent";
		break;
	default:
		throw std::runtime_error("This should not happen");
	}
}

inline void from_json(const json &j, LiveChatTextMessageDetails &s)
{
	s.messageText = j.at("messageText").get<std::string>();
}

inline void to_json(json &j, const LiveChatTextMessageDetails &s)
{
	j = json::object();
	j["messageText"] = s.messageText;
}

inline void from_json(const json &j, LiveChatMessageSnippet &s)
{
	s.type = j.at("type").get<LiveChatTextMessageType>();
	s.liveChatId = j.at("liveChatId").get<std::string>();
	s.textMessageDetails = j.at("textMessageDetails");
}

inline void to_json(json &j, const LiveChatMessageSnippet &s)
{
	j = json::object();
	j["type"] = s.type;
	j["liveChatId"] = s.liveChatId;
	j["textMessageDetails"] = s.textMessageDetails;
}

inline void from_json(const json &j, LiveChatMessage &s)
{
	s.snippet = j.at("snippet").get<LiveChatMessageSnippet>();
}

inline void to_json(json &j, const LiveChatMessage &s)
{
	j = json::object();
	j["snippet"] = s.snippet;
}

}
