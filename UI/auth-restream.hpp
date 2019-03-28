#pragma once

#include "auth-oauth.hpp"

class RestreamWidget;

class RestreamAuth : public OAuthStreamKey {
	Q_OBJECT

	QSharedPointer<RestreamWidget> chat;
	QSharedPointer<RestreamWidget> info;
	QSharedPointer<QAction> chatMenu;
	QSharedPointer<QAction> infoMenu;
	bool uiLoaded = false;

	virtual bool RetryLogin() override;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	bool GetChannelInfo();

	virtual void LoadUI() override;

public:
	RestreamAuth(const Def &d);

	static std::shared_ptr<Auth> Login(QWidget *parent);
};
