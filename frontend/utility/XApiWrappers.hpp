#pragma once

#include <oauth/XAuth.hpp>
#include <utility/XBroadcastLogic.hpp>

#include <QString>

inline bool IsXService(const std::string &service)
{
	return service == xServiceDef.service;
}

class XApiWrappers : public XAuth {
	Q_OBJECT

	QString lastError;
	QString userId;
	QString region;
	QString sourceId;
	QString ingestUrl;
	QString broadcastId;
	bool broadcastPublished = false;

	bool pendingPublish = false;
	QString pendingTitle;
	bool pendingLowLatency = true;
	int pendingChatOption = 2;
	bool pendingNoTweet = false;
	QString statusText;

	bool Request(const QString &url, const char *method, const char *body, json11::Json &jsonOut, bool allowRefresh,
		     long *statusOut, bool followRedirects);

	bool GetSource(const QString &id, XStreamSource &out);
	bool ListSources(QVector<XStreamSource> &out);
	bool CreateSource(const QString &name, const QString &regionName, XStreamSource &created);
	bool RecommendedRegion(QString &regionOut);
	bool CreateBroadcast(QString &createdId);
	bool SetBroadcastState(const QString &id, const char *body);
	void RememberSource(const XStreamSource &source);

public:
	explicit XApiWrappers(const Def &d);

	QString LastError() const { return lastError; }
	QString StatusText() const { return statusText; }
	void SetStatus(const QString &text) { statusText = text; }
	QString IngestUrl() const { return ingestUrl; }
	QString Region() const { return region; }
	QString SourceId() const { return sourceId; }
	QString UserId() const { return userId; }

	void SetStreamKey(const QString &key) { key_ = key.toStdString(); }

	bool FetchIdentity();
	bool EnsureSource();
	void ApplyIngestToService();
	bool WaitUntilStreamActive();
	void SetPendingPublish(const QString &title, bool lowLatency, int chatOption, bool noTweet);
	bool HasPendingPublish() const { return pendingPublish; }
	bool PublishPendingBroadcast();
	bool EndPublishedBroadcast();
	void Persist();

	virtual void OnStreamConfig() override;
	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;
};
