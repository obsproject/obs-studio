#pragma once

#include <json11.hpp>

#include <QString>
#include <QVector>

#include <string>

// Pure X livestream decisions shared by the UI and the offline tests.
// END is valid only for RUNNING -> ENDED, and that body must be exactly
// {"state":"END"}. A broadcast left in NOT_STARTED is timed out by X.

struct XStreamSource {
	QString id;
	QString name;
	QString region;
	QString rtmpUrl;
	QString rtmpsUrl;
	QString streamKey;
	bool streamActive = false;
};

inline QString XJsonString(const json11::Json &value)
{
	if (value.is_string()) {
		return QString::fromStdString(value.string_value());
	}
	return {};
}

inline XStreamSource XParseSource(const json11::Json &item)
{
	XStreamSource source;
	const json11::Json object = item["source"].is_object() ? item["source"] : item;
	source.id = XJsonString(object["id"]);
	if (source.id.isEmpty()) {
		source.id = XJsonString(object["rtmp_stream_key"]);
	}
	source.name = XJsonString(object["name"]);
	source.region = XJsonString(object["rtmp_region"]);
	source.rtmpUrl = XJsonString(object["rtmp_url"]);
	source.rtmpsUrl = XJsonString(object["rtmps_url"]);
	source.streamKey = XJsonString(object["rtmp_stream_key"]);
	if (source.streamKey.isEmpty()) {
		source.streamKey = source.id;
	}
	source.streamActive = object["is_stream_active"].bool_value();
	return source;
}

inline void XCollectSources(const json11::Json &json, QVector<XStreamSource> &out)
{
	if (json.is_array()) {
		for (const json11::Json &item : json.array_items()) {
			XStreamSource source = XParseSource(item);
			if (!source.id.isEmpty()) {
				out.push_back(source);
			}
		}
		return;
	}
	if (json["source"].is_object()) {
		XStreamSource source = XParseSource(json);
		if (!source.id.isEmpty()) {
			out.push_back(source);
		}
	}
	const char *keys[] = {"sources", "data"};
	for (const char *key : keys) {
		if (!json[key].is_array()) {
			continue;
		}
		for (const json11::Json &item : json[key].array_items()) {
			XStreamSource source = XParseSource(item);
			if (!source.id.isEmpty()) {
				out.push_back(source);
			}
		}
	}
}

inline void XCollectSourceResponse(const json11::Json &json, QVector<XStreamSource> &out)
{
	XCollectSources(json, out);
	if (out.isEmpty() && json["data"].is_object()) {
		XCollectSources(json["data"], out);
	}
}

inline QString XPreferredIngest(const XStreamSource &source)
{
	if (!source.rtmpsUrl.isEmpty()) {
		return source.rtmpsUrl;
	}
	return source.rtmpUrl;
}

inline std::string XPublishStateBody(const std::string &title, bool shouldNotTweet, int chatOption)
{
	const json11::Json::object payload = {
		{"state", "PUBLISH"},
		{"title", title},
		{"should_not_tweet", shouldNotTweet},
		{"chat_option", chatOption},
	};
	return json11::Json(payload).dump();
}

inline const char *XEndStateBody()
{
	return "{\"state\":\"END\"}";
}

inline bool XShouldEndBroadcast(bool broadcastPublished, bool hasBroadcastId)
{
	return broadcastPublished && hasBroadcastId;
}

inline bool XIsAccessDenied(long status)
{
	return status == 401 || status == 403;
}

inline bool XStopOutputAfterPublishFailure(bool published)
{
	return !published;
}

enum class XStartKeyAction { Allow, ApplyThenRecheck, Block };

inline XStartKeyAction XPlanStartKey(bool serviceHasKey, bool authHasKey)
{
	if (serviceHasKey) {
		return XStartKeyAction::Allow;
	}
	if (authHasKey) {
		return XStartKeyAction::ApplyThenRecheck;
	}
	return XStartKeyAction::Block;
}

inline bool XStartBlockedAfterApply(XStartKeyAction action, bool serviceHasKeyAfterApply)
{
	switch (action) {
	case XStartKeyAction::Allow:
		return false;
	case XStartKeyAction::ApplyThenRecheck:
		return !serviceHasKeyAfterApply;
	case XStartKeyAction::Block:
		return true;
	}
	return true;
}

enum class XEnsureResult { Failed, ReusedSaved, ReusedListed, Created };

template<typename RegionFn, typename GetFn, typename ListFn, typename CreateFn>
inline XEnsureResult XRunEnsureSource(const QString &savedId, RegionFn &&regionFn, GetFn &&getFn, ListFn &&listFn,
				      CreateFn &&createFn, XStreamSource &chosen, QString &regionOut)
{
	if (!regionFn(regionOut)) {
		return XEnsureResult::Failed;
	}
	if (!savedId.isEmpty()) {
		XStreamSource existing;
		if (getFn(savedId, existing) && existing.region == regionOut && !XPreferredIngest(existing).isEmpty()) {
			chosen = existing;
			return XEnsureResult::ReusedSaved;
		}
	}
	QVector<XStreamSource> sources;
	if (listFn(sources)) {
		for (const XStreamSource &source : sources) {
			if (source.region == regionOut && !XPreferredIngest(source).isEmpty()) {
				chosen = source;
				return XEnsureResult::ReusedListed;
			}
		}
	}
	if (!createFn(regionOut, chosen)) {
		return XEnsureResult::Failed;
	}
	if (chosen.region.isEmpty()) {
		chosen.region = regionOut;
	}
	return XEnsureResult::Created;
}

enum class XGoLiveResult { FailedBeforeCreate, FailedCreate, FailedPublish, Published };

template<typename WaitFn, typename CreateFn, typename PublishFn>
inline XGoLiveResult XRunGoLive(WaitFn &&waitFn, CreateFn &&createFn, PublishFn &&publishFn, QString &createdId)
{
	if (!waitFn()) {
		return XGoLiveResult::FailedBeforeCreate;
	}
	if (!createFn(createdId)) {
		return XGoLiveResult::FailedCreate;
	}
	if (!publishFn(createdId)) {
		return XGoLiveResult::FailedPublish;
	}
	return XGoLiveResult::Published;
}
