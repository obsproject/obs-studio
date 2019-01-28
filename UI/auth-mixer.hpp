#pragma once

#include "auth-oauth.hpp"

class MixerChat;

class MixerAuth : public OAuthStreamKey {
	Q_OBJECT

	QSharedPointer<MixerChat> chat;
	QSharedPointer<QAction> chatMenu;
	bool uiLoaded = false;

	std::string name;
	std::string id;
	int currentScopeVer = 0;

	bool TokenExpired();
	bool RetryLogin();
	bool GetToken(const std::string &auth_code = std::string(),
			bool retry = false);

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	bool GetChannelInfo();

	virtual void LoadUI() override;
	virtual void OnStreamConfig() override;

public:
	MixerAuth(const Def &d);

	static std::shared_ptr<Auth> Login(QWidget *parent);
};
