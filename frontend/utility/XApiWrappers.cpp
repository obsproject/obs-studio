#include "XApiWrappers.hpp"

#include <OBSApp.hpp>
#include <utility/RemoteTextThread.hpp>
#include <widgets/OBSBasic.hpp>

#include <qt-wrappers.hpp>

#include <QThread>
#include <QUrl>
#include <QVector>

#include <json11.hpp>

#include "moc_XApiWrappers.cpp"

using namespace json11;

namespace {

constexpr const char *XApiHost = "https://api.x.com";
constexpr int kStreamActiveAttempts = 20;

QString JsonAsString(const Json &value)
{
	if (value.is_string()) {
		return QString::fromStdString(value.string_value());
	}
	return {};
}

QString ErrorText(const Json &json)
{
	const std::string message = json["message"].string_value();
	if (!message.empty()) {
		return QString::fromStdString(message);
	}
	const std::string detail = json["detail"].string_value();
	if (!detail.empty()) {
		return QString::fromStdString(detail);
	}
	const std::string title = json["title"].string_value();
	if (!title.empty()) {
		return QString::fromStdString(title);
	}
	const auto &errors = json["errors"].array_items();
	if (!errors.empty()) {
		const QString nested = ErrorText(errors[0]);
		if (!nested.isEmpty()) {
			return nested;
		}
	}
	const std::string error = json["error"].string_value();
	if (!error.empty()) {
		const std::string description = json["error_description"].string_value();
		if (!description.empty()) {
			return QString::fromStdString(error + ": " + description);
		}
		return QString::fromStdString(error);
	}
	return {};
}

} // namespace

XApiWrappers::XApiWrappers(const Def &d) : XAuth(d) {}

bool XApiWrappers::Request(const QString &url, const char *method, const char *body, Json &jsonOut, bool allowRefresh,
			   long *statusOut, bool followRedirects)
{
	lastError.clear();
	if ((token.empty() || TokenExpired()) && allowRefresh) {
		if (!RefreshToken()) {
			lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("unauthorized"));
			return false;
		}
	}
	if (token.empty()) {
		lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("unauthorized"));
		return false;
	}

	std::string output;
	std::string error;
	long status = 0;
	const std::vector<std::string> headers = {"Authorization: Bearer " + token, "Accept: application/json"};
	const bool success = GetRemoteFile(QT_TO_UTF8(url), output, error, &status, "application/json", method, body,
					   headers, nullptr, 20, false, 0, followRedirects);
	if (statusOut) {
		*statusOut = status;
	}

	if (status == 401 && allowRefresh) {
		if (!RefreshToken()) {
			lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("unauthorized"));
			return false;
		}
		return Request(url, method, body, jsonOut, false, statusOut, followRedirects);
	}

	if (!output.empty()) {
		std::string parseError;
		jsonOut = Json::parse(output, parseError);
		if (!parseError.empty()) {
			lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("invalid JSON"));
			return false;
		}
	}

	if (!success || status >= 400 || status == 0) {
		QString message = ErrorText(jsonOut);
		if (message.isEmpty()) {
			message = error.empty() ? QString::number(status) : QString::fromStdString(error);
		}
		if (XIsAccessDenied(status)) {
			lastError = QTStr("X.Settings.AccessDenied").arg(message.toHtmlEscaped());
		} else {
			lastError = QTStr("X.Actions.Error.Api").arg(message.toHtmlEscaped());
		}
		blog(LOG_WARNING, "X API %s failed (%ld)", method ? method : "GET", status);
		return false;
	}
	return true;
}

bool XApiWrappers::FetchIdentity()
{
	// Livestream paths require the numeric id from the token. /2/users/me is
	// the documented way to read it. users.read is not a livestream scope;
	// it exists so this call can return that id.
	Json json;
	const QString url = QStringLiteral("%1/2/users/me?user.fields=id,name,username").arg(XApiHost);
	if (!Request(url, "GET", nullptr, json, true, nullptr, false)) {
		return false;
	}
	const Json data = json["data"].is_object() ? json["data"] : json;
	const QString id = JsonAsString(data["id"]);
	if (id.isEmpty()) {
		lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("missing user id"));
		return false;
	}
	userId = id;
	QString name = JsonAsString(data["username"]);
	if (name.isEmpty()) {
		name = JsonAsString(data["name"]);
	}
	if (!name.isEmpty()) {
		username = name;
	}
	return true;
}

bool XApiWrappers::RecommendedRegion(QString &regionOut)
{
	Json json;
	long status = 0;
	const QString url = QStringLiteral("%1/2/region").arg(XApiHost);
	if (!Request(url, "GET", nullptr, json, true, &status, true)) {
		return false;
	}
	regionOut = JsonAsString(json["region"]);
	if (regionOut.isEmpty()) {
		lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("missing region"));
		return false;
	}
	return true;
}

bool XApiWrappers::ListSources(QVector<XStreamSource> &out)
{
	out.clear();
	if (userId.isEmpty() && !FetchIdentity()) {
		return false;
	}
	Json json;
	const QString url = QStringLiteral("%1/2/users/%2/sources").arg(XApiHost, userId);
	if (!Request(url, "GET", nullptr, json, true, nullptr, false)) {
		return false;
	}
	XCollectSourceResponse(json, out);
	return true;
}

bool XApiWrappers::GetSource(const QString &id, XStreamSource &out)
{
	if (userId.isEmpty() && !FetchIdentity()) {
		return false;
	}
	Json json;
	const QString url = QStringLiteral("%1/2/users/%2/sources/%3").arg(XApiHost, userId, id);
	if (!Request(url, "GET", nullptr, json, true, nullptr, false)) {
		return false;
	}
	out = XParseSource(json);
	if (out.id.isEmpty()) {
		lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("missing source"));
		return false;
	}
	return true;
}

bool XApiWrappers::CreateSource(const QString &name, const QString &regionName, XStreamSource &created)
{
	if (userId.isEmpty() && !FetchIdentity()) {
		return false;
	}
	const Json::object payload = {{"name", name.toStdString()}, {"region", regionName.toStdString()}};
	const std::string body = Json(payload).dump();
	Json json;
	const QString url = QStringLiteral("%1/2/users/%2/sources").arg(XApiHost, userId);
	if (!Request(url, "POST", body.c_str(), json, true, nullptr, false)) {
		return false;
	}
	created = XParseSource(json);
	if (created.streamKey.isEmpty() || XPreferredIngest(created).isEmpty()) {
		lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("source missing ingest"));
		return false;
	}
	return true;
}

void XApiWrappers::RememberSource(const XStreamSource &source)
{
	sourceId = source.id;
	if (!source.region.isEmpty()) {
		region = source.region;
	}
	ingestUrl = XPreferredIngest(source);
	if (!source.streamKey.isEmpty()) {
		key_ = source.streamKey.toStdString();
	}
}

bool XApiWrappers::EnsureSource()
{
	if (userId.isEmpty() && !FetchIdentity()) {
		return false;
	}

	XStreamSource chosen;
	QString recommended;
	const XEnsureResult result = XRunEnsureSource(
		sourceId, [&](QString &out) { return RecommendedRegion(out); },
		[&](const QString &id, XStreamSource &out) { return GetSource(id, out); },
		[&](QVector<XStreamSource> &out) { return ListSources(out); },
		[&](const QString &regionName, XStreamSource &created) {
			return CreateSource(QStringLiteral("OBS Studio"), regionName, created);
		},
		chosen, recommended);
	switch (result) {
	case XEnsureResult::Failed:
		return false;
	case XEnsureResult::ReusedSaved:
	case XEnsureResult::ReusedListed:
	case XEnsureResult::Created:
		region = recommended;
		RememberSource(chosen);
		ApplyIngestToService();
		Persist();
		return true;
	}
	return false;
}

void XApiWrappers::ApplyIngestToService()
{
	if (ingestUrl.isEmpty() || key_.empty()) {
		return;
	}
	OBSBasic *main = OBSBasic::Get();
	if (!main) {
		return;
	}
	obs_service_t *service = main->GetService();
	if (!service) {
		return;
	}
	OBSDataAutoRelease settings = obs_service_get_settings(service);
	obs_data_set_string(settings, "server", ingestUrl.toUtf8().constData());
	obs_data_set_string(settings, "key", key_.c_str());
	if (ingestUrl.startsWith(QStringLiteral("rtmps://"))) {
		obs_data_set_string(settings, "protocol", "RTMPS");
	}
	obs_service_update(service, settings);
}

bool XApiWrappers::WaitUntilStreamActive()
{
	if (sourceId.isEmpty()) {
		lastError = QTStr("X.Actions.Error.NeedSource");
		return false;
	}
	for (int attempt = 0; attempt < kStreamActiveAttempts; attempt++) {
		XStreamSource source;
		if (GetSource(sourceId, source) && source.streamActive) {
			RememberSource(source);
			return true;
		}
		if (attempt + 1 < kStreamActiveAttempts) {
			QThread::sleep(1);
		}
	}
	lastError = QTStr("X.Actions.Error.IngestInactive");
	return false;
}

bool XApiWrappers::CreateBroadcast(QString &createdId)
{
	if (userId.isEmpty() && !FetchIdentity()) {
		return false;
	}
	const Json::object payload = {
		{"source_id", sourceId.toStdString()},
		{"region", region.toStdString()},
		{"is_low_latency", pendingLowLatency},
	};
	const std::string body = Json(payload).dump();
	Json json;
	long status = 0;
	const QString url = QStringLiteral("%1/2/users/%2/broadcasts").arg(XApiHost, userId);
	if (!Request(url, "POST", body.c_str(), json, true, &status, false)) {
		return false;
	}
	const Json broadcast = json["broadcast"].is_object() ? json["broadcast"] : json["data"];
	createdId = JsonAsString(broadcast["broadcast_id"]);
	if (createdId.isEmpty()) {
		createdId = JsonAsString(broadcast["id"]);
	}
	if (createdId.isEmpty()) {
		lastError = QTStr("X.Actions.Error.Api").arg(QStringLiteral("missing broadcast id"));
		return false;
	}
	return true;
}

bool XApiWrappers::SetBroadcastState(const QString &id, const char *body)
{
	if (userId.isEmpty() && !FetchIdentity()) {
		return false;
	}
	Json json;
	const QString url = QStringLiteral("%1/2/users/%2/broadcasts/%3/state").arg(XApiHost, userId, id);
	return Request(url, "PUT", body, json, true, nullptr, false);
}

void XApiWrappers::SetPendingPublish(const QString &title, bool lowLatency, int chatOption, bool noTweet)
{
	pendingTitle = title;
	pendingLowLatency = lowLatency;
	pendingChatOption = chatOption;
	pendingNoTweet = noTweet;
	pendingPublish = true;
}

bool XApiWrappers::PublishPendingBroadcast()
{
	if (!pendingPublish) {
		return true;
	}
	SetStatus(QTStr("X.Settings.Status.Waiting"));
	QString created;
	const XGoLiveResult result = XRunGoLive([&]() { return WaitUntilStreamActive(); },
						[&](QString &id) {
							if (!CreateBroadcast(id)) {
								return false;
							}
							broadcastId = id;
							return true;
						},
						[&](const QString &id) {
							QString title = pendingTitle.trimmed();
							if (title.isEmpty()) {
								title = QStringLiteral("OBS Studio");
							}
							const std::string body = XPublishStateBody(
								title.toStdString(), pendingNoTweet, pendingChatOption);
							return SetBroadcastState(id, body.c_str());
						},
						created);
	switch (result) {
	case XGoLiveResult::FailedBeforeCreate:
	case XGoLiveResult::FailedCreate:
		pendingPublish = false;
		SetStatus(lastError);
		return false;
	case XGoLiveResult::FailedPublish:
		// The broadcast exists as NOT_STARTED. END is only RUNNING -> ENDED,
		// and any extra field is rejected. Leave it for the platform timeout.
		pendingPublish = false;
		broadcastPublished = false;
		blog(LOG_INFO,
		     "X broadcast '%s' was created but not published. Leaving it NOT_STARTED for platform timeout.",
		     QT_TO_UTF8(broadcastId));
		SetStatus(lastError);
		return false;
	case XGoLiveResult::Published:
		pendingPublish = false;
		broadcastPublished = true;
		SetStatus(QTStr("X.Settings.Status.Live"));
		return true;
	}
	return false;
}

bool XApiWrappers::EndPublishedBroadcast()
{
	// Connect failure never publishes. Publish failure keeps broadcastPublished
	// false, including when a NOT_STARTED broadcast id was stored. Both no-op.
	if (!XShouldEndBroadcast(broadcastPublished, !broadcastId.isEmpty())) {
		return true;
	}
	if (!SetBroadcastState(broadcastId, XEndStateBody())) {
		SetStatus(lastError);
		return false;
	}
	broadcastPublished = false;
	broadcastId.clear();
	SetStatus(QTStr("X.Settings.Status.Ended"));
	return true;
}

void XApiWrappers::OnStreamConfig()
{
	OAuthStreamKey::OnStreamConfig();
	ApplyIngestToService();
}

void XApiWrappers::Persist()
{
	SaveInternal();
	OBSBasic *main = OBSBasic::Get();
	if (main) {
		config_save_safe(main->Config(), "tmp", nullptr);
	}
}

void XApiWrappers::SaveInternal()
{
	XAuth::SaveInternal();
	OBSBasic *main = OBSBasic::Get();
	config_set_string(main->Config(), service(), "UserId", QT_TO_UTF8(userId));
	config_set_string(main->Config(), service(), "SourceId", QT_TO_UTF8(sourceId));
	config_set_string(main->Config(), service(), "Region", QT_TO_UTF8(region));
	config_set_string(main->Config(), service(), "IngestUrl", QT_TO_UTF8(ingestUrl));
	config_set_string(main->Config(), service(), "StreamKey", key_.c_str());
}

bool XApiWrappers::LoadInternal()
{
	if (!XAuth::LoadInternal()) {
		return false;
	}
	OBSBasic *main = OBSBasic::Get();
	auto read = [&](const char *name) {
		const char *value = config_get_string(main->Config(), service(), name);
		return value ? QString::fromUtf8(value) : QString();
	};
	userId = read("UserId");
	sourceId = read("SourceId");
	region = read("Region");
	ingestUrl = read("IngestUrl");
	const QString key = read("StreamKey");
	if (!key.isEmpty()) {
		key_ = key.toStdString();
	}
	return true;
}
