#pragma once

#include <QDialog>
#include <string>
#include <memory>

#include "auth-oauth.hpp"

class TwitchChat;

class TwitchAuth : public OAuthStreamKey {
	Q_OBJECT

	friend class TwitchLogin;

	QSharedPointer<TwitchChat> chat;
	QSharedPointer<QAction> chatMenu;
	bool uiLoaded = false;

	std::string name;

	virtual void SaveInternal() override;
	virtual bool LoadInternal() override;

	bool GetChannelInfo();

	virtual void LoadUI() override;
	virtual void OnStreamConfig() override;

public:
	TwitchAuth(const Def &d);

	static std::shared_ptr<Auth> Login(QWidget *parent);
};
