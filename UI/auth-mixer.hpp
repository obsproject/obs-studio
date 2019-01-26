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

	bool TokenExpired();
	bool GetToken(const std::string &auth_code = std::string());

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	bool GetChannelInfo();

	virtual void LoadUI() override;
	virtual void OnStreamConfig() override;

public:
	MixerAuth(const Def &d);

	static std::shared_ptr<Auth> Login(QWidget *parent);
};
