#pragma once

#include "OAuth.hpp"

class QCefWidget;

struct RestreamEventDescription {
	std::string id;
	std::string title;
	qint64 scheduledFor;
	std::string showId;
};

class RestreamAuth : public OAuthStreamKey {
	Q_OBJECT

	bool uiLoaded = false;
	std::string showId;

	QCefWidget *chatWidgetBrowser = NULL;
	QCefWidget *titlesWidgetBrowser = NULL;
	QCefWidget *channelWidgetBrowser = NULL;

	virtual bool RetryLogin() override;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	virtual void LoadUI() override;

public:
	RestreamAuth(const Def &d);
	~RestreamAuth();

	QVector<RestreamEventDescription> GetBroadcastInfo();
	std::string GetStreamingKey(std::string eventId);
	bool SelectShow(std::string eventId, std::string showId);
	void ResetShow();
	std::string GetShowId();
	bool IsBroadcastReady();

	static std::shared_ptr<Auth> Login(QWidget *parent, const std::string &service_name);
};

bool IsRestreamService(const std::string &service);
